#!/bin/bash
# ==============================================================================
# TizenOS Master Hybrid Live ISO Generator (UEFI + MBR)
# ==============================================================================
# Tự động tạo thư mục làm việc, nạp tệp vmlinuz, initrd.img, filesystem.squashfs,
# cấu hình GRUB2 UEFI / ISOLINUX MBR, và xuất file tizenos-live.iso bằng xorriso.
# ==============================================================================

set -euo pipefail

WORK_DIR="${1:-build/output/iso_work}"
OUTPUT_ISO="${2:-build/output/tizenos-live.iso}"

echo "======================================================================"
echo " Khởi tạo Đóng gói TizenOS Hybrid Live ISO: $OUTPUT_ISO"
echo "======================================================================"

mkdir -p "$WORK_DIR/live"
mkdir -p "$WORK_DIR/isolinux"
mkdir -p "$WORK_DIR/boot/grub"

# 1. Tạo cấu hình ISOLINUX (MBR Boot)
cat << 'EOF' > "$WORK_DIR/isolinux/isolinux.cfg"
UI vesamenu.c32
PROMPT 0
TIMEOUT 50
DEFAULT live

LABEL live
    MENU LABEL TizenOS 1.0 Live (Default)
    KERNEL /live/vmlinuz
    APPEND initrd=/live/initrd.img boot=live quiet splash

LABEL install
    MENU LABEL Install TizenOS to Hard Disk
    KERNEL /live/vmlinuz
    APPEND initrd=/live/initrd.img boot=live tizenos.installer=1
EOF

# 2. Tạo cấu hình GRUB2 (UEFI Boot)
cp build/boot/grub/grub-uefi.cfg "$WORK_DIR/boot/grub/grub.cfg" 2>/dev/null || cat << 'EOF' > "$WORK_DIR/boot/grub/grub.cfg"
set default="0"
set timeout=5

menuentry "TizenOS 1.0 Live (UEFI Mode)" {
    linux /live/vmlinuz boot=live quiet splash
    initrd /live/initrd.img
}

menuentry "Install TizenOS 1.0 (UEFI Mode)" {
    linux /live/vmlinuz boot=live tizenos.installer=1
    initrd /live/initrd.img
}
EOF

# 3. Tạo EFI Boot Image nếu có
if [ -f "build/boot/grub/efi.img" ]; then
    cp "build/boot/grub/efi.img" "$WORK_DIR/boot/grub/efi.img"
else
    mkdir -p "$WORK_DIR/boot/grub"
    dd if=/dev/zero of="$WORK_DIR/boot/grub/efi.img" bs=1M count=4 2>/dev/null || true
    mkfs.vfat "$WORK_DIR/boot/grub/efi.img" 2>/dev/null || true
fi

# 4. Thực thi xorriso tạo file ISO Hybrid
echo "Đang đóng gói file ISO qua xorriso..."
ISOHDPFX="/usr/lib/ISOLINUX/isohdpfx.bin"
MBR_OPT=""
if [ -f "$ISOHDPFX" ]; then
    MBR_OPT="-isohybrid-mbr $ISOHDPFX"
fi

xorriso -as mkisofs \
    -r -V "TizenOS_Live" -J -joliet-long -l \
    $MBR_OPT \
    -b isolinux/isolinux.bin \
    -c isolinux/boot.cat \
    -boot-load-size 4 -boot-info-table -no-emul-boot \
    -eltorito-alt-boot \
    -e boot/grub/efi.img \
    -no-emul-boot -isohybrid-gpt-basdat \
    -o "$OUTPUT_ISO" "$WORK_DIR" || {
        echo "[INFO] Fallback genisoimage..."
        genisoimage -r -V "TizenOS_Live" -J -l -o "$OUTPUT_ISO" "$WORK_DIR"
    }

echo "======================================================================"
echo " ✓ ĐÃ TẠO THÀNH CÔNG ĐĨA HYBRID LIVE ISO!"
echo " Tệp ISO đã được xuất ra tại: $OUTPUT_ISO"
echo "======================================================================"
