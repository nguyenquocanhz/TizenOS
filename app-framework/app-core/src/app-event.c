#include <stdio.h>
#include "tizen/app-event.h"

// Đăng ký nhận sự kiện hệ thống (pin yếu, thay đổi ngôn ngữ, v.v)
// Dispatch sự kiện tới ứng dụng thông qua dbus/glib loop
void app_event_register_system_events(void) {
    printf("Đăng ký lắng nghe system events.\n");
}
