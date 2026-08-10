#include "user-setup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TARGET_MOUNT "/mnt/target"

bool user_setup_create_account(const char *username, const char *password, const char *fullname) {
    printf("[USER-SETUP] Tạo tài khoản người dùng trên /mnt/target: %s (%s)...\n", username, fullname);
    char cmd[1024];

    // Debian sử dụng nhóm sudo và nopasswdlogin
    snprintf(cmd, sizeof(cmd),
        "chroot " TARGET_MOUNT " /bin/bash -c \"groupadd -r nopasswdlogin 2>/dev/null || true; useradd -m -c '%s' -G sudo,nopasswdlogin -s /bin/bash %s 2>/dev/null || usermod -aG sudo,nopasswdlogin %s 2>/dev/null || true\"",
        fullname, username, username);
    system(cmd);

    // Thiết lập mật khẩu thật bằng chpasswd
    snprintf(cmd, sizeof(cmd),
        "chroot " TARGET_MOUNT " /bin/bash -c \"echo '%s:%s' | chpasswd\"",
        username, password);
    int res = system(cmd);

    // Thiết lập mật khẩu root mặc định
    snprintf(cmd, sizeof(cmd),
        "chroot " TARGET_MOUNT " /bin/bash -c \"echo 'root:tizenroot' | chpasswd\"");
    system(cmd);

    // Bảo đảm quyền sở hữu thư mục cá nhân
    snprintf(cmd, sizeof(cmd),
        "chroot " TARGET_MOUNT " /bin/bash -c \"chown -R %s:%s /home/%s 2>/dev/null || true\"",
        username, username, username);
    system(cmd);

    printf("[USER-SETUP] ✓ Đã thiết lập tài khoản & mật khẩu thành công!\n");
    return (res == 0);
}

bool user_setup_set_hostname(const char *hostname) {
    printf("[USER-SETUP] Thiết lập hostname thành: %s...\n", hostname);
    char path[512];
    snprintf(path, sizeof(path), "%s/etc/hostname", TARGET_MOUNT);

    FILE *fp = fopen(path, "w");
    if (!fp) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "echo \"%s\" > /etc/hostname", hostname);
        return system(cmd) == 0;
    }
    fprintf(fp, "%s\n", hostname);
    fclose(fp);
    return true;
}

bool user_setup_enable_autologin(const char *username) {
    printf("[USER-SETUP] Bật tính năng tự động đăng nhập LightDM cho: %s...\n", username);
    char conf_path[512];
    snprintf(conf_path, sizeof(conf_path), "%s/etc/lightdm/lightdm.conf.d/80-tizenos-autologin.conf", TARGET_MOUNT);

    system("mkdir -p " TARGET_MOUNT "/etc/lightdm/lightdm.conf.d");
    FILE *fp = fopen(conf_path, "w");
    if (fp) {
        fprintf(fp, "[Seat:*]\nautologin-user=%s\nautologin-user-timeout=0\nuser-session=xfce\npam-service=lightdm-autologin\n", username);
        fclose(fp);
    }
    return true;
}
