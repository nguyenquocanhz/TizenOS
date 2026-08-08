#!/bin/bash
# ==============================================================================
# TizenOS Master Build Environment Installer (Debian 12 / Ubuntu / WSL)
# ==============================================================================
# Tự động cài đặt 100% các công cụ biên dịch C, thư viện dev, và trình tạo đĩa ISO
# ==============================================================================

set -euo pipefail

echo "======================================================================"
echo " Starting TizenOS Build Environment Auto-Installer..."
echo "======================================================================"

sudo apt-get update -qq

echo "[1/4] Cài đặt công cụ biên dịch C & Đóng gói Debian..."
sudo apt-get install -y --no-install-recommends \
    build-essential debhelper dpkg-dev cmake pkg-config lintian reprepro gnupg Ninja-build

echo "[2/4] Cài đặt công cụ Khởi tạo Live ISO & Bootloader..."
sudo apt-get install -y --no-install-recommends \
    debootstrap squashfs-tools xorriso mtools syslinux-utils isolinux grub-pc-bin grub-efi-amd64-bin grub-efi-ia32-bin

echo "[3/4] Cài đặt các thư viện C Development Headers cho TizenOS..."
sudo apt-get install -y --no-install-recommends \
    libglib2.0-dev libgio2.0-cil-dev libsystemd-dev libdbus-1-dev \
    libwayland-dev libwlroots-dev libdrm-dev libgbm-dev libegl1-mesa-dev libgles2-mesa-dev \
    libgtk-4-dev libgstreamer1.0-dev libpipewire-0.3-dev \
    libarchive-dev libzip-dev libsqlite3-dev libssl-dev libpam0g-dev libpolkit-gobject-1-dev

echo "[4/4] Cài đặt công cụ Cross-compile ARM64 (Optional)..."
sudo apt-get install -y --no-install-recommends \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static || true

echo "======================================================================"
echo " ✓ HOÀN TẤT: Môi trường biên dịch TizenOS đã SẴN SÀNG 100%!"
echo " Bạn có thể chạy ngay lệnh: make all hoặc ./build/scripts/build-packages.sh"
echo "======================================================================"
