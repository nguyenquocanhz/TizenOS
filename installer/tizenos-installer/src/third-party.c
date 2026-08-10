#include "third-party.h"
#include <stdio.h>
#include <stdlib.h>

bool third_party_install_nvidia(void) {
    printf("[THIRD-PARTY] Cài đặt trình điều khiển NVIDIA độc quyền (nvidia-driver)...\n");
    return system("apt-get install -y nvidia-driver 2>/dev/null || true") == 0;
}

bool third_party_install_codecs(void) {
    printf("[THIRD-PARTY] Cài đặt Multimedia codecs (gstreamer, ffmpeg)...\n");
    return system("apt-get install -y ffmpeg gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav 2>/dev/null || true") == 0;
}

bool third_party_install_flatpak(void) {
    printf("[THIRD-PARTY] Cấu hình Flatpak và Flathub repo...\n");
    system("apt-get install -y flatpak 2>/dev/null || true");
    system("flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo 2>/dev/null || true");
    return true;
}
