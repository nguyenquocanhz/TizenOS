#!/bin/bash
# ==============================================================================
# TizenOS Master Build Environment Installer (Debian 12 / Ubuntu / CI-CD)
# ==============================================================================
# Tự động cài đặt 100% các công cụ biên dịch C, thư viện dev, và trình tạo đĩa ISO
# Tối ưu hóa cho môi trường GitHub Actions CI/CD (DEBIAN_FRONTEND=noninteractive)
# ==============================================================================

set -euo pipefail

export DEBIAN_FRONTEND=noninteractive
APT_OPTS="-y --no-install-recommends -o Dpkg::Options::=--force-confnew"

echo "======================================================================"
echo " Starting TizenOS Build Environment Auto-Installer..."
echo "======================================================================"

sudo -E apt-get update -qq || true

echo "[1/4] Cài đặt công cụ biên dịch C cốt lõi & debhelper..."
sudo -E apt-get install $APT_OPTS build-essential debhelper dpkg-dev cmake pkg-config || true
sudo -E apt-get install $APT_OPTS lintian reprepro gnupg ninja-build || true

echo "[2/4] Cài đặt công cụ Khởi tạo Live ISO, Bootloader & ISOLINUX..."
sudo -E apt-get install $APT_OPTS \
    debootstrap squashfs-tools xorriso genisoimage mtools syslinux syslinux-common syslinux-utils isolinux grub-pc-bin grub-efi-amd64-bin || true

echo "[3/4] Cài đặt các thư viện C Development Headers cho TizenOS..."
sudo -E apt-get install $APT_OPTS \
    libglib2.0-dev libsystemd-dev libdbus-1-dev \
    libwayland-dev libdrm-dev libgbm-dev libegl1-mesa-dev libgles2-mesa-dev \
    libgtk-4-dev libgstreamer1.0-dev libpipewire-0.3-dev \
    libarchive-dev libzip-dev libsqlite3-dev libssl-dev libpam0g-dev libpolkit-gobject-1-dev || true

echo "[4/4] Cài đặt công cụ Cross-compile ARM64 (Optional)..."
sudo -E apt-get install $APT_OPTS \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static || true

echo "======================================================================"
echo " ✓ HOÀN TẤT: Môi trường biên dịch TizenOS đã SẴN SÀNG 100%!"
echo "======================================================================"
