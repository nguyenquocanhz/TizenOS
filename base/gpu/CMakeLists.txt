# ==============================================================================
# TizenOS GPU Management Module - CMake Build
# ==============================================================================
# Install scripts, configs, và systemd units cho GPU detection/management.
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

# Thư mục install targets
set(TIZENOS_GPU_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib/tizenos/gpu"
    CACHE PATH "Thư mục cài đặt scripts GPU")
set(TIZENOS_SYSTEMD_UNITDIR "/usr/lib/systemd/system"
    CACHE PATH "Thư mục systemd unit files")
set(TIZENOS_MODPROBE_DIR "/etc/modprobe.d"
    CACHE PATH "Thư mục cấu hình modprobe")
set(TIZENOS_UDEV_RULESDIR "/usr/lib/udev/rules.d"
    CACHE PATH "Thư mục udev rules")
set(TIZENOS_XORG_CONFDIR "/etc/X11/xorg.conf.d"
    CACHE PATH "Thư mục xorg config")

# ---- Scripts -----------------------------------------------------------------
# Cài đặt các scripts detection/setup/fallback
install(
    PROGRAMS
        scripts/tizenos-gpu-detect.sh
        scripts/tizenos-nvidia-setup.sh
        scripts/tizenos-display-fallback.sh
    DESTINATION ${TIZENOS_GPU_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

# ---- systemd services -------------------------------------------------------
install(
    FILES
        systemd/tizenos-gpu-detect.service
        systemd/tizenos-nvidia-setup.service
        systemd/tizenos-gpu-monitor.service
    DESTINATION ${TIZENOS_SYSTEMD_UNITDIR}
)

# ---- modprobe config ---------------------------------------------------------
install(
    FILES
        config/modprobe.d/tizenos-nouveau-blacklist.conf
    DESTINATION ${TIZENOS_MODPROBE_DIR}
)

# ---- udev rules --------------------------------------------------------------
install(
    FILES
        config/udev/rules.d/90-tizenos-gpu.rules
    DESTINATION ${TIZENOS_UDEV_RULESDIR}
)

# ---- Xorg fallback configs ---------------------------------------------------
install(
    FILES
        config/xorg.conf.d/10-tizenos-nvidia.conf
        config/xorg.conf.d/10-tizenos-modesetting.conf
    DESTINATION ${TIZENOS_XORG_CONFDIR}
)

# ---- Documentation -----------------------------------------------------------
install(
    FILES README.md
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/tizenos
    RENAME gpu-management.md
)

# ---- Post-install: enable systemd services ------------------------------------
# Chỉ tạo symlink nếu chạy trong chroot/target environment
install(CODE "
    if(EXISTS \"/etc/systemd/system\")
        message(STATUS \"Enabling tizenos-gpu-detect.service...\")
        execute_process(
            COMMAND systemctl enable tizenos-gpu-detect.service
            RESULT_VARIABLE result
            ERROR_QUIET
        )
        if(NOT result EQUAL 0)
            message(WARNING \"Không thể enable tizenos-gpu-detect.service (chạy ngoài target?)\")
        endif()
    endif()
")
