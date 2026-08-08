#include "partman.h"
#include <stdio.h>
#include <stdlib.h>

bool partman_erase_disk(const char *disk) {
    // Xóa phân vùng bằng cách sử dụng sgdisk hoặc parted
    // Lệnh: sgdisk --zap-all /dev/sda
    printf("Đang xóa toàn bộ dữ liệu trên ổ đĩa %s...\n", disk);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sgdisk --zap-all %s", disk);
    return system(cmd) == 0;
}

bool partman_setup_luks(const char *disk, const char *passphrase) {
    // Thiết lập LUKS (Linux Unified Key Setup) để mã hóa toàn bộ ổ đĩa
    // Lệnh: cryptsetup luksFormat /dev/sda2
    printf("Đang thiết lập mã hóa LUKS trên %s...\n", disk);
    // Lưu ý: Thực tế cần truyền passphrase qua stdin cho cryptsetup
    return true; // Giả lập thành công
}

bool partman_format_partitions(const char *esp_part, const char *root_part) {
    // Định dạng phân vùng boot (ESP) với FAT32
    printf("Đang định dạng phân vùng EFI (ESP): %s...\n", esp_part);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "mkfs.fat -F32 %s", esp_part);
    system(cmd);

    // Định dạng rootfs với ext4
    printf("Đang định dạng phân vùng Rootfs: %s...\n", root_part);
    snprintf(cmd, sizeof(cmd), "mkfs.ext4 -F %s", root_part);
    return system(cmd) == 0;
}
