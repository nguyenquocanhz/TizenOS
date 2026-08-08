#!/bin/bash
set -euo pipefail

# Script cài đặt các dependencies cần thiết để build TizenOS trên Debian 12

MINIMAL=0
FULL=0

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --minimal) MINIMAL=1; shift ;;
        --full) FULL=1; shift ;;
        *) echo "Tham số không hợp lệ: $1"; exit 1 ;;
    esac
done

echo "Đang cập nhật danh sách gói..."
sudo apt-get update

# Cài đặt các công cụ cốt lõi
echo "Đang cài đặt các công cụ build cốt lõi..."
sudo apt-get install -y build-essential debhelper dpkg-dev cmake pkg-config lintian reprepro gnupg

if [ "$MINIMAL" -eq 1 ]; then
    echo "Đã hoàn tất cài đặt minimal."
    exit 0
fi

# Cài đặt các công cụ cross-compile nếu cần (chế độ full)
if [ "$FULL" -eq 1 ]; then
    echo "Đang cài đặt cross-compilation tools cho arm64..."
    sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static
fi

# Cài đặt thư viện dev cho TizenOS
echo "Đang cài đặt các thư viện phát triển (lib dev)..."
sudo apt-get install -y libwayland-dev libdrm-dev libgbm-dev libegl1-mesa-dev libgles2-mesa-dev libsystemd-dev libdbus-1-dev

echo "Kiểm tra cài đặt..."
dpkg -l build-essential debhelper cmake reprepro | grep "^ii"

echo "Đã cài đặt thành công các build dependencies."
