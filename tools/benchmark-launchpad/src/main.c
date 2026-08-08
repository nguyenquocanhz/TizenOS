/*
 * TizenOS Launchpad Benchmark Tool
 * =============================================================================
 * Đo lường chính xác thời gian khởi chạy ứng dụng (thời gian tính bằng Microseconds/Milliseconds):
 * 1. Phương pháp Truyền thống: fork() + execve() nạp từ đĩa thô.
 * 2. Phương pháp Pre-fork Pool (TizenOS Launchpad): Chuyển socket handle tới Worker nạp sẵn RAM.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <dlfcn.h>

#define NUM_TRIALS 10

static double get_time_ms(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) * 1000.0 +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000.0;
}

/* 1. Mở App theo phương pháp Truyền thống (Cold Start: fork + execve) */
static double benchmark_traditional(void) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();
    if (pid == 0) {
        // Giả lập nạp ứng dụng và nạp thư viện hệ thống
        execlp("true", "true", NULL);
        exit(0);
    } else {
        waitpid(pid, NULL, 0);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    return get_time_ms(start, end);
}

/* 2. Mở App theo phương pháp Pre-fork Pool (Warm Start: TizenOS Launchpad) */
static double benchmark_prefork(int socket_fd) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Gửi yêu cầu kích hoạt app qua socket tới tiến trình pre-fork sẵn
    char dummy_cmd[] = "RUN_APP";
    write(socket_fd, dummy_cmd, strlen(dummy_cmd));

    char response[64];
    read(socket_fd, response, sizeof(response));

    clock_gettime(CLOCK_MONOTONIC, &end);
    return get_time_ms(start, end);
}

static void mock_prefork_worker(int socket_fd) {
    // Nạp trước thư viện nặng vào RAM
    dlopen("libc.so.6", RTLD_NOW);
    
    char buf[64];
    while (read(socket_fd, buf, sizeof(buf)) > 0) {
        // Giả lập thực thi hàm app_main tức thì
        write(socket_fd, "OK", 2);
    }
    exit(0);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("====================================================================\n");
    printf(" 🚀 THỬ NGHIỆM ĐO THỜI GIAN KHỞI ĐỘNG ỨNG DỤNG (LAUNCHPAD BENCHMARK)\n");
    printf("====================================================================\n\n");

    // Khởi tạo cặp socket cho Pre-forked Worker
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair");
        return 1;
    }

    pid_t worker_pid = fork();
    if (worker_pid == 0) {
        close(sv[0]);
        mock_prefork_worker(sv[1]);
        exit(0);
    }
    close(sv[1]);

    double total_trad = 0.0;
    double total_prefork = 0.0;

    printf("Thực hiện %d lần thử nghiệm đo độ trễ...\n\n", NUM_TRIALS);
    printf(" Lần thử | Truyền thống (Cold Start) | Pre-fork Pool (TizenOS) | Tốc độ Tăng tốc\n");
    printf("--------------------------------------------------------------------------------\n");

    for (int i = 1; i <= NUM_TRIALS; i++) {
        double t_trad = benchmark_traditional();
        double t_prefork = benchmark_prefork(sv[0]);

        total_trad += t_trad;
        total_prefork += t_prefork;

        double speedup = t_trad / t_prefork;
        printf(" Lần %2d  | %12.4f ms         | %12.4f ms        |  Nhanh gấp %.1fx\n",
               i, t_trad, t_prefork, speedup);
        usleep(50000); // 50ms pause
    }

    double avg_trad = total_trad / NUM_TRIALS;
    double avg_prefork = total_prefork / NUM_TRIALS;

    printf("--------------------------------------------------------------------------------\n");
    printf(" 📊 KẾT QUẢ TRUNG BÌNH:\n");
    printf("    • Khởi động Truyền thống: %.4f ms\n", avg_trad);
    printf("    • TizenOS Pre-fork Pool  : %.4f ms\n", avg_prefork);
    printf("    👉 KẾT LUẬN: Launchpad Pre-fork giúp mở ứng dụng nhanh gấp %.1f LẦN!\n", avg_trad / avg_prefork);
    printf("====================================================================\n");

    kill(worker_pid, 9);
    close(sv[0]);
    return 0;
}
