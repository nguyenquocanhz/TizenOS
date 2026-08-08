#include "iso-mount.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Sử dụng: %s <mount|unmount> <file_iso_hoac_mount_point> [mount_point]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "mount") == 0) {
        if (argc < 4) {
            printf("Thiếu đường dẫn mount_point\n");
            return 1;
        }
        if (mount_iso(argv[2], argv[3])) {
            printf("Gắn kết thành công!\n");
        } else {
            printf("Gắn kết thất bại!\n");
        }
    } else if (strcmp(argv[1], "unmount") == 0) {
        if (unmount_iso(argv[2])) {
            printf("Ngắt gắn kết thành công!\n");
        } else {
            printf("Ngắt gắn kết thất bại!\n");
        }
    } else {
        printf("Lệnh không hợp lệ\n");
    }

    return 0;
}
