# Quản lý GPU trong TizenOS

Module GPU management xử lý tự động phát hiện GPU, giải quyết xung đột NVIDIA driver,
và quyết định sử dụng Wayland hay X11 fallback.

## Kiến trúc

```
Boot Sequence:
  systemd-modules-load
    → tizenos-gpu-detect.service    (phát hiện GPU, ghi /run/tizenos/gpu-info.env)
    → tizenos-nvidia-setup.service  (nếu NVIDIA: blacklist nouveau, config modeset)
    → display-manager
      → tizenos-display-fallback.sh (quyết định Wayland vs X11)
        → tizenos-compositor        (Wayland) hoặc Xorg (X11 fallback)
```

## Các xung đột được xử lý

| Xung đột | Giải pháp |
|-----------|-----------|
| nouveau vs nvidia | Blacklist nouveau qua modprobe.d |
| nvidia-drm modeset | Force `modeset=1` qua modprobe.d + GRUB |
| GBM (driver < 495) | Detect version → X11 fallback nếu cần |
| XWayland HW accel | Set `__GLX_VENDOR_LIBRARY_NAME=nvidia` |
| PRIME hybrid GPU | Auto-detect, `tizenos-prime-run` cho offload |
| Power management | nvidia-powerd + runtime PM |
| Suspend/resume | PreserveVideoMemoryAllocations + nvidia_uvm reload |

## Sử dụng

### Force chế độ hiển thị
```bash
# Bắt buộc X11 (bỏ qua Wayland)
TIZENOS_FORCE_X11=1 tizenos-display-fallback.sh

# Bắt buộc Wayland (bỏ qua kiểm tra)
TIZENOS_FORCE_WAYLAND=1 tizenos-display-fallback.sh
```

### PRIME offloading (laptop hybrid GPU)
```bash
# Chạy app trên NVIDIA dGPU
tizenos-prime-run glxgears
tizenos-prime-run steam
```

### NVIDIA rollback
```bash
# Khôi phục về nouveau (gỡ mọi cấu hình NVIDIA)
sudo tizenos-nvidia-setup.sh rollback
sudo reboot
```

### Kiểm tra trạng thái
```bash
# Xem kết quả GPU detection
cat /run/tizenos/gpu-info.env

# Xem log GPU
journalctl -u tizenos-gpu-detect
journalctl -u tizenos-nvidia-setup

# Verify NVIDIA setup
sudo tizenos-nvidia-setup.sh verify
```

## Troubleshooting

### Màn hình đen sau boot
1. Khởi động vào TTY: `Ctrl+Alt+F2`
2. Kiểm tra log: `journalctl -u tizenos-gpu-detect -b`
3. Thử force X11: `TIZENOS_FORCE_X11=1 startx`
4. Nếu NVIDIA: kiểm tra modeset: `cat /sys/module/nvidia_drm/parameters/modeset`

### XWayland apps chạy chậm (software rendering)
1. Kiểm tra: `glxinfo | grep "OpenGL renderer"` (phải hiện GPU, không phải llvmpipe)
2. Xác nhận: `echo $__GLX_VENDOR_LIBRARY_NAME` (phải là "nvidia")
3. Kiểm tra modeset: `cat /sys/module/nvidia_drm/parameters/modeset` (phải là Y)

### eGPU không nhận
1. Kiểm tra udev: `udevadm monitor` rồi cắm eGPU
2. Restart detection: `sudo systemctl restart tizenos-gpu-detect`
3. Kiểm tra DRM: `ls /sys/class/drm/card*`

## Cấu trúc files

```
base/gpu/
├── scripts/
│   ├── tizenos-gpu-detect.sh       → /usr/lib/tizenos/gpu/
│   ├── tizenos-nvidia-setup.sh     → /usr/lib/tizenos/gpu/
│   └── tizenos-display-fallback.sh → /usr/lib/tizenos/gpu/
├── config/
│   ├── modprobe.d/                 → /etc/modprobe.d/
│   ├── udev/rules.d/              → /usr/lib/udev/rules.d/
│   └── xorg.conf.d/               → /etc/X11/xorg.conf.d/
├── systemd/                        → /usr/lib/systemd/system/
├── CMakeLists.txt
└── README.md                       → /usr/share/doc/tizenos/gpu-management.md
```
