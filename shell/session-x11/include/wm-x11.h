#ifndef WM_X11_H
#define WM_X11_H

#include <X11/Xlib.h>

/* Khởi tạo Window Manager */
int wm_init(Display *dpy);

/* Chạy vòng lặp sự kiện của Window Manager */
void wm_run(void);

/* Dọn dẹp tài nguyên */
void wm_cleanup(void);

#endif /* WM_X11_H */
