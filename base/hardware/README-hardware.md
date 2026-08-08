# Ngăn xếp Trình điều khiển Phần cứng và Lưu trữ TizenOS (Hardware & Storage Stack)

Đây là kho chứa cấu hình, quy tắc udev, systemd services, và module cho việc khởi tạo phần cứng cốt lõi trên TizenOS. Hệ thống được tối ưu đặc biệt cho Debian 12.

## 1. Mạng Không dây (Wi-Fi & Bluetooth)
*   **Wi-Fi**: Tích hợp các bộ firmware chuẩn: `firmware-iwlwifi` (Intel), `firmware-realtek` (RTL series), `firmware-atheros` (Qualcomm). 
    *   Quy tắc udev (`95-tizenos-hardware.rules`) sẽ tự động bật tính năng tiết kiệm pin (Power Save) qua `iw`.
*   **Bluetooth**: Sử dụng ngăn xếp BlueZ, `bluez-tools`. Hệ thống sẽ tự kích hoạt (Auto-enable) adapter thông qua udev (dùng `hciconfig up`). Codec âm thanh cao cấp được hỗ trợ qua `pipewire-audio-codecs`.

## 2. Card Mạng LAN
*   Tương thích với các module `e1000e`, `igc` (Intel 2.5G), và `r8169` (Realtek).
*   Sử dụng `ethtool` trong udev rule để tự động kích hoạt tính năng Wake-on-LAN (`wol g`), cho phép đánh thức hệ thống từ xa.

## 3. Lưu trữ NVMe (Chuẩn 1.4/2.0)
*   TizenOS sử dụng `nvme-cli` để quản lý thiết bị.
*   **APST (Autonomous Power State Transition)**: Udev rule gửi lệnh qua `nvme set-feature` để tối ưu hóa quản lý nhiệt lượng và điện năng của NVMe.
*   **NVMe Multipathing**: Được bật thông qua cấu hình `modprobe.d/tizenos-storage-raid.conf` (tham số `multipath=Y`).

## 4. Công nghệ RAID (Intel VMD & mdadm)
*   **Intel VMD (Volume Management Device)**: Module `vmd` được cấu hình để tự tải thông qua `alias` trong modprobe, nhằm phát hiện trực tiếp các ổ SSD NVMe nằm sau bộ điều khiển RAID của Intel.
*   **Mdadm**: Hỗ trợ phần mềm thiết lập RAID (0, 1, 5, 10). TizenOS bao gồm `mdadm` và `lvm2` để cung cấp sự linh hoạt trong quản lý Volume.

## Hướng dẫn Build (Đóng gói Debian)
Chạy lệnh sau tại thư mục gốc:
```bash
dpkg-buildpackage -us -uc -b
```
Nó sẽ xuất ra 4 package nhị phân:
- `tizenos-hardware-wifi`
- `tizenos-hardware-bluetooth`
- `tizenos-hardware-lan`
- `tizenos-hardware-storage`
