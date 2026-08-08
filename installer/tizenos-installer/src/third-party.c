#include "third-party.h"
#include <stdio.h>
#include <stdlib.h>

bool third_party_install_nvidia(void) {
    // Cài đặt trình điều khiển đồ họa NVIDIA độc quyền
    printf("Đang cài đặt trình điều khiển NVIDIA độc quyền (nvidia-driver)...\n");
    return system("apt-get install -y nvidia-driver") == 0;
}

bool third_party_install_codecs(void) {
    // Cài đặt các codec đa phương tiện (FFmpeg/GStreamer bị giới hạn)
    printf("Đang cài đặt Multimedia codecs (gstreamer, ffmpeg)...\n");
    return system("apt-get install -y ubuntu-restricted-extras") == 0;
}

bool third_party_install_flatpak(void) {
    // Cài đặt Flatpak và thêm repo Flathub
    printf("Đang cấu hình Flatpak và tích hợp Web apps...\n");
    system("apt-get install -y flatpak");
    system("flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo");
    return true;
}
