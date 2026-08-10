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

# Cài đặt GRUB EFI bootloader cho raw disk image
echo "Cài đặt GRUB bootloader..."
mount --bind /dev "$MOUNT_DIR/dev"
mount --bind /dev/pts "$MOUNT_DIR/dev/pts"
mount -t proc /proc "$MOUNT_DIR/proc"
mount -t sysfs /sys "$MOUNT_DIR/sys"

# Mount efivarfs nếu có (Samsung Tizen best practice)
if [ -d /sys/firmware/efi/efivars ]; then
    mkdir -p "$MOUNT_DIR/sys/firmware/efi/efivars" 2>/dev/null || true
    mount --bind /sys/firmware/efi/efivars "$MOUNT_DIR/sys/firmware/efi/efivars" 2>/dev/null || true
fi

# Cài GRUB EFI
if command -v grub-install >/dev/null 2>&1; then
    chroot "$MOUNT_DIR" grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=TizenOS --recheck --removable 2>/dev/null || true
    chroot "$MOUNT_DIR" update-grub 2>/dev/null || true
fi

# Tạo fstab
ROOT_UUID=$(blkid -s UUID -o value "${LOOP_DEV}p2")
EFI_UUID=$(blkid -s UUID -o value "${LOOP_DEV}p1")
cat > "$MOUNT_DIR/etc/fstab" << FSTAB_EOF
UUID=$ROOT_UUID  /             ext4  noatime,errors=remount-ro  0  1
UUID=$EFI_UUID   /boot/efi     vfat  umask=0077                 0  2
tmpfs            /tmp          tmpfs defaults,noatime,mode=1777 0  0
FSTAB_EOF

# Unmount virtual filesystems
umount "$MOUNT_DIR/sys/firmware/efi/efivars" 2>/dev/null || true
umount "$MOUNT_DIR/proc" 2>/dev/null || true
umount "$MOUNT_DIR/sys" 2>/dev/null || true
umount "$MOUNT_DIR/dev/pts" 2>/dev/null || true
umount "$MOUNT_DIR/dev" 2>/dev/null || true

# Dọn dẹp
umount "$MOUNT_DIR/boot/efi"
umount "$MOUNT_DIR"
rm -rf "$MOUNT_DIR"
losetup -d "$LOOP_DEV"

echo "Hoàn thành việc tạo disk image $IMG_FILE thành công!"
