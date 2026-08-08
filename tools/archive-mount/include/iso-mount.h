#ifndef ISO_MOUNT_H
#define ISO_MOUNT_H

#include <stdbool.h>

/**
 * Gắn kết ổ đĩa ảo (ISO, IMG, NRG, BIN).
 * Sử dụng udisks2, losetup, hoặc fuseiso.
 */
bool mount_iso(const char *iso_path, const char *mount_point);

/**
 * Ngắt gắn kết ổ đĩa ảo.
 */
bool unmount_iso(const char *mount_point);

#endif // ISO_MOUNT_H
