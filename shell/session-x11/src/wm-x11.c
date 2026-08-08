#include "../include/wm-x11.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>

static Display *display;
static Window root;
static int running = 1;

/* Khởi tạo Window Manager */
int wm_init(Display *dpy) {
    display = dpy;
    root = DefaultRootWindow(display);

    /* Đăng ký sự kiện SubstructureRedirectMask để quản lý cửa sổ */
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask);
    XSync(display, False);

    /* Cấu hình các Atoms cho EWMH */
    Atom net_supported = XInternAtom(display, "_NET_SUPPORTED", False);
    Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
    Atom net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    
    Atom supported[] = { net_supported, net_wm_name, net_active_window };
    XChangeProperty(display, root, net_supported, XA_ATOM, 32, PropModeReplace, (unsigned char*)supported, 3);

    return 0;
}

/* Xử lý yêu cầu tạo cửa sổ mới */
static void handle_map_request(XMapRequestEvent *e) {
    /* Đặt cửa sổ hiển thị */
    XMapWindow(display, e->window);
    
    /* Thiết lập focus cho cửa sổ mới */
    XSetInputFocus(display, e->window, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(display, e->window);
    
    /* Cập nhật _NET_ACTIVE_WINDOW */
    Atom net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    XChangeProperty(display, root, net_active_window, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&e->window, 1);
}

/* Chạy vòng lặp sự kiện chính */
void wm_run(void) {
    XEvent ev;
    while (running && !XNextEvent(display, &ev)) {
        switch (ev.type) {
            case MapRequest:
                handle_map_request(&ev.xmaprequest);
                break;
            /* Các sự kiện khác như ConfigureRequest, DestroyNotify... có thể được xử lý tại đây */
            default:
                break;
        }
    }
}

/* Dọn dẹp */
void wm_cleanup(void) {
    running = 0;
}
