#ifndef __TIZEN_APP_H__
#define __TIZEN_APP_H__

// Khai báo các hàm quản lý vòng đời ứng dụng Tizen
// Hỗ trợ ứng dụng khởi tạo, tạm dừng, tiếp tục và kết thúc

typedef struct {
    void (*create)(void *data);
    void (*terminate)(void *data);
    void (*pause)(void *data);
    void (*resume)(void *data);
    void (*control)(void *app_control, void *data);
} app_callbacks_t;

int app_main(int argc, char **argv, app_callbacks_t *callbacks, void *user_data);
void app_exit(void);

#endif // __TIZEN_APP_H__
