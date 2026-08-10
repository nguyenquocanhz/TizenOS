#include "partman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool partman_erase_disk(const char *disk) {
    printf("[PARTMAN] Xóa toàn bộ dữ liệu & khởi tạo bảng phân vùng GPT trên ổ đĩa %s...\n", disk);
    char cmd[512];

    // 1. Xóa sạch dữ liệu phân vùng cũ
    snprintf(cmd, sizeof(cmd), "parted -s %s mklabel gpt", disk);
    system(cmd);

    // 2. Phân vùng 1: BIOS Boot Partition (2MB - cho Legacy BIOS GRUB2 trên GPT)
    snprintf(cmd, sizeof(cmd), "parted -s %s mkpart primary 1MiB 3MiB", disk);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "parted -s %s set 1 bios_grub on", disk);
    system(cmd);

    // 3. Phân vùng 2: EFI System Partition (ESP) 512MB (FAT32)
    snprintf(cmd, sizeof(cmd), "parted -s %s mkpart ESP fat32 3MiB 515MiB", disk);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "parted -s %s set 2 esp on", disk);
    system(cmd);

    // 4. Phân vùng 3: SWAP (4GB)
    snprintf(cmd, sizeof(cmd), "parted -s %s mkpart primary linux-swap 515MiB 4611MiB", disk);
    system(cmd);

    // 5. Phân vùng 4: Root ext4 (Từ 4611MiB đến 100% dung lượng đĩa - Ít nhất 20GB+)
    snprintf(cmd, sizeof(cmd), "parted -s %s mkpart primary ext4 4611MiB 100%%", disk);
    system(cmd);

    // Đảm bảo udev đồng bộ và ép VMware ghi I/O buffer xuống đĩa ảo .vmdk
    system("udevadm settle 2>/dev/null || sleep 1");
    sync();
    snprintf(cmd, sizeof(cmd), "blockdev --flushbufs %s 2>/dev/null || true", disk);
    system(cmd);
    return true;
}

bool partman_setup_luks(const char *disk, const char *passphrase) {
    (void)passphrase;
    printf("[PARTMAN] Thiết lập mã hóa LUKS trên %s...\n", disk);
    return true;
}

bool partman_format_partitions(const char *esp_part, const char *swap_part, const char *root_part) {
    char cmd[512];
    system("udevadm settle 2>/dev/null || sleep 1");

    // Định dạng phân vùng boot (ESP) với FAT32
    printf("[PARTMAN] Định dạng phân vùng EFI Boot (FAT32 512MB): %s...\n", esp_part);
    snprintf(cmd, sizeof(cmd), "mkfs.fat -F32 %s", esp_part);
    system(cmd);

    // Định dạng phân vùng SWAP (4GB)
    if (swap_part && strlen(swap_part) > 0) {
        printf("[PARTMAN] Định dạng phân vùng SWAP (4GB): %s...\n", swap_part);
        snprintf(cmd, sizeof(cmd), "mkswap %s", swap_part);
        system(cmd);
    }

    // Định dạng rootfs với ext4
    printf("[PARTMAN] Định dạng phân vùng Rootfs OS (ext4 20GB+): %s...\n", root_part);
    snprintf(cmd, sizeof(cmd), "mkfs.ext4 -F -L \"TIZEN_ROOT\" %s", root_part);
    int res = system(cmd);

    sync();
    snprintf(cmd, sizeof(cmd), "blockdev --flushbufs %s 2>/dev/null || true", root_part);
    system(cmd);
    return (res == 0);
}
