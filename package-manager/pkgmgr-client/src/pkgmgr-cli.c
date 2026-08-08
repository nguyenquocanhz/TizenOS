#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int pkgmgr_install(const char *tpk_path);
extern int pkgmgr_uninstall(const char *pkg_id);

void usage() {
    printf("Sử dụng: tpkg <lệnh> [tham số]\n");
    printf("Các lệnh:\n");
    printf("  install <file.tpk>  Cài đặt gói TPK\n");
    printf("  remove <pkg_id>     Gỡ bỏ gói\n");
    printf("  list                Danh sách các gói đã cài đặt\n");
    printf("  info <pkg_id>       Xem thông tin gói\n");
    printf("  apt <lệnh_apt>      Cầu nối (proxy) sang hệ thống APT của Debian\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "install") == 0 && argc >= 3) {
        // Cài đặt gói TPK
        pkgmgr_install(argv[2]);
    } else if (strcmp(cmd, "remove") == 0 && argc >= 3) {
        // Gỡ bỏ gói
        pkgmgr_uninstall(argv[2]);
    } else if (strcmp(cmd, "apt") == 0 && argc >= 3) {
        // Cơ chế APT bridge: Chuyển hướng các lệnh apt-get quen thuộc sang hệ thống con APT
        // Điều này cho phép quản lý các gói hệ thống debian bằng chính lệnh tpkg
        printf("Chuyển hướng lệnh sang apt-get...\n");
        char apt_cmd[256];
        snprintf(apt_cmd, sizeof(apt_cmd), "apt-get %s", argv[2]);
        for (int i = 3; i < argc; i++) {
            strncat(apt_cmd, " ", sizeof(apt_cmd) - strlen(apt_cmd) - 1);
            strncat(apt_cmd, argv[i], sizeof(apt_cmd) - strlen(apt_cmd) - 1);
        }
        system(apt_cmd);
    } else {
        printf("Lệnh không hợp lệ hoặc thiếu tham số.\n");
        usage();
    }

    return 0;
}
