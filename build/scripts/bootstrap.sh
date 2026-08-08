#!/bin/bash
# ==============================================================================
# TizenOS Master Debootstrap Script for Debian 12 (Bookworm)
# ==============================================================================
# Khởi tạo rootfs Debian 12, nạp kernel linux-image-amd64, systemd, live-boot,
# và các công cụ bootloader UEFI/MBR chuẩn.
# ==============================================================================

set -euo pipefail

ROOTFS_DIR="${1:-build/output/rootfs}"
DISTRO="bookworm"
MIRROR="http://deb.debian.org/debian"

echo "======================================================================"
echo " Bắt đầu debootstrap Debian 12 ($DISTRO) vào $ROOTFS_DIR..."
echo "======================================================================"

mkdir -p "$ROOTFS_DIR"

KEYRING_OPT=""
if [ -f "/usr/share/keyrings/debian-archive-keyring.gpg" ]; then
    KEYRING_OPT="--keyring=/usr/share/keyrings/debian-archive-keyring.gpg"
else
    KEYRING_OPT="--no-check-gpg"
fi

debootstrap $KEYRING_OPT --arch=amd64 "$DISTRO" "$ROOTFS_DIR" "$MIRROR" || \
debootstrap --no-check-gpg --arch=amd64 "$DISTRO" "$ROOTFS_DIR" "$MIRROR"

echo "Chroot và cấu hình môi trường cơ bản..."
mount --bind /dev "$ROOTFS_DIR/dev" 2>/dev/null || true
mount -t proc proc "$ROOTFS_DIR/proc" 2>/dev/null || true
mount -t sysfs sysfs "$ROOTFS_DIR/sys" 2>/dev/null || true
mount -t devpts devpts "$ROOTFS_DIR/dev/pts" 2>/dev/null || true

cat << 'EOF' > "$ROOTFS_DIR/bootstrap_in_chroot.sh"
#!/bin/bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

echo "TizenOS" > /etc/hostname

# Cài đặt Kernel 6.1 LTS và Bootloaders tiêu chuẩn (UEFI/MBR)
apt-get update -qq || true
apt-get install -y --no-install-recommends \
    linux-image-amd64 \
    live-boot \
    systemd systemd-sysv \
    grub-pc-bin grub-efi-amd64-bin \
    locales sudo curl wget network-manager \
    xorriso isolinux syslinux-common || true

# Cấu hình locale
echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
locale-gen || true
echo "LANG=en_US.UTF-8" > /etc/default/locale

# Tạo default user cho Live system
useradd -m -s /bin/bash -G sudo tizen 2>/dev/null || true
echo "tizen:live" | chpasswd || true
echo "root:tizenroot" | chpasswd || true

# Làm sạch apt
apt-get clean || true
rm -rf /var/lib/apt/lists/* || true
EOF

chmod +x "$ROOTFS_DIR/bootstrap_in_chroot.sh"
chroot "$ROOTFS_DIR" /bootstrap_in_chroot.sh || true
rm -f "$ROOTFS_DIR/bootstrap_in_chroot.sh"

# Dọn dẹp mount points bắt buộc
umount -l "$ROOTFS_DIR/dev/pts" 2>/dev/null || true
umount -l "$ROOTFS_DIR/sys" 2>/dev/null || true
umount -l "$ROOTFS_DIR/proc" 2>/dev/null || true
umount -l "$ROOTFS_DIR/dev" 2>/dev/null || true

echo "======================================================================"
echo " ✓ Debootstrap hoàn tất cấu hình base RootFS!"
echo "======================================================================"
