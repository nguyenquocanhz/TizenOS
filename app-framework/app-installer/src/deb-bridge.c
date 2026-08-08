#include <stdio.h>
#include <stdlib.h>

// Cầu nối APT để ánh xạ .deb thành ứng dụng có biểu tượng Tizen
void deb_bridge_install(const char *package_name) {
    printf("Cài đặt gói debian '%s' thông qua APT và tạo shortcut cho Tizen launcher...\n", package_name);
    // Gọi APT
    // system("apt-get install -y package_name");
}
