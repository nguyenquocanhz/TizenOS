cmake_minimum_required(VERSION 3.16)
project(tizenos-hardware-stack)

# Đường dẫn cài đặt
set(BINDIR "usr/bin")
set(SYSCONFDIR "etc")
set(UDEVDIR "lib/udev/rules.d")
set(SYSTEMDDIR "lib/systemd/system")
set(SCRIPTDIR "usr/lib/tizenos/scripts")

# Cài đặt scripts
install(PROGRAMS scripts/tizenos-hardware-detect.sh DESTINATION ${SCRIPTDIR})

# Cài đặt file cấu hình modprobe
install(FILES config/modprobe.d/tizenos-storage-raid.conf DESTINATION ${SYSCONFDIR}/modprobe.d)

# Cài đặt udev rules
install(FILES config/udev/rules.d/95-tizenos-hardware.rules DESTINATION ${UDEVDIR})

# Cài đặt systemd service
install(FILES systemd/tizenos-hardware-detect.service DESTINATION ${SYSTEMDDIR})
