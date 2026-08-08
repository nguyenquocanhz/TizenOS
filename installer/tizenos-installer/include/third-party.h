#ifndef THIRD_PARTY_H
#define THIRD_PARTY_H

#include <stdbool.h>

// Cài đặt trình điều khiển NVIDIA
bool third_party_install_nvidia(void);

// Cài đặt Multimedia codecs
bool third_party_install_codecs(void);

// Cài đặt Flatpak và Web apps
bool third_party_install_flatpak(void);

#endif
