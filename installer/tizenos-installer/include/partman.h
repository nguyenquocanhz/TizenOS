#ifndef PARTMAN_H
#define PARTMAN_H

#include <stdbool.h>

// Xóa toàn bộ đĩa và tạo phân vùng mới
bool partman_erase_disk(const char *disk);

// Thiết lập mã hóa LUKS
bool partman_setup_luks(const char *disk, const char *passphrase);

// Định dạng phân vùng EFI (ESP) và rootfs (ext4)
bool partman_format_partitions(const char *esp_part, const char *root_part);

#endif
