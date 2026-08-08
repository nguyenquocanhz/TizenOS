#!/bin/bash
# ==============================================================================
# Script phát hiện phần cứng TizenOS
# Hỗ trợ: Wi-Fi (iwlwifi, realtek, atheros), Bluetooth (bluez), LAN, NVMe, RAID
# ==============================================================================

OUT_ENV="/run/tizenos/hardware-info.env"
mkdir -p $(dirname "$OUT_ENV")
echo "# TizenOS Hardware Detection Info" > "$OUT_ENV"

# 1. Phát hiện Wi-Fi
echo "[*] Đang kiểm tra thiết bị Wi-Fi..."
WIFI_MODULES=$(lspci -knn | grep -i net -A2 | grep "Kernel modules" | awk '{print $3}' | grep -E "iwlwifi|ath|rtw|wl|brcm")
if [ -n "$WIFI_MODULES" ]; then
    echo "WIFI_AVAILABLE=1" >> "$OUT_ENV"
    echo "WIFI_DRIVER=\"$WIFI_MODULES\"" >> "$OUT_ENV"
else
    echo "WIFI_AVAILABLE=0" >> "$OUT_ENV"
fi

# 2. Phát hiện Bluetooth
echo "[*] Đang kiểm tra thiết bị Bluetooth..."
if hciconfig -a >/dev/null 2>&1 || lsusb | grep -qi bluetooth; then
    echo "BLUETOOTH_AVAILABLE=1" >> "$OUT_ENV"
else
    echo "BLUETOOTH_AVAILABLE=0" >> "$OUT_ENV"
fi

# 3. Phát hiện mạng LAN
echo "[*] Đang kiểm tra thiết bị LAN..."
LAN_DRIVERS=$(lspci -knn | grep -i ethernet -A2 | grep "Kernel driver" | awk '{print $5}' | grep -E "e1000e|igc|r8169|tg3")
if [ -n "$LAN_DRIVERS" ]; then
    echo "LAN_AVAILABLE=1" >> "$OUT_ENV"
    echo "LAN_DRIVER=\"$LAN_DRIVERS\"" >> "$OUT_ENV"
fi

# 4. Phát hiện lưu trữ NVMe
echo "[*] Đang kiểm tra SSD NVMe..."
if ls /dev/nvme* >/dev/null 2>&1; then
    echo "NVME_AVAILABLE=1" >> "$OUT_ENV"
    NVME_COUNT=$(ls -1 /dev/nvme[0-9]n[0-9] 2>/dev/null | wc -l)
    echo "NVME_COUNT=$NVME_COUNT" >> "$OUT_ENV"
else
    echo "NVME_AVAILABLE=0" >> "$OUT_ENV"
fi

# 5. Phát hiện RAID (Software/Hardware RAID)
echo "[*] Đang kiểm tra cấu hình RAID..."
RAID_INFO=""
if grep -q "md" /proc/mdstat 2>/dev/null; then
    RAID_INFO="mdadm"
elif lspci | grep -qi "RAID"; then
    RAID_INFO="hardware"
fi

if [ -n "$RAID_INFO" ]; then
    echo "RAID_ACTIVE=1" >> "$OUT_ENV"
    echo "RAID_TYPE=\"$RAID_INFO\"" >> "$OUT_ENV"
else
    echo "RAID_ACTIVE=0" >> "$OUT_ENV"
fi

chmod 644 "$OUT_ENV"
echo "[+] Hoàn thành quét phần cứng. Kết quả được lưu tại $OUT_ENV."
exit 0
