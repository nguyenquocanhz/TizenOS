#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

// Khởi tạo các module
extern void media_db_init(void);
extern void media_scanner_start(void);

int main(int argc, char *argv[]) {
    // Khởi tạo database SQLite để lưu trữ metadata
    media_db_init();

    // Khởi động trình quét file (sử dụng GStreamer discoverer)
    media_scanner_start();

    // Vòng lặp chính của daemon
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_print("TizenOS Media Server đang chạy...\n");
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    return 0;
}
