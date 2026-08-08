#!/bin/bash
set -euo pipefail

# Kịch bản tạo Raw Disk Image (.img) hỗ trợ GPT + ESP cho UEFI và MBR (cho ARM64 / x86_64)

IMG_FILE="${1:-tizenos-disk.img}"
IMG_SIZE="${2:-4G}"
ROOTFS_DIR="${3:-rootfs}"

echo "Tạo disk image rỗng kích thước $IMG_SIZE..."
truncate -s "$IMG_SIZE" "$IMG_FILE"

echo "Phân vùng đĩa (GPT)..."
parted -s "$IMG_FILE" mklabel gpt
# Phân vùng EFI System Partition (ESP)
parted -s "$IMG_FILE" mkpart ESP fat32 1MiB 513MiB
parted -s "$IMG_FILE" set 1 esp on
# Phân vùng RootFS
parted -s "$IMG_FILE" mkpart ROOT ext4 513MiB 100%

# Cấu hình loop device
LOOP_DEV=$(losetup -fP --show "$IMG_FILE")

echo "Format các phân vùng..."
mkfs.fat -F32 "${LOOP_DEV}p1"
mkfs.ext4 -L rootfs "${LOOP_DEV}p2"

echo "Mount và chép dữ liệu rootfs..."
MOUNT_DIR=$(mktemp -d)
mount "${LOOP_DEV}p2" "$MOUNT_DIR"
mkdir -p "$MOUNT_DIR/boot/efi"
mount "${LOOP_DEV}p1" "$MOUNT_DIR/boot/efi"

# Chép rootfs sang disk image
cp -a "$ROOTFS_DIR/"* "$MOUNT_DIR/"

# (Tuỳ chọn) Thực hiện chroot để cài GRUB EFI / U-Boot bootloader cho raw image
# ...

# Dọn dẹp
umount "$MOUNT_DIR/boot/efi"
umount "$MOUNT_DIR"
rm -rf "$MOUNT_DIR"
losetup -d "$LOOP_DEV"

echo "Hoàn thành việc tạo disk image $IMG_FILE thành công!"
