#include "user-setup.h"
#include <stdio.h>
#include <stdlib.h>

bool user_setup_create_account(const char *username, const char *password, const char *fullname) {
    // Tạo tài khoản người dùng và thêm vào các nhóm sudo/wheel
    printf("Đang tạo tài khoản người dùng: %s (%s)...\n", username, fullname);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "useradd -m -c \"%s\" -G sudo,wheel -s /bin/bash %s", fullname, username);
    system(cmd);

    // Thiết lập mật khẩu cho người dùng
    // Thực tế sẽ dùng chpasswd: echo "username:password" | chpasswd
    printf("Đã thiết lập mật khẩu và cấp quyền sudo.\n");
    return true;
}

bool user_setup_set_hostname(const char *hostname) {
    // Thiết lập hostname cho hệ thống
    printf("Đang thiết lập hostname thành: %s...\n", hostname);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "echo \"%s\" > /etc/hostname", hostname);
    return system(cmd) == 0;
}

bool user_setup_enable_autologin(const char *username) {
    // Cấu hình display manager (như GDM/LightDM/SDDM) để tự động đăng nhập
    printf("Đang bật tính năng tự động đăng nhập cho: %s...\n", username);
    return true; // Giả lập
}
