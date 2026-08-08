#include <stdio.h>
#include "tizen/app-control.h"

// Hàm gửi request chạy ứng dụng qua IPC intent system
void app_control_send_launch_request(const char *app_id) {
    printf("Gửi yêu cầu launch qua AppControl cho app: %s\n", app_id);
}
