#!/bin/bash
set -euo pipefail

# =============================================================================
# TizenOS Display Fallback Script
# =============================================================================
# Quyết định thông minh: Wayland compositor vs X11 session
# Dựa trên kết quả GPU detection (/run/tizenos/gpu-info.env)
# Hỗ trợ: force override, retry logic, user notification.
# =============================================================================

readonly VERSION="1.0.0"
readonly LOG_TAG="tizenos-display-fallback"
readonly GPU_INFO="/run/tizenos/gpu-info.env"
readonly COMPOSITOR_BIN="/usr/bin/tizenos-compositor"
readonly FALLBACK_STATE="/run/tizenos/display-fallback-state"
readonly MAX_COMPOSITOR_RETRIES=2

# ---- Logging ----------------------------------------------------------------
log_info()  { echo "[DISPLAY] $*" | systemd-cat -t "$LOG_TAG" -p info    2>/dev/null || echo "[INFO] $*"; }
log_warn()  { echo "[DISPLAY] $*" | systemd-cat -t "$LOG_TAG" -p warning 2>/dev/null || echo "[WARN] $*"; }
log_error() { echo "[DISPLAY] $*" | systemd-cat -t "$LOG_TAG" -p err     2>/dev/null || echo "[ERROR] $*"; }

# ---- User Notification -------------------------------------------------------

# Gửi thông báo desktop cho người dùng
notify_user() {
    local urgency="${1:-normal}"
    local title="$2"
    local message="$3"

    # Thử notify-send (cần display server đã chạy)
    if command -v notify-send &>/dev/null; then
        # Chạy với timeout, không block nếu D-Bus chưa sẵn sàng
        timeout 5 notify-send --urgency="$urgency" "$title" "$message" 2>/dev/null || true
    fi

    # Luôn log
    log_info "Notification [$urgency]: $title - $message"
}

# Ghi trạng thái fallback (để desktop session đọc và hiển thị warning)
write_fallback_state() {
    local mode="$1"
    local reason="$2"
    mkdir -p "$(dirname "$FALLBACK_STATE")"
    cat > "$FALLBACK_STATE" <<EOF
DISPLAY_MODE=${mode}
FALLBACK_REASON=${reason}
FALLBACK_TIME=$(date -Iseconds)
EOF
}

# ---- Wayland Launcher --------------------------------------------------------

# Khởi chạy Wayland compositor với retry logic
launch_wayland() {
    local renderer="${1:-auto}"
    local extra_env="${2:-}"
    local attempt=0

    log_info "Khởi chạy Wayland compositor (renderer=$renderer)..."

    while [ $attempt -lt $MAX_COMPOSITOR_RETRIES ]; do
        attempt=$((attempt + 1))
        log_info "Wayland attempt $attempt/$MAX_COMPOSITOR_RETRIES"

        local env_vars=()

        # Renderer override
        case "$renderer" in
            vulkan)  env_vars+=(WLR_RENDERER=vulkan) ;;
            gles2)   env_vars+=(WLR_RENDERER=gles2) ;;
            pixman)  env_vars+=(WLR_RENDERER=pixman) ;;
            auto)    ;; # Để wlroots tự chọn
        esac

        # Extra env vars (NVIDIA GBM, etc.)
        if [ -n "$extra_env" ]; then
            # shellcheck disable=SC2206
            env_vars+=($extra_env)
        fi

        # Export env vars và launch
        for var in "${env_vars[@]}"; do
            export "${var?}"
        done

        write_fallback_state "wayland" "normal"

        # Chạy compositor — nếu crash (exit code != 0), retry
        if "$COMPOSITOR_BIN"; then
            # Compositor thoát bình thường (user logout)
            return 0
        fi

        local exit_code=$?
        log_warn "Compositor thoát với mã lỗi $exit_code (attempt $attempt)"

        # Chờ một chút trước retry (tránh crash loop)
        sleep 2
    done

    log_error "Wayland compositor thất bại sau $MAX_COMPOSITOR_RETRIES lần thử"
    return 1
}

# ---- X11 Launcher ------------------------------------------------------------

# Khởi chạy X11 session fallback
launch_x11() {
    local xorg_config="${1:-}"
    local reason="${2:-unknown}"

    log_warn "=== FALLBACK: Chuyển sang X11 session ==="
    log_warn "Lý do: $reason"

    write_fallback_state "x11" "$reason"

    # Tạo X11 session script
    local xinitrc="/tmp/tizenos-xinitrc"
    cat > "$xinitrc" <<'XINITRC'
#!/bin/bash
# TizenOS X11 Fallback Session

# Đặt cursor
xsetroot -cursor_name left_ptr 2>/dev/null || true

# Thông báo cho user về fallback mode
if command -v notify-send &>/dev/null; then
    notify-send --urgency=critical \
        "TizenOS: Chế độ X11 Fallback" \
        "Wayland không khả dụng. Đang chạy X11.\nKiểm tra driver GPU: journalctl -u tizenos-gpu-detect" &
fi

# Chạy window manager fallback (nếu compositor không hỗ trợ X11)
if command -v tizenos-session-x11 &>/dev/null; then
    exec tizenos-session-x11
elif command -v openbox &>/dev/null; then
    exec openbox-session
elif command -v xfwm4 &>/dev/null; then
    exec xfce4-session
else
    # Absolute fallback: xterm
    exec xterm -e "echo 'TizenOS X11 Fallback - Cài thêm window manager'; bash"
fi
XINITRC
    chmod +x "$xinitrc"

    # Chuẩn bị Xorg arguments
    local xorg_args=()
    if [ -n "$xorg_config" ] && [ -f "$xorg_config" ]; then
        xorg_args+=(-config "$xorg_config")
    fi

    # Ghi thông báo tới console
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════╗"
    echo "║  TizenOS: Đang khởi động X11 Fallback Mode                     ║"
    echo "║  Lý do: $reason"
    echo "║  Để debug: journalctl -u tizenos-gpu-detect                     ║"
    echo "╚══════════════════════════════════════════════════════════════════╝"
    echo ""

    # Launch Xorg
    exec startx "$xinitrc" -- "${xorg_args[@]}" 2>/dev/null || {
        log_error "X11 cũng thất bại! Dropping to TTY console."
        echo "CRITICAL: Cả Wayland và X11 đều thất bại."
        echo "Đăng nhập qua TTY console (Ctrl+Alt+F2)"
        exit 1
    }
}

# ---- Decision Engine ---------------------------------------------------------

make_display_decision() {
    # Nguồn dữ liệu GPU
    if [ ! -f "$GPU_INFO" ]; then
        log_error "Không tìm thấy $GPU_INFO — chạy tizenos-gpu-detect trước!"
        launch_x11 "" "GPU info not available"
        return
    fi

    # shellcheck source=/dev/null
    source "$GPU_INFO"

    log_info "=== Quyết định Display Mode ==="
    log_info "Vendor=$GPU_VENDOR Driver=$GPU_DRIVER Wayland=$GPU_WAYLAND_CAPABLE GBM=$GPU_HAS_GBM"

    # --- Force overrides (user/admin) ---
    if [ "${TIZENOS_FORCE_X11:-}" = "1" ]; then
        log_info "TIZENOS_FORCE_X11=1 → Bắt buộc X11"
        launch_x11 "" "User forced X11 mode"
        return
    fi

    if [ "${TIZENOS_FORCE_WAYLAND:-}" = "1" ]; then
        log_info "TIZENOS_FORCE_WAYLAND=1 → Bắt buộc Wayland"
        launch_wayland "auto" ""
        return
    fi

    # --- Decision tree ---

    # Case 1: GPU hỗ trợ Wayland đầy đủ
    if [ "${GPU_WAYLAND_CAPABLE:-no}" = "yes" ]; then

        # Sub-case 1a: NVIDIA proprietary với GBM
        if [ "$GPU_VENDOR" = "nvidia" ] && [ "$GPU_DRIVER" = "nvidia" ]; then
            log_info "NVIDIA proprietary + GBM → Wayland"
            local nvidia_env="GBM_BACKEND=nvidia-drm __GLX_VENDOR_LIBRARY_NAME=nvidia"

            # PRIME: dùng iGPU cho compositor (tiết kiệm pin)
            if [ "${GPU_PRIME_AVAILABLE:-no}" = "yes" ] && [ -n "${GPU_RENDER_NODE:-}" ]; then
                nvidia_env="$nvidia_env WLR_DRM_DEVICES=${GPU_RENDER_NODE}"
                log_info "PRIME: Compositor dùng iGPU, dGPU dùng cho offload"
            fi

            if ! launch_wayland "auto" "$nvidia_env"; then
                log_warn "NVIDIA Wayland thất bại → Fallback X11"
                launch_x11 "/etc/X11/xorg.conf.d/10-tizenos-nvidia.conf" "NVIDIA Wayland compositor crashed"
            fi
            return
        fi

        # Sub-case 1b: AMD/Intel (Mesa native)
        if [ "$GPU_VENDOR" = "amd" ] || [ "$GPU_VENDOR" = "intel" ]; then
            log_info "$GPU_VENDOR/$GPU_DRIVER → Wayland native (Mesa)"
            if ! launch_wayland "auto" ""; then
                launch_x11 "/etc/X11/xorg.conf.d/10-tizenos-modesetting.conf" "Mesa Wayland compositor crashed"
            fi
            return
        fi

        # Sub-case 1c: nouveau (Mesa)
        if [ "$GPU_VENDOR" = "nvidia" ] && [ "$GPU_DRIVER" = "nouveau" ]; then
            log_info "nouveau → Wayland (Mesa, hiệu năng hạn chế)"
            if ! launch_wayland "auto" ""; then
                launch_x11 "/etc/X11/xorg.conf.d/10-tizenos-modesetting.conf" "nouveau Wayland failed"
            fi
            return
        fi

        # Sub-case 1d: Mặc định Wayland
        log_info "GPU Wayland capable → launch Wayland"
        if ! launch_wayland "auto" ""; then
            launch_x11 "" "Wayland compositor crashed"
        fi
        return
    fi

    # Case 2: Virtual GPU (QEMU/VirtualBox/VMware)
    if [ "${GPU_IS_VIRTUAL:-no}" = "yes" ]; then
        log_info "Virtual GPU → Wayland (Pixman software rendering)"
        if ! launch_wayland "pixman" ""; then
            launch_x11 "" "Virtual GPU Wayland failed"
        fi
        return
    fi

    # Case 3: NVIDIA cũ (< 495, không GBM)
    if [ "$GPU_VENDOR" = "nvidia" ] && [ "$GPU_DRIVER" = "nvidia" ] && [ "${GPU_HAS_GBM:-no}" = "no" ]; then
        log_warn "NVIDIA < 495: KHÔNG hỗ trợ GBM → X11 bắt buộc"
        log_warn "Khuyến nghị: Nâng cấp NVIDIA driver lên >= 495 cho Wayland support"
        launch_x11 "/etc/X11/xorg.conf.d/10-tizenos-nvidia.conf" "NVIDIA driver too old for Wayland (need >= 495)"
        return
    fi

    # Case 4: NVIDIA không load driver
    if [ "$GPU_VENDOR" = "nvidia" ] && [ "$GPU_DRIVER" = "none" ]; then
        log_error "NVIDIA GPU không có driver! Thử modesetting fallback..."
        launch_x11 "/etc/X11/xorg.conf.d/10-tizenos-modesetting.conf" "No NVIDIA driver loaded"
        return
    fi

    # Case 5: GPU không xác định / không driver
    log_warn "GPU không hỗ trợ Wayland → X11 modesetting"
    launch_x11 "/etc/X11/xorg.conf.d/10-tizenos-modesetting.conf" "GPU not Wayland capable"
}

# ---- Main --------------------------------------------------------------------

main() {
    log_info "=== TizenOS Display Fallback v${VERSION} ==="

    # Kiểm tra compositor binary
    if [ ! -x "$COMPOSITOR_BIN" ]; then
        log_warn "Compositor $COMPOSITOR_BIN không tìm thấy hoặc không executable"
        log_warn "Sẽ fallback sang X11 nếu Wayland được chọn"
    fi

    make_display_decision
}

main "$@"
