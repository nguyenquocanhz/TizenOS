#ifndef TARGET_INSTALL_H
#define TARGET_INSTALL_H

#include <stdbool.h>

// Sao chép rootfs từ Squashfs sang phân vùng đích
bool target_install_copy_rootfs(const char *source, const char *dest);

// Tạo file fstab với Root, EFI và Swap UUID
bool target_install_generate_fstab(const char *root_part, const char *esp_part, const char *swap_part);

// Cài đặt GRUB2 bootloader kép (UEFI/MBR)
bool target_install_grub(const char *disk, bool is_uefi);

#endif
