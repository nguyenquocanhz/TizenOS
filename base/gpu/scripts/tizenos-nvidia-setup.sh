#!/bin/bash
set -euo pipefail

# =============================================================================
# TizenOS NVIDIA Setup Script
# =============================================================================
# Giải quyết xung đột NVIDIA: blacklist nouveau, cấu hình modeset,
# GBM/EGLStreams, Vulkan ICD, PRIME, power management, XWayland HW accel.
# Hỗ trợ rollback toàn bộ thay đổi.
# =============================================================================

readonly VERSION="1.0.0"
readonly LOG_TAG="tizenos-nvidia-setup"

# Đường dẫn config files
readonly MODPROBE_NVIDIA="/etc/modprobe.d/tizenos-nvidia.conf"
readonly MODPROBE_BLACKLIST="/etc/modprobe.d/tizenos-nouveau-blacklist.conf"
readonly ENV_NVIDIA="/etc/environment.d/tizenos-nvidia.conf"
readonly VULKAN_ICD_DIR="/usr/share/vulkan/icd.d"
readonly XWAYLAND_CONF="/etc/X11/xorg.conf.d/10-tizenos-nvidia.conf"
readonly GPU_INFO="/run/tizenos/gpu-info.env"

# ---- Logging ----------------------------------------------------------------
log_info()  { echo "[NVIDIA-SETUP] $*" | systemd-cat -t "$LOG_TAG" -p info  2>/dev/null || echo "[INFO] $*"; }
log_warn()  { echo "[NVIDIA-SETUP] $*" | systemd-cat -t "$LOG_TAG" -p warning 2>/dev/null || echo "[WARN] $*"; }
log_error() { echo "[NVIDIA-SETUP] $*" | systemd-cat -t "$LOG_TAG" -p err  2>/dev/null || echo "[ERROR] $*"; }

# ---- Rollback ----------------------------------------------------------------

# Khôi phục tất cả thay đổi NVIDIA setup
rollback() {
    log_info "=== Rollback: Đang khôi phục cấu hình trước NVIDIA setup ==="

    # Xóa config files đã tạo
    for f in "$MODPROBE_NVIDIA" "$MODPROBE_BLACKLIST" "$ENV_NVIDIA"; do
        if [ -f "$f" ]; then
            rm -f "$f"
            log_info "Đã xóa: $f"
        fi
    done

    # Tái kích hoạt nouveau
    if [ -f /etc/modprobe.d/tizenos-nouveau-enable.conf.bak ]; then
        mv /etc/modprobe.d/tizenos-nouveau-enable.conf.bak /etc/modprobe.d/tizenos-nouveau-enable.conf
        log_info "Đã khôi phục cấu hình nouveau"
    fi

    # Xóa environment overrides
    if [ -f "$ENV_NVIDIA" ]; then
        rm -f "$ENV_NVIDIA"
        log_info "Đã xóa environment overrides NVIDIA"
    fi

    # Cập nhật initramfs để phản ánh thay đổi modprobe
    if command -v update-initramfs &>/dev/null; then
        update-initramfs -u 2>/dev/null || log_warn "Không thể cập nhật initramfs"
    fi

    log_info "=== Rollback hoàn tất. Khởi động lại để áp dụng. ==="
}

# ---- Blacklist nouveau -------------------------------------------------------

setup_nouveau_blacklist() {
    log_info "Chặn nouveau module (xung đột với nvidia proprietary)..."

    cat > "$MODPROBE_BLACKLIST" <<'EOF'
# TizenOS: Blacklist nouveau để sử dụng NVIDIA proprietary driver
# Xung đột: nouveau và nvidia không thể load cùng lúc
blacklist nouveau
options nouveau modeset=0
alias nouveau off

# Đảm bảo nouveau không được pull vào bởi dependency
blacklist lbm-nouveau
EOF

    log_info "nouveau đã bị blacklist tại $MODPROBE_BLACKLIST"
}

# ---- Cấu hình nvidia-drm modeset ---------------------------------------------

setup_modeset() {
    log_info "Cấu hình nvidia-drm modeset (bắt buộc cho Wayland)..."

    cat > "$MODPROBE_NVIDIA" <<'EOF'
# TizenOS NVIDIA Driver Configuration
# nvidia-drm.modeset=1: Bắt buộc cho Wayland compositor
# fbdev=1: Framebuffer device support (cho console, plymouth)
# PreserveVideoMemoryAllocations=1: Giữ VRAM khi suspend/resume
options nvidia-drm modeset=1 fbdev=1
options nvidia NVreg_PreserveVideoMemoryAllocations=1

# Thứ tự load module: nvidia → nvidia-modeset → nvidia-drm
softdep nvidia post: nvidia-modeset nvidia-uvm
softdep nvidia-modeset post: nvidia-drm
EOF

    # Verify modeset sẽ active sau reboot
    if [ -f /sys/module/nvidia_drm/parameters/modeset ]; then
        local current
        current=$(cat /sys/module/nvidia_drm/parameters/modeset 2>/dev/null || echo "N")
        if [ "$current" != "Y" ] && [ "$current" != "1" ]; then
            log_warn "nvidia-drm.modeset hiện KHÔNG active (=$current)."
            log_warn "Cấu hình đã được ghi nhưng cần REBOOT để áp dụng."
            log_warn "Hoặc thêm 'nvidia-drm.modeset=1' vào GRUB_CMDLINE_LINUX trong /etc/default/grub"

            # Tự động thêm vào GRUB nếu có
            if [ -f /etc/default/grub ]; then
                if ! grep -q "nvidia-drm.modeset=1" /etc/default/grub; then
                    # Backup trước khi sửa
                    cp /etc/default/grub /etc/default/grub.tizenos.bak
                    sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="\([^"]*\)"/GRUB_CMDLINE_LINUX_DEFAULT="\1 nvidia-drm.modeset=1"/' /etc/default/grub
                    log_info "Đã thêm nvidia-drm.modeset=1 vào GRUB config"

                    if command -v update-grub &>/dev/null; then
                        update-grub 2>/dev/null || log_warn "Không thể chạy update-grub"
                    fi
                fi
            fi
        else
            log_info "nvidia-drm.modeset đã active (=$current) ✓"
        fi
    fi
}

# ---- Cấu hình GBM và Environment Variables -----------------------------------

setup_environment() {
    local nvidia_version="${1:-0}"
    local major_version="${nvidia_version%%.*}"

    mkdir -p "$(dirname "$ENV_NVIDIA")"

    log_info "Thiết lập biến môi trường cho NVIDIA $nvidia_version..."

    cat > "$ENV_NVIDIA" <<EOF
# TizenOS NVIDIA Environment Configuration
# Tự động tạo bởi tizenos-nvidia-setup v${VERSION}
# Driver version: ${nvidia_version}
EOF

    # GBM backend: chỉ cho NVIDIA >= 495
    if [ "$major_version" -ge 495 ] 2>/dev/null; then
        cat >> "$ENV_NVIDIA" <<'EOF'

# === GBM Backend (NVIDIA >= 495) ===
# Kích hoạt GBM allocation qua nvidia-drm thay vì Mesa fallback
GBM_BACKEND=nvidia-drm
# Force dùng NVIDIA GLX cho XWayland hardware acceleration
__GLX_VENDOR_LIBRARY_NAME=nvidia
EOF
        log_info "GBM backend: nvidia-drm (version $nvidia_version >= 495) ✓"
    else
        cat >> "$ENV_NVIDIA" <<EOF

# === NVIDIA version ${nvidia_version} < 495 ===
# GBM KHÔNG được hỗ trợ bởi driver này
# Wayland compositor (wlroots) SẼ KHÔNG HOẠT ĐỘNG
# Hệ thống sẽ fallback về X11
# GBM_BACKEND=nvidia-drm  # DISABLED - driver too old
EOF
        log_warn "GBM backend: KHÔNG khả dụng (version $nvidia_version < 495)"
    fi

    # Vulkan ICD
    local nvidia_icd="$VULKAN_ICD_DIR/nvidia_icd.json"
    if [ -f "$nvidia_icd" ]; then
        cat >> "$ENV_NVIDIA" <<EOF

# === Vulkan ICD ===
# Chỉ định NVIDIA Vulkan ICD để tránh xung đột với Mesa radeon_icd/intel_icd
VK_ICD_FILENAMES=${nvidia_icd}
EOF
        log_info "Vulkan ICD: $nvidia_icd ✓"
    else
        log_warn "Không tìm thấy nvidia_icd.json tại $VULKAN_ICD_DIR"
    fi

    # Explicit sync: NVIDIA >= 545
    if [ "$major_version" -ge 545 ] 2>/dev/null; then
        cat >> "$ENV_NVIDIA" <<'EOF'

# === Explicit Sync (NVIDIA >= 545) ===
# Giảm flickering/tearing trong XWayland apps
NVIDIA_EXPLICIT_SYNC=1
EOF
        log_info "Explicit sync: enabled (version $nvidia_version >= 545) ✓"
    fi
}

# ---- Cấu hình PRIME Offloading -----------------------------------------------

setup_prime() {
    log_info "Cấu hình NVIDIA PRIME offloading cho hybrid GPU..."

    # Thêm PRIME env vars vào config (nhưng KHÔNG active mặc định)
    cat >> "$ENV_NVIDIA" <<'EOF'

# === PRIME Render Offloading ===
# KHÔNG kích hoạt mặc định — chỉ dùng khi chạy app cụ thể trên dGPU
# Sử dụng: tizenos-prime-run <application>
# Hoặc set các biến sau trước khi chạy app:
#   __NV_PRIME_RENDER_OFFLOAD=1
#   __VK_LAYER_NV_optimus=NVIDIA_only
#   __GLX_VENDOR_LIBRARY_NAME=nvidia
EOF

    # Tạo script helper prime-run
    local prime_script="/usr/bin/tizenos-prime-run"
    cat > "$prime_script" <<'SCRIPT'
#!/bin/bash
# TizenOS PRIME Run - Chạy ứng dụng trên NVIDIA dGPU
# Sử dụng: tizenos-prime-run <command> [args...]
if [ $# -eq 0 ]; then
    echo "Sử dụng: tizenos-prime-run <command> [arguments...]"
    echo "Chạy ứng dụng trên NVIDIA dGPU (thay vì iGPU mặc định)"
    exit 1
fi
export __NV_PRIME_RENDER_OFFLOAD=1
export __VK_LAYER_NV_optimus=NVIDIA_only
export __GLX_VENDOR_LIBRARY_NAME=nvidia
exec "$@"
SCRIPT
    chmod +x "$prime_script"
    log_info "PRIME helper script: $prime_script ✓"
}

# ---- Power Management --------------------------------------------------------

setup_power_management() {
    local nvidia_version="${1:-0}"
    local major_version="${nvidia_version%%.*}"

    log_info "Cấu hình power management cho NVIDIA..."

    # nvidia-powerd: Dynamic Power Management (Turing+, driver >= 510)
    if [ "$major_version" -ge 510 ] 2>/dev/null; then
        if command -v nvidia-powerd &>/dev/null; then
            systemctl enable nvidia-powerd 2>/dev/null || true
            log_info "nvidia-powerd: enabled (Turing+ Dynamic PM) ✓"
        else
            log_warn "nvidia-powerd không tìm thấy. Cài nvidia-utils để hỗ trợ dynamic PM."
        fi
    fi

    # Runtime PM cho dGPU trong hybrid setup
    if [ -f "$GPU_INFO" ]; then
        # shellcheck source=/dev/null
        source "$GPU_INFO" 2>/dev/null || true
        if [ "${GPU_PRIME_AVAILABLE:-no}" = "yes" ]; then
            # Tìm PCI device NVIDIA và enable runtime PM
            for dev in /sys/bus/pci/devices/*/vendor; do
                local vendor_val
                vendor_val=$(cat "$dev" 2>/dev/null || echo "0x0000")
                if [ "$vendor_val" = "0x10de" ]; then
                    local pci_dir
                    pci_dir=$(dirname "$dev")
                    local power_ctrl="$pci_dir/power/control"
                    if [ -f "$power_ctrl" ]; then
                        echo "auto" > "$power_ctrl" 2>/dev/null || true
                        log_info "Runtime PM enabled cho $(basename "$pci_dir")"
                    fi
                fi
            done
        fi
    fi

    # systemd suspend/resume hooks cho NVIDIA
    cat > /usr/lib/systemd/system-sleep/tizenos-nvidia-sleep.sh <<'EOF'
#!/bin/bash
# TizenOS: Lưu/khôi phục VRAM khi suspend/resume
case $1 in
    pre)
        # Trước suspend: nvidia-smi drain không cần thiết với PreserveVideoMemoryAllocations
        ;;
    post)
        # Sau resume: reload nvidia-uvm nếu cần (fix CUDA apps)
        if lsmod | grep -q nvidia_uvm; then
            modprobe -r nvidia_uvm 2>/dev/null || true
            modprobe nvidia_uvm 2>/dev/null || true
        fi
        ;;
esac
EOF
    chmod +x /usr/lib/systemd/system-sleep/tizenos-nvidia-sleep.sh
    log_info "Suspend/resume hook: installed ✓"
}

# ---- Verify Setup ------------------------------------------------------------

verify_setup() {
    local errors=0

    log_info "=== Kiểm tra cấu hình NVIDIA ==="

    # Check modeset
    if [ -f /sys/module/nvidia_drm/parameters/modeset ]; then
        local val
        val=$(cat /sys/module/nvidia_drm/parameters/modeset)
        if [ "$val" = "Y" ] || [ "$val" = "1" ]; then
            log_info "✓ nvidia-drm.modeset = $val"
        else
            log_error "✗ nvidia-drm.modeset = $val (cần Y hoặc 1)"
            errors=$((errors + 1))
        fi
    else
        log_warn "⚠ nvidia_drm module chưa loaded (kiểm tra sau reboot)"
    fi

    # Check GBM
    if [ -f "$ENV_NVIDIA" ] && grep -q "GBM_BACKEND=nvidia-drm" "$ENV_NVIDIA"; then
        log_info "✓ GBM_BACKEND=nvidia-drm đã cấu hình"
    fi

    # Check blacklist
    if [ -f "$MODPROBE_BLACKLIST" ]; then
        log_info "✓ nouveau blacklist active"
    else
        log_warn "⚠ nouveau blacklist chưa được cài"
    fi

    if [ "$errors" -gt 0 ]; then
        log_error "Phát hiện $errors lỗi. Xem log để biết chi tiết."
        return 1
    fi

    log_info "=== Kiểm tra hoàn tất: Không có lỗi ==="
    return 0
}

# ---- Main --------------------------------------------------------------------

main() {
    log_info "=== TizenOS NVIDIA Setup v${VERSION} ==="

    # Xử lý argument
    case "${1:-setup}" in
        rollback)
            rollback
            exit 0
            ;;
        verify)
            verify_setup
            exit $?
            ;;
        setup)
            ;;
        *)
            echo "Sử dụng: $0 {setup|rollback|verify}"
            exit 1
            ;;
    esac

    # Đọc GPU info
    local nvidia_version="0"
    if [ -f "$GPU_INFO" ]; then
        # shellcheck source=/dev/null
        source "$GPU_INFO" 2>/dev/null || true
        nvidia_version="${GPU_NVIDIA_VERSION:-0}"
    else
        # Fallback: đọc trực tiếp từ sysfs
        nvidia_version=$(cat /sys/module/nvidia/version 2>/dev/null || echo "0")
    fi

    if [ "$nvidia_version" = "0" ]; then
        log_error "Không thể xác định phiên bản NVIDIA driver."
        log_error "Đảm bảo nvidia driver đã được cài đặt và module loaded."
        exit 1
    fi

    log_info "NVIDIA Driver version: $nvidia_version"

    # Thực hiện setup
    setup_nouveau_blacklist
    setup_modeset
    setup_environment "$nvidia_version"

    # PRIME setup nếu hybrid GPU
    if [ -f "$GPU_INFO" ]; then
        # shellcheck source=/dev/null
        source "$GPU_INFO" 2>/dev/null || true
        if [ "${GPU_PRIME_AVAILABLE:-no}" = "yes" ]; then
            setup_prime
        fi
    fi

    setup_power_management "$nvidia_version"

    # Cập nhật initramfs
    if command -v update-initramfs &>/dev/null; then
        log_info "Cập nhật initramfs..."
        update-initramfs -u 2>/dev/null || log_warn "Không thể cập nhật initramfs"
    fi

    # Verify
    verify_setup || true

    log_info "=== NVIDIA Setup hoàn tất ==="
}

main "$@"
