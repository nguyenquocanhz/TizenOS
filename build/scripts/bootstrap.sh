#!/bin/bash
set -euo pipefail

# Kịch bản de-bootstrap cho Debian 12 (Bookworm) và cài đặt các thành phần TizenOS
# Cấu hình rootfs, cài grub-pc, grub-efi, systemd, kernel 6.1 LTS, v.v.

ROOTFS_DIR="${1:-rootfs}"
DISTRO="bookworm"
MIRROR="http://deb.debian.org/debian"

echo "Bắt đầu debootstrap Debian 12 ($DISTRO)..."
mkdir -p "$ROOTFS_DIR"
debootstrap --arch=amd64 "$DISTRO" "$ROOTFS_DIR" "$MIRROR"

echo "Chroot và cấu hình môi trường cơ bản..."
mount --bind /dev "$ROOTFS_DIR/dev"
mount -t proc proc "$ROOTFS_DIR/proc"
mount -t sysfs sysfs "$ROOTFS_DIR/sys"
mount -t devpts devpts "$ROOTFS_DIR/dev/pts"

cat << 'EOF' > "$ROOTFS_DIR/bootstrap_in_chroot.sh"
#!/bin/bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

echo "TizenOS" > /etc/hostname

# Cài đặt Kernel 6.1 LTS và Bootloaders cho Dual Boot (UEFI/MBR)
apt-get update
apt-get install -y --no-install-recommends \
    linux-image-amd64 \
    live-boot \
    systemd systemd-sysv \
    grub-pc-bin grub-efi-amd64-bin grub-efi-amd64-signed grub-efi-arm64-signed \
    locales sudo curl wget vim network-manager \
    memtest86+ xorriso isolinux syslinux-common

# Cấu hình locale
echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
locale-gen
echo "LANG=en_US.UTF-8" > /etc/default/locale

# Tạo default user cho Live system
useradd -m -s /bin/bash -G sudo tizen
echo "tizen:live" | chpasswd
echo "root:tizenroot" | chpasswd

# Làm sạch apt
apt-get clean
rm -rf /var/lib/apt/lists/*
EOF

chmod +x "$ROOTFS_DIR/bootstrap_in_chroot.sh"
chroot "$ROOTFS_DIR" /bootstrap_in_chroot.sh
rm "$ROOTFS_DIR/bootstrap_in_chroot.sh"

umount "$ROOTFS_DIR/dev/pts"
umount "$ROOTFS_DIR/sys"
umount "$ROOTFS_DIR/proc"
umount "$ROOTFS_DIR/dev"

echo "Bootstrap hoàn thành cấu hình base RootFS!"
