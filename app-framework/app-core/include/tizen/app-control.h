#ifndef __TIZEN_APP_CONTROL_H__
#define __TIZEN_APP_CONTROL_H__

// Cấu trúc IPC app-control để gọi các ứng dụng khác
// Quản lý việc gửi/nhận yêu cầu khởi chạy

void app_control_send_launch_request(const char *app_id);

#endif // __TIZEN_APP_CONTROL_H__
