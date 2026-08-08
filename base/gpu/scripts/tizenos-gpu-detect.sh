#!/bin/bash
set -euo pipefail

# =============================================================================
# TizenOS GPU Detection Script
# =============================================================================
# Phát hiện GPU toàn diện: vendor, driver, khả năng Wayland/XWayland,
# PRIME hybrid, GBM, DRM modeset, explicit sync.
# Kết quả ghi ra /run/tizenos/gpu-info.env để các thành phần khác đọc.
# =============================================================================

readonly VERSION="1.0.0"
readonly OUT_DIR="/run/tizenos"
readonly OUT_FILE="${OUT_DIR}/gpu-info.env"
readonly LOG_TAG="tizenos-gpu-detect"

# ---- Logging ----------------------------------------------------------------
log_info()  { echo "[GPU-DETECT] $*" | systemd-cat -t "$LOG_TAG" -p info  2>/dev/null || echo "[INFO] $*"; }
log_warn()  { echo "[GPU-DETECT] $*" | systemd-cat -t "$LOG_TAG" -p warning 2>/dev/null || echo "[WARN] $*"; }
log_error() { echo "[GPU-DETECT] $*" | systemd-cat -t "$LOG_TAG" -p err  2>/dev/null || echo "[ERROR] $*"; }

# ---- Khởi tạo giá trị mặc định -----------------------------------------------
GPU_VENDOR="unknown"
GPU_DRIVER="unknown"
GPU_DRIVER_VERSION=""
GPU_HAS_GBM="no"
GPU_PRIME_AVAILABLE="no"
GPU_WAYLAND_CAPABLE="no"
GPU_XWAYLAND_HW_ACCEL="no"
GPU_EXPLICIT_SYNC="no"
GPU_COUNT=0
GPU_PRIMARY=""
GPU_RENDER_NODE=""
GPU_IGPU_DRIVER=""       # Driver iGPU trong hybrid setup
GPU_DGPU_DRIVER=""       # Driver dGPU trong hybrid setup
GPU_NVIDIA_VERSION="0"
GPU_MODESET_ACTIVE="no"
GPU_IS_VIRTUAL="no"

# ---- Tiện ích ----------------------------------------------------------------

# So sánh version: trả về 0 nếu $1 >= $2
version_ge() {
    local v1="${1%%.*}" v2="${2%%.*}"
    [ "$v1" -gt "$v2" ] 2>/dev/null && return 0
    [ "$v1" -lt "$v2" ] 2>/dev/null && return 1
    # Major bằng nhau → so sánh minor
    local m1="${1#*.}" m2="${2#*.}"
    m1="${m1%%.*}"; m2="${m2%%.*}"
    [ "${m1:-0}" -ge "${m2:-0}" ] 2>/dev/null
}

# Lấy driver đang active cho PCI device
get_pci_driver() {
    local pci_path="$1"
    local driver_link="/sys/bus/pci/devices/$pci_path/driver"
    if [ -L "$driver_link" ]; then
        basename "$(readlink -f "$driver_link")"
    else
        echo "none"
    fi
}

# ---- Phát hiện GPU qua sysfs -------------------------------------------------

# Đếm và liệt kê GPU qua DRM subsystem (chính xác hơn lspci)
enumerate_gpus() {
    local count=0
    local gpu_list=""

    # Duyệt DRM cards
    for card_dir in /sys/class/drm/card[0-9]*; do
        [ -d "$card_dir/device" ] || continue

        # Chỉ đếm card chính (không đếm renderD*)
        local card_name
        card_name=$(basename "$card_dir")
        [[ "$card_name" =~ ^card[0-9]+$ ]] || continue

        count=$((count + 1))

        # Lấy PCI vendor:device ID
        local vendor_id device_id
        vendor_id=$(cat "$card_dir/device/vendor" 2>/dev/null || echo "0x0000")
        device_id=$(cat "$card_dir/device/device" 2>/dev/null || echo "0x0000")

        gpu_list="${gpu_list}${card_name}:${vendor_id}:${device_id} "
        log_info "Phát hiện GPU: $card_name (vendor=$vendor_id, device=$device_id)"
    done

    GPU_COUNT=$count
    GPU_PRIMARY=$(echo "$gpu_list" | awk '{print $1}' | cut -d: -f1)
    log_info "Tổng số GPU: $GPU_COUNT, Primary: ${GPU_PRIMARY:-none}"
}

# ---- Phát hiện Vendor và Driver -----------------------------------------------

detect_vendor_and_driver() {
    # Phương pháp 1: Qua sysfs DRM card vendor ID (ưu tiên)
    local primary_vendor=""
    if [ -n "$GPU_PRIMARY" ] && [ -f "/sys/class/drm/${GPU_PRIMARY}/device/vendor" ]; then
        primary_vendor=$(cat "/sys/class/drm/${GPU_PRIMARY}/device/vendor" 2>/dev/null)
    fi

    case "$primary_vendor" in
        0x10de)  GPU_VENDOR="nvidia" ;;
        0x1002)  GPU_VENDOR="amd" ;;
        0x8086)  GPU_VENDOR="intel" ;;
        0x1af4)  GPU_VENDOR="virtual"; GPU_IS_VIRTUAL="yes" ;;  # virtio
        0x15ad)  GPU_VENDOR="virtual"; GPU_IS_VIRTUAL="yes" ;;  # VMware
        0x1b36)  GPU_VENDOR="virtual"; GPU_IS_VIRTUAL="yes" ;;  # QXL (QEMU)
        0x1234)  GPU_VENDOR="virtual"; GPU_IS_VIRTUAL="yes" ;;  # QEMU stdvga
        *)
            # Phương pháp 2: Fallback sang lspci
            if command -v lspci &>/dev/null; then
                local lspci_out
                lspci_out=$(lspci -nn 2>/dev/null | grep -iE 'vga|3d|display' || true)
                if echo "$lspci_out" | grep -qi "nvidia"; then
                    GPU_VENDOR="nvidia"
                elif echo "$lspci_out" | grep -qi "amd\|ati\|radeon"; then
                    GPU_VENDOR="amd"
                elif echo "$lspci_out" | grep -qi "intel"; then
                    GPU_VENDOR="intel"
                elif echo "$lspci_out" | grep -qiE "virtio|vmware|qxl|bochs"; then
                    GPU_VENDOR="virtual"
                    GPU_IS_VIRTUAL="yes"
                fi
            fi
            ;;
    esac

    # Phát hiện driver đang loaded
    if [ "$GPU_VENDOR" = "nvidia" ]; then
        if [ -d /sys/module/nvidia ]; then
            GPU_DRIVER="nvidia"
            GPU_NVIDIA_VERSION=$(cat /sys/module/nvidia/version 2>/dev/null || echo "0")
            GPU_DRIVER_VERSION="$GPU_NVIDIA_VERSION"
        elif [ -d /sys/module/nouveau ]; then
            GPU_DRIVER="nouveau"
            GPU_DRIVER_VERSION=$(modinfo nouveau 2>/dev/null | grep "^version:" | awk '{print $2}' || echo "kernel")
        else
            GPU_DRIVER="none"
            log_warn "NVIDIA GPU phát hiện nhưng không có driver nào được load!"
        fi
    elif [ "$GPU_VENDOR" = "amd" ]; then
        if [ -d /sys/module/amdgpu ]; then
            GPU_DRIVER="amdgpu"
        elif [ -d /sys/module/radeon ]; then
            GPU_DRIVER="radeon"
            log_warn "Đang dùng driver radeon cũ. Khuyến nghị chuyển sang amdgpu."
        fi
    elif [ "$GPU_VENDOR" = "intel" ]; then
        if [ -d /sys/module/i915 ]; then
            GPU_DRIVER="i915"
        elif [ -d /sys/module/xe ]; then
            GPU_DRIVER="xe"  # Intel Xe driver mới cho Arc GPU
        fi
    elif [ "$GPU_VENDOR" = "virtual" ]; then
        if [ -d /sys/module/virtio_gpu ]; then
            GPU_DRIVER="virtio-gpu"
        elif [ -d /sys/module/vmwgfx ]; then
            GPU_DRIVER="vmwgfx"
        elif [ -d /sys/module/qxl ]; then
            GPU_DRIVER="qxl"
        else
            GPU_DRIVER="modesetting"
        fi
    fi

    log_info "Vendor: $GPU_VENDOR, Driver: $GPU_DRIVER, Version: ${GPU_DRIVER_VERSION:-n/a}"
}

# ---- Phát hiện PRIME Hybrid GPU -----------------------------------------------

detect_prime() {
    if [ "$GPU_COUNT" -lt 2 ]; then
        return
    fi

    local has_igpu=false has_dgpu=false

    # Duyệt tất cả GPU cards
    for card_dir in /sys/class/drm/card[0-9]*/device; do
        [ -d "$card_dir" ] || continue
        local vendor
        vendor=$(cat "$card_dir/vendor" 2>/dev/null || echo "0x0000")

        case "$vendor" in
            0x8086|0x1002)
                # Intel hoặc AMD → có thể là iGPU
                has_igpu=true
                local drv
                drv=$(get_pci_driver "$(basename "$(dirname "$card_dir")" | sed 's/card//')" 2>/dev/null || echo "unknown")
                GPU_IGPU_DRIVER="$drv"
                ;;
            0x10de)
                # NVIDIA → dGPU
                has_dgpu=true
                GPU_DGPU_DRIVER=$([ -d /sys/module/nvidia ] && echo "nvidia" || echo "nouveau")
                ;;
        esac
    done

    if $has_igpu && $has_dgpu; then
        GPU_PRIME_AVAILABLE="yes"
        log_info "Phát hiện PRIME Hybrid GPU: iGPU ($GPU_IGPU_DRIVER) + dGPU ($GPU_DGPU_DRIVER)"
    fi
}

# ---- Kiểm tra khả năng Wayland/GBM -------------------------------------------

check_wayland_capabilities() {
    # AMD và Intel: luôn hỗ trợ Wayland qua Mesa GBM
    if [ "$GPU_VENDOR" = "amd" ] || [ "$GPU_VENDOR" = "intel" ]; then
        GPU_HAS_GBM="yes"
        GPU_WAYLAND_CAPABLE="yes"
        GPU_XWAYLAND_HW_ACCEL="yes"
        log_info "GPU $GPU_VENDOR/$GPU_DRIVER: Wayland native (Mesa GBM)"
        return
    fi

    # Virtual GPU: Wayland qua software rendering
    if [ "$GPU_IS_VIRTUAL" = "yes" ]; then
        GPU_HAS_GBM="yes"  # Mesa virtio-gpu hỗ trợ GBM
        GPU_WAYLAND_CAPABLE="yes"
        GPU_XWAYLAND_HW_ACCEL="no"  # Không có HW accel thực sự
        log_info "Virtual GPU: Wayland khả dụng (software/virtio rendering)"
        return
    fi

    # NVIDIA: phụ thuộc version
    if [ "$GPU_VENDOR" = "nvidia" ]; then
        if [ "$GPU_DRIVER" = "nouveau" ]; then
            # nouveau hỗ trợ GBM qua Mesa, nhưng hiệu năng hạn chế
            GPU_HAS_GBM="yes"
            GPU_WAYLAND_CAPABLE="yes"
            GPU_XWAYLAND_HW_ACCEL="yes"
            log_info "nouveau: Wayland khả dụng (Mesa GBM, hiệu năng hạn chế)"
            return
        fi

        if [ "$GPU_DRIVER" = "nvidia" ]; then
            # Kiểm tra nvidia-drm modeset
            if [ -f /sys/module/nvidia_drm/parameters/modeset ]; then
                local modeset_val
                modeset_val=$(cat /sys/module/nvidia_drm/parameters/modeset 2>/dev/null || echo "N")
                if [ "$modeset_val" = "Y" ] || [ "$modeset_val" = "1" ]; then
                    GPU_MODESET_ACTIVE="yes"
                else
                    GPU_MODESET_ACTIVE="no"
                    log_warn "nvidia-drm.modeset KHÔNG active! Wayland sẽ không hoạt động."
                    log_warn "Cần thêm 'nvidia-drm.modeset=1' vào modprobe.d hoặc kernel cmdline."
                fi
            fi

            # GBM support: NVIDIA >= 495
            if version_ge "$GPU_NVIDIA_VERSION" "495"; then
                GPU_HAS_GBM="yes"
                log_info "NVIDIA $GPU_NVIDIA_VERSION: GBM được hỗ trợ"

                if [ "$GPU_MODESET_ACTIVE" = "yes" ]; then
                    GPU_WAYLAND_CAPABLE="yes"
                    GPU_XWAYLAND_HW_ACCEL="yes"

                    # Explicit sync: NVIDIA >= 545 + kernel >= 6.1
                    if version_ge "$GPU_NVIDIA_VERSION" "545"; then
                        GPU_EXPLICIT_SYNC="yes"
                        log_info "NVIDIA $GPU_NVIDIA_VERSION: Explicit sync khả dụng"
                    else
                        log_warn "NVIDIA $GPU_NVIDIA_VERSION: Thiếu explicit sync (cần >= 545). XWayland có thể bị flickering."
                    fi
                else
                    GPU_WAYLAND_CAPABLE="no"
                    log_warn "NVIDIA GBM sẵn sàng nhưng modeset chưa active → Wayland bị chặn"
                fi
            else
                # NVIDIA < 495: KHÔNG hỗ trợ GBM → KHÔNG dùng được wlroots
                GPU_HAS_GBM="no"
                GPU_WAYLAND_CAPABLE="no"
                GPU_XWAYLAND_HW_ACCEL="no"
                log_warn "NVIDIA $GPU_NVIDIA_VERSION: Quá cũ cho Wayland (cần >= 495). Fallback X11."
            fi
        fi

        if [ "$GPU_DRIVER" = "none" ]; then
            GPU_WAYLAND_CAPABLE="no"
            log_error "Không có driver NVIDIA nào loaded. Fallback X11 với VESA."
        fi
    fi
}

# ---- Tìm render node ---------------------------------------------------------

find_render_node() {
    # Ưu tiên render node của GPU primary
    if [ -n "$GPU_PRIMARY" ]; then
        local expected_render="/dev/dri/renderD$((128 + ${GPU_PRIMARY#card}))"
        if [ -c "$expected_render" ]; then
            GPU_RENDER_NODE="$expected_render"
            log_info "Render node (primary): $GPU_RENDER_NODE"
            return
        fi
    fi

    # Fallback: lấy render node đầu tiên
    for render in /dev/dri/renderD*; do
        if [ -c "$render" ]; then
            GPU_RENDER_NODE="$render"
            log_info "Render node (fallback): $GPU_RENDER_NODE"
            return
        fi
    done

    log_warn "Không tìm thấy render node nào trong /dev/dri/"
}

# ---- Ghi kết quả ra file -----------------------------------------------------

write_results() {
    mkdir -p "$OUT_DIR"

    cat > "$OUT_FILE" <<EOF
# TizenOS GPU Detection Results
# Được tạo bởi tizenos-gpu-detect v${VERSION}
# Thời gian: $(date -Iseconds)

GPU_VENDOR=${GPU_VENDOR}
GPU_DRIVER=${GPU_DRIVER}
GPU_DRIVER_VERSION=${GPU_DRIVER_VERSION}
GPU_HAS_GBM=${GPU_HAS_GBM}
GPU_PRIME_AVAILABLE=${GPU_PRIME_AVAILABLE}
GPU_WAYLAND_CAPABLE=${GPU_WAYLAND_CAPABLE}
GPU_XWAYLAND_HW_ACCEL=${GPU_XWAYLAND_HW_ACCEL}
GPU_EXPLICIT_SYNC=${GPU_EXPLICIT_SYNC}
GPU_COUNT=${GPU_COUNT}
GPU_PRIMARY=${GPU_PRIMARY}
GPU_RENDER_NODE=${GPU_RENDER_NODE}
GPU_IS_VIRTUAL=${GPU_IS_VIRTUAL}
GPU_MODESET_ACTIVE=${GPU_MODESET_ACTIVE}
GPU_NVIDIA_VERSION=${GPU_NVIDIA_VERSION}
GPU_IGPU_DRIVER=${GPU_IGPU_DRIVER}
GPU_DGPU_DRIVER=${GPU_DGPU_DRIVER}
EOF

    chmod 644 "$OUT_FILE"
    log_info "Kết quả đã ghi ra $OUT_FILE"
}

# ---- Main --------------------------------------------------------------------

main() {
    log_info "=== TizenOS GPU Detection v${VERSION} bắt đầu ==="

    enumerate_gpus

    if [ "$GPU_COUNT" -eq 0 ]; then
        log_error "KHÔNG phát hiện GPU nào! Hệ thống có thể không có card đồ họa."
        GPU_WAYLAND_CAPABLE="no"
        write_results
        exit 0
    fi

    detect_vendor_and_driver
    detect_prime
    check_wayland_capabilities
    find_render_node
    write_results

    log_info "=== GPU Detection hoàn tất: vendor=$GPU_VENDOR driver=$GPU_DRIVER wayland=$GPU_WAYLAND_CAPABLE ==="
}

main "$@"
