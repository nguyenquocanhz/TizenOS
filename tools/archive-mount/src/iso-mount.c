#include "iso-mount.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Gắn kết ISO thông qua udisksctl hoặc lệnh mount loop
bool mount_iso(const char *iso_path, const char *mount_point) {
    char cmd[512];
    // Sử dụng udisksctl để thiết lập loop device (tự động phân bổ thiết bị loop)
    snprintf(cmd, sizeof(cmd), "udisksctl loop-setup -r -f \"%s\"", iso_path);
    int ret = system(cmd);
    if (ret != 0) {
        // Fallback sử dụng mount loop trực tiếp
        snprintf(cmd, sizeof(cmd), "sudo mount -o loop \"%s\" \"%s\"", iso_path, mount_point);
        ret = system(cmd);
    }
    return ret == 0;
}

// Ngắt gắn kết
bool unmount_iso(const char *mount_point) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "sudo umount \"%s\"", mount_point);
    return system(cmd) == 0;
}
