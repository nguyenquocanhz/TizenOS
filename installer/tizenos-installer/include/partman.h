#ifndef PARTMAN_H
#define PARTMAN_H

#include <stdbool.h>

// Xóa toàn bộ đĩa và tạo phân vùng mới GPT kép (BIOS + UEFI)
bool partman_erase_disk(const char *disk);

// Thiết lập mã hóa LUKS
bool partman_setup_luks(const char *disk, const char *passphrase);

// Định dạng phân vùng EFI (ESP FAT32), SWAP và Rootfs (ext4)
bool partman_format_partitions(const char *esp_part, const char *swap_part, const char *root_part);

#endif
