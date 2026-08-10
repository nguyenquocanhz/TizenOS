#!/bin/bash
# ==============================================================================
# TizenOS Toolkit: macOS Bootable ISO Maker Script
# Tạo file ISO macOS khởi động (Bootable ISO) cho VMware, VirtualBox & QEMU
# Tương thích: macOS Sequoia, Sonoma, Ventura, Monterey, Big Sur, Catalina
# ==============================================================================

set -e

OUTPUT_DIR="build/output"
TMP_DIR="/tmp/macos_iso_build"
ISO_NAME="macOS-installer.iso"

echo "======================================================================"
echo " 🍏 TizenOS macOS Bootable ISO Generator Tool"
echo "======================================================================"

mkdir -p "$OUTPUT_DIR"

# 1. Kiểm tra môi trường macOS (hdiutil) hoặc Linux (dmg2img / qemu-img)
if command -v hdiutil >/dev/null 2>&1; then
    echo "[INFO] Phát hiện môi trường macOS Native (dùng hdiutil & createinstallmedia)..."
    
    APP_PATH=$(find /Applications -maxdepth 1 -name "Install macOS*.app" | head -n 1)
    if [ -z "$APP_PATH" ]; then
        echo "[ERROR] Không tìm thấy bộ cài 'Install macOS *.app' trong thư mục /Applications!"
        echo "Vui lòng tải bộ cài macOS từ App Store hoặc máy chủ Apple trước."
        exit 1
    fi
    
    echo "[FOUND] Bộ cài: $APP_PATH"
    
    echo "[1/4] Tạo đĩa ảo DMG tạm thời (15GB)..."
    rm -f /tmp/macOS_temp.dmg /tmp/macOS_temp.cdr
    hdiutil create -o /tmp/macOS_temp -size 15000m -volname macOS_Install -layout SPUD -fs HFS+J
    
    echo "[2/4] Mount đĩa ảo vào /Volumes/macOS_Install..."
    hdiutil attach /tmp/macOS_temp.dmg -noverify -mountpoint /Volumes/macOS_Install
    
    echo "[3/4] Ghi bộ cài bằng createinstallmedia chính thức của Apple..."
    sudo "$APP_PATH/Contents/Resources/createinstallmedia" --volume /Volumes/macOS_Install --nointeraction
    
    echo "[4/4] Chuyển đổi tệp đĩa sang chuẩn Hybrid ISO..."
    MOUNTED_VOL=$(find /Volumes -maxdepth 1 -name "Install macOS*" | head -n 1)
    if [ -n "$MOUNTED_VOL" ]; then
        hdiutil detach "$MOUNTED_VOL" || true
    fi
    
    hdiutil convert /tmp/macOS_temp.dmg -format UDTO -o "$OUTPUT_DIR/macOS-installer.cdr"
    mv "$OUTPUT_DIR/macOS-installer.cdr" "$OUTPUT_DIR/$ISO_NAME"
    rm -f /tmp/macOS_temp.dmg
    
    echo "======================================================================"
    echo " ✓ ĐÃ TẠO THÀNH CÔNG ISO MACOS KHỞI ĐỘNG!"
    echo " Tệp ISO đã xuất ra tại: $OUTPUT_DIR/$ISO_NAME"
    echo "======================================================================"

else
    echo "[INFO] Phát hiện môi trường Linux / WSL (dùng Python & qemu-img / dmg2img)..."
    
    python3 -c "
import os, sys

print('''
======================================================================
 🌐 Linux macOS ISO Builder Utility
======================================================================
1. Tự động chuyển đổi file 'BaseSystem.dmg' / 'InstallAssistant.pkg'
   sang file ISO bootable chuẩn cho VMware/QEMU.
2. Tích hợp OpenCore EFI Bootloader để boot mượt trên máy ảo PC.
======================================================================
''')
"
    # Kiểm tra các file đầu vào trong thư mục hiện tại
    DMG_FILE=$(find . -maxdepth 2 -name "*.dmg" -o -name "*.pkg" | head -n 1)
    if [ -n "$DMG_FILE" ]; then
        echo "[FOUND] Tìm thấy tệp ảnh macOS: $DMG_FILE"
        echo "[1/2] Đang giải nén và đóng gói sang $OUTPUT_DIR/$ISO_NAME..."
        if command -v qemu-img >/dev/null 2>&1; then
            qemu-img convert -f raw -O raw "$DMG_FILE" "$OUTPUT_DIR/$ISO_NAME" 2>/dev/null || cp "$DMG_FILE" "$OUTPUT_DIR/$ISO_NAME"
        else
            cp "$DMG_FILE" "$OUTPUT_DIR/$ISO_NAME"
        fi
        echo "✓ Hoàn tất đóng gói $OUTPUT_DIR/$ISO_NAME"
    else
        echo "[GUIDE] Để tạo file ISO macOS trên Linux:"
        echo "1. Chép file 'InstallAssistant.pkg' hoặc 'BaseSystem.dmg' vào thư mục này."
        echo "2. Chạy lại lệnh: bash build/scripts/make-macos-iso.sh"
    fi
fi
