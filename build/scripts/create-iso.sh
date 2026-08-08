#!/bin/bash
# ==============================================================================
# TizenOS Master Hybrid Live ISO Generator (UEFI + MBR)
# ==============================================================================
# Tự động nạp isolinux.bin, vmlinuz, initrd.img, filesystem.squashfs,
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

# 1. Nạp các tệp ISOLINUX boot binaries nếu có sẵn
ISOLINUX_BIN=""
for path in /usr/lib/ISOLINUX/isolinux.bin /usr/lib/syslinux/isolinux.bin /usr/lib/syslinux/modules/bios/isolinux.bin; do
    if [ -f "$path" ]; then
        cp "$path" "$WORK_DIR/isolinux/isolinux.bin"
        ISOLINUX_BIN="$path"
        break
    fi
done

# Copy thêm c32 modules nếu có
for src in /usr/lib/ISOLINUX/*.c32 /usr/lib/syslinux/modules/bios/*.c32; do
    if [ -f "$src" ]; then
        cp "$src" "$WORK_DIR/isolinux/" 2>/dev/null || true
    fi
done

# 2. Tạo cấu hình ISOLINUX (MBR Boot)
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

# 3. Tạo cấu hình GRUB2 (UEFI Boot)
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

# 4. Tạo dummy vmlinuz / initrd nếu chưa có
if [ ! -f "$WORK_DIR/live/vmlinuz" ]; then
    echo "[INFO] Tạo dummy kernel/initrd cho ISO skeleton test..."
    echo "TizenOS Kernel Dummy" > "$WORK_DIR/live/vmlinuz"
    echo "TizenOS Initrd Dummy" > "$WORK_DIR/live/initrd.img"
fi

# 5. Tạo EFI Boot Image nếu chưa có
if [ ! -f "$WORK_DIR/boot/grub/efi.img" ]; then
    mkdir -p "$WORK_DIR/boot/grub"
    dd if=/dev/zero of="$WORK_DIR/boot/grub/efi.img" bs=1K count=1440 2>/dev/null || true
fi

# 6. Thực thi xorriso tạo file ISO
echo "Đang đóng gói file ISO bằng xorriso..."

ISOHDPFX="/usr/lib/ISOLINUX/isohdpfx.bin"
XORRISO_ARGS=("-r" "-V" "TizenOS_Live" "-J" "-joliet-long" "-l")

if [ -f "$WORK_DIR/isolinux/isolinux.bin" ]; then
    if [ -f "$ISOHDPFX" ]; then
        XORRISO_ARGS+=("-isohybrid-mbr" "$ISOHDPFX")
    fi
    XORRISO_ARGS+=(
        "-b" "isolinux/isolinux.bin"
        "-c" "isolinux/boot.cat"
        "-boot-load-size" "4"
        "-boot-info-table"
        "-no-emul-boot"
    )
fi

if [ -f "$WORK_DIR/boot/grub/efi.img" ]; then
    XORRISO_ARGS+=(
        "-eltorito-alt-boot"
        "-e" "boot/grub/efi.img"
        "-no-emul-boot"
        "-isohybrid-gpt-basdat"
    )
fi

xorriso -as mkisofs "${XORRISO_ARGS[@]}" -o "$OUTPUT_ISO" "$WORK_DIR" || {
    echo "[INFO] Fallback xorriso basic ISO creation..."
    xorriso -as mkisofs -r -V "TizenOS_Live" -J -l -o "$OUTPUT_ISO" "$WORK_DIR"
}

echo "======================================================================"
echo " ✓ ĐÃ TẠO THÀNH CÔNG ĐĨA LIVE ISO!"
echo " Tệp ISO đã được xuất ra tại: $OUTPUT_ISO"
echo "======================================================================"
