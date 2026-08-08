#!/bin/bash
set -euo pipefail

# Kịch bản tạo Hybrid ISO (hỗ trợ cả Legacy MBR/BIOS và UEFI x86_64)
# Sử dụng xorriso -as mkisofs với chuẩn Debian Live ISO

WORK_DIR="${1:-iso_work}"
OUTPUT_ISO="${2:-tizenos-live.iso}"

if [ ! -d "$WORK_DIR" ]; then
    echo "Thư mục làm việc $WORK_DIR không tồn tại!"
    exit 1
fi

echo "Tạo file ISO Hybrid ($OUTPUT_ISO)..."

# xorriso để build ISO hỗ trợ boot MBR (qua ISOLINUX) và UEFI (qua GRUB EFI)
xorriso -as mkisofs \
    -r -V "TizenOS_Live" -J -joliet-long -l \
    -isohybrid-mbr /usr/lib/ISOLINUX/isohdpfx.bin \
    -b isolinux/isolinux.bin \
    -c isolinux/boot.cat \
    -boot-load-size 4 -boot-info-table -no-emul-boot \
    -eltorito-alt-boot \
    -e boot/grub/efi.img \
    -no-emul-boot -isohybrid-gpt-basdat \
    -o "$OUTPUT_ISO" "$WORK_DIR"

echo "Quá trình tạo Hybrid ISO thành công!"
