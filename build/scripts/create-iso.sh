#!/bin/bash
# ==============================================================================
# TizenOS Master Hybrid Live ISO Generator (UEFI + MBR)
# ==============================================================================
# Tự động nạp debootstrap Debian 12 RootFS (với sudo/chroot), mksquashfs nén hệ thống,
# nạp vmlinuz, initrd.img, isolinux.bin, cấu hình GRUB2 UEFI / ISOLINUX MBR,
# và xuất file tizenos-live.iso hoàn chỉnh bằng xorriso.
# ==============================================================================

set -euo pipefail

WORK_DIR="${1:-build/output/iso_work}"
OUTPUT_ISO="${2:-build/output/tizenos-live.iso}"
ROOTFS_DIR="${3:-build/output/rootfs}"

echo "======================================================================"
echo " Khởi tạo Đóng gói TizenOS Hybrid Live ISO: $OUTPUT_ISO"
echo "======================================================================"

mkdir -p "$WORK_DIR/live"
mkdir -p "$WORK_DIR/isolinux"
mkdir -p "$WORK_DIR/boot/grub"

# 1. Nếu chưa có filesystem.squashfs, tiến hành debootstrap & mksquashfs
if [ ! -f "$WORK_DIR/live/filesystem.squashfs" ] || [ ! -s "$WORK_DIR/live/filesystem.squashfs" ]; then
    echo "======================================================================"
    echo " [1/3] Khởi tạo Debian 12 RootFS bằng debootstrap..."
    echo "======================================================================"
    if [ ! -d "$ROOTFS_DIR" ] || [ ! -e "$ROOTFS_DIR/sbin/init" ]; then
        chmod +x build/scripts/bootstrap.sh 2>/dev/null || true
        bash build/scripts/bootstrap.sh "$ROOTFS_DIR"
    fi

    echo "Dọn dẹp virtual mounts trước khi nén squashfs..."
    umount -l "$ROOTFS_DIR/proc" 2>/dev/null || true
    umount -l "$ROOTFS_DIR/sys" 2>/dev/null || true
    umount -l "$ROOTFS_DIR/dev" 2>/dev/null || true

    echo "Đang nén RootFS thành filesystem.squashfs..."
    mksquashfs "$ROOTFS_DIR" "$WORK_DIR/live/filesystem.squashfs" -comp xz -e proc sys dev run tmp
fi

# 2. Copy Kernel & Initrd
echo "======================================================================"
echo " [2/3] Nạp Kernel (vmlinuz) & Initrd..."
echo "======================================================================"
KERNEL_FILE=$(ls -1 "$ROOTFS_DIR/boot/vmlinuz-"* 2>/dev/null | sort -V | tail -n 1 || true)
INITRD_FILE=$(ls -1 "$ROOTFS_DIR/boot/initrd.img-"* 2>/dev/null | sort -V | tail -n 1 || true)

if [ -n "$KERNEL_FILE" ] && [ -f "$KERNEL_FILE" ]; then
    cp "$KERNEL_FILE" "$WORK_DIR/live/vmlinuz"
else
    echo "[ERROR] Không tìm thấy vmlinuz trong $ROOTFS_DIR/boot!"
    exit 1
fi

if [ -n "$INITRD_FILE" ] && [ -f "$INITRD_FILE" ]; then
    cp "$INITRD_FILE" "$WORK_DIR/live/initrd.img"
else
    echo "[ERROR] Không tìm thấy initrd.img trong $ROOTFS_DIR/boot!"
    exit 1
fi

# 3. Tạo cấu hình ISOLINUX Bootloader cho MBR / Legacy BIOS
if [ -f "build/boot/isolinux/isolinux.cfg" ]; then
    echo "Sử dụng isolinux.cfg tĩnh từ build/boot/isolinux/"
    cp "build/boot/isolinux/isolinux.cfg" "$WORK_DIR/isolinux/isolinux.cfg"
else
cat << 'ISOLINUX_EOF' > "$WORK_DIR/isolinux/isolinux.cfg"
UI vesamenu.c32
PROMPT 0
TIMEOUT 50
ONTIMEOUT tizenos

MENU TITLE TizenOS 1.0 Live System (Debian Edition)
MENU BACKGROUND /isolinux/splash.png

LABEL tizenos
    MENU LABEL TizenOS Live System (Standard Boot & Installer)
    KERNEL /live/vmlinuz
    APPEND initrd=/live/initrd.img boot=live components union=overlay tizenos.installer=1 quiet

LABEL tizenos-toram
    MENU LABEL TizenOS Live System (Copy to RAM - 4GB+ RAM required)
    KERNEL /live/vmlinuz
    APPEND initrd=/live/initrd.img boot=live components union=overlay toram tizenos.installer=1 quiet

LABEL tizenos-safe
    MENU LABEL TizenOS Live (Safe Mode - Compatible with VMware/VirtualBox)
    KERNEL /live/vmlinuz
    APPEND initrd=/live/initrd.img boot=live components union=overlay nomodeset quiet

LABEL tizenos-debug
    MENU LABEL TizenOS Live (Debug Mode - Serial Console)
    KERNEL /live/vmlinuz
    APPEND initrd=/live/initrd.img boot=live components union=overlay nomodeset earlyprintk=serial console=ttyS0,115200 console=tty0 systemd.log_level=debug
ISOLINUX_EOF
fi

# Copy isolinux binaries & toàn bộ các module COM32 (*.c32)
if [ -d "/usr/lib/ISOLINUX" ] || [ -d "/usr/lib/syslinux" ]; then
    cp /usr/lib/ISOLINUX/isolinux.bin "$WORK_DIR/isolinux/" 2>/dev/null || true
    cp /usr/lib/ISOLINUX/isohdpfx.bin "$WORK_DIR/isolinux/" 2>/dev/null || true
    cp /usr/lib/syslinux/modules/bios/*.c32 "$WORK_DIR/isolinux/" 2>/dev/null || true
    cp /usr/lib/syslinux/bios/*.c32 "$WORK_DIR/isolinux/" 2>/dev/null || true
fi

# 4. Tạo cấu hình GRUB2 Bootloader cho UEFI
if [ -f "build/boot/grub/grub-uefi.cfg" ]; then
    echo "Sử dụng grub-uefi.cfg tĩnh từ build/boot/grub/"
    cp "build/boot/grub/grub-uefi.cfg" "$WORK_DIR/boot/grub/grub.cfg"
else
cat << 'GRUB_EOF' > "$WORK_DIR/boot/grub/grub.cfg"
set default=0
set timeout=5

insmod part_gpt
insmod part_msdos
insmod fat
insmod ext2
insmod all_video
insmod gfxterm

set theme=/boot/grub/theme.txt

menuentry "TizenOS 1.0 Live System (Standard Boot & Installer)" {
    linux /live/vmlinuz boot=live components union=overlay tizenos.installer=1 quiet
    initrd /live/initrd.img
}

menuentry "TizenOS 1.0 Live System (Copy to RAM - 4GB+ RAM required)" {
    linux /live/vmlinuz boot=live components union=overlay toram tizenos.installer=1 quiet
    initrd /live/initrd.img
}

menuentry "TizenOS 1.0 Live (Safe Mode - Compatible with VMware/VirtualBox)" {
    linux /live/vmlinuz boot=live components union=overlay nomodeset quiet
    initrd /live/initrd.img
}

menuentry "TizenOS 1.0 Live (Debug Mode - Serial Console)" {
    linux /live/vmlinuz boot=live components union=overlay nomodeset earlyprintk=serial console=ttyS0,115200 console=tty0 systemd.log_level=debug
    initrd /live/initrd.img
}
GRUB_EOF
fi

# 5. Tạo EFI System Partition (efi.img) nếu có grub-mkstandalone
if command -v grub-mkstandalone >/dev/null 2>&1; then
    echo "Đang tạo BOOTX64.EFI chuẩn bằng grub-mkstandalone..."
    mkdir -p /tmp/efi_work/EFI/BOOT
    cat << 'EFI_EARLY_EOF' > /tmp/efi_work/grub.cfg
search --file --set=root /live/vmlinuz
set prefix=($root)/boot/grub
configfile $prefix/grub.cfg
EFI_EARLY_EOF

    grub-mkstandalone -O x86_64-efi -o /tmp/efi_work/EFI/BOOT/BOOTX64.EFI "boot/grub/grub.cfg=/tmp/efi_work/grub.cfg" 2>/dev/null || true

    if [ -f "/tmp/efi_work/EFI/BOOT/BOOTX64.EFI" ]; then
        echo "Đang khởi tạo đĩa EFI System Partition (efi.img)..."
        dd if=/dev/zero of="$WORK_DIR/boot/grub/efi.img" bs=1M count=8 2>/dev/null || true
        mkfs.fat -F12 "$WORK_DIR/boot/grub/efi.img" >/dev/null 2>&1 || true
        mmd -i "$WORK_DIR/boot/grub/efi.img" ::EFI ::EFI/BOOT 2>/dev/null || true
        mcopy -i "$WORK_DIR/boot/grub/efi.img" /tmp/efi_work/EFI/BOOT/BOOTX64.EFI ::EFI/BOOT/ 2>/dev/null || true
        rm -rf /tmp/efi_work
    fi
fi

# 6. Đóng gói ISO hoàn chỉnh bằng xorriso
echo "======================================================================"
echo " Đang đóng gói file ISO bằng xorriso..."
echo "======================================================================"

XORRISO_ARGS=(
    "-as" "mkisofs"
    "-r" "-V" "TizenOS_Live"
    "-J" "-joliet-long"
    "-l"
)

if [ -f "$WORK_DIR/isolinux/isolinux.bin" ]; then
    XORRISO_ARGS+=(
        "-isohybrid-mbr" "/usr/lib/ISOLINUX/isohdpfx.bin"
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

rm -f "$OUTPUT_ISO" 2>/dev/null || true

xorriso "${XORRISO_ARGS[@]}" -o "$OUTPUT_ISO" "$WORK_DIR" || {
    echo "[INFO] Fallback xorriso basic ISO creation..."
    xorriso -as mkisofs -r -V "TizenOS_Live" -J -l -o "$OUTPUT_ISO" "$WORK_DIR"
}

SIZE=$(du -h "$OUTPUT_ISO" 2>/dev/null | cut -f1 || echo "unknown")
echo "======================================================================"
echo " ✓ ĐÃ TẠO THÀNH CÔNG ĐĨA HYBRID LIVE ISO (Dung lượng: $SIZE)!"
echo " Tệp ISO đã me xuất ra tại: $OUTPUT_ISO"
echo "======================================================================"
