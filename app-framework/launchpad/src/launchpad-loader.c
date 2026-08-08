/*
 * TizenOS Launchpad Pre-fork Pool Engine - Implementation
 * =============================================================================
 * Cơ chế Pre-fork Process Pool:
 * 1. Duy trì N tiến trình chờ sẵn trong RAM (Idle Worker Processes).
 * 2. Mỗi worker process đã nạp trước 90% thư viện hệ thống dùng chung:
 *    - libgtk-4.so, libglib-2.0.so, libwayland-client.so, libtizen-core.so.
 * 3. Khi nhận tín hiệu D-Bus mở App ID:
 *    - Đơn giản gửi Tên file shared library .so của App qua UNIX Socket.
 *    - Worker process gọi dlopen() và app_main() lập tức hiển thị UI < 50ms.
 * 4. Tự động fork() bù ngay 1 worker process mới vào Pool để duy trì số lượng.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>

#define PREFORK_POOL_SIZE 3
#define LAUNCHPAD_SOCKET_PATH "/run/tizen/launchpad.sock"

typedef struct {
    pid_t pid;
    int socket_fd;
    bool is_busy;
} PreForkWorker;

static PreForkWorker pool[PREFORK_POOL_SIZE];

/* Nạp trước các thư viện dùng chung nặng vào RAM của Worker Process */
static void preload_shared_libraries(void) {
    // Nạp trước các thư viện cơ sở để kích hoạt Copy-On-Write (COW)
    dlopen("libglib-2.0.so.0", RTLD_NOW | RTLD_GLOBAL);
    dlopen("libgio-2.0.so.0", RTLD_NOW | RTLD_GLOBAL);
    dlopen("libgtk-4.so.1", RTLD_NOW | RTLD_GLOBAL);
    dlopen("libwayland-client.so.0", RTLD_NOW | RTLD_GLOBAL);
}

/* Tiến trình Con Worker Process sau khi được fork() */
static void worker_process_main(int socket_pair_fd) {
    g_print("[LAUNCHPAD-WORKER-PID %d] Worker process ready & waiting for App launch signal...\n", getpid());

    char app_so_path[512];
    ssize_t bytes_read = read(socket_pair_fd, app_so_path, sizeof(app_so_path) - 1);
    if (bytes_read > 0) {
        app_so_path[bytes_read] = '\0';
        g_print("[LAUNCHPAD-WORKER-PID %d] 🚀 Launching App: %s (Pre-forked Instant Launch!)\n", getpid(), app_so_path);

        // Dynamically load the target App's .so or execute entrypoint
        void *app_handle = dlopen(app_so_path, RTLD_NOW);
        if (app_handle) {
            typedef int (*app_main_fn)(int, char**);
            app_main_fn app_main = (app_main_fn)dlsym(app_handle, "app_main");
            if (app_main) {
                char *args[] = { app_so_path, NULL };
                app_main(1, args);
            }
            dlclose(app_handle);
        }
    }
    close(socket_pair_fd);
    exit(EXIT_SUCCESS);
}

/* Khởi tạo 1 Worker mới vào Pool */
void launchpad_spawn_worker(int index) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // Child Process (Worker)
        close(sv[0]);
        preload_shared_libraries();
        worker_process_main(sv[1]);
    } else {
        // Parent Process (Launchpad Daemon)
        close(sv[1]);
        pool[index].pid = pid;
        pool[index].socket_fd = sv[0];
        pool[index].is_busy = false;
        g_print("[LAUNCHPAD-DAEMON] Spawned Pre-fork Worker #%d (PID: %d)\n", index, pid);
    }
}

/* Khởi tạo toàn bộ Pre-fork Pool */
void launchpad_init_pool(void) {
    g_print("[LAUNCHPAD-DAEMON] Initializing Pre-fork Process Pool (Size: %d)...\n", PREFORK_POOL_SIZE);
    for (int i = 0; i < PREFORK_POOL_SIZE; i++) {
        launchpad_spawn_worker(i);
    }
}

/* Kích hoạt khởi động App tức thì từ Pool */
bool launchpad_launch_app(const char *app_so_path) {
    for (int i = 0; i < PREFORK_POOL_SIZE; i++) {
        if (!pool[i].is_busy && pool[i].pid > 0) {
            g_print("[LAUNCHPAD-DAEMON] Allocating Worker #%d (PID: %d) for App: %s\n", i, pool[i].pid, app_so_path);
            
            // Gửi đường dẫn app sang worker process qua socket
            write(pool[i].socket_fd, app_so_path, strlen(app_so_path));
            close(pool[i].socket_fd);
            pool[i].is_busy = true;

            // Fork ngay 1 worker mới lấp chỗ trống vào pool
            launchpad_spawn_worker(i);
            return true;
        }
    }
    return false;
}
