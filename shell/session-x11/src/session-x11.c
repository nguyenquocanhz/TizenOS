#include "../include/wm-x11.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/* Khởi chạy tiến trình nền */
void spawn(const char *cmd) {
    if (fork() == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Không thể mở Display X11.\n");
        return EXIT_FAILURE;
    }

    /* Thiết lập hình nền (nếu cần) */
    spawn("xsetroot -solid '#222222'");

    /* Khởi động GTK4 panel / taskbar (giả lập) */
    spawn("tizenos-panel &");

    /* Khởi tạo và chạy Window Manager */
    if (wm_init(dpy) == 0) {
        printf("TizenOS X11 Window Manager started.\n");
        wm_run();
        wm_cleanup();
    }

    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
