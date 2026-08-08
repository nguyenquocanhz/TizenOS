#include <stdio.h>
#include <string.h>
#include <sys/xattr.h>
#include "smack-util.h"

// Hàm áp dụng nhãn Smack cho một file descriptor (cấp quyền truy cập MAC)
int apply_smack_label(int fd, const char *label) {
    // Đặt extended attribute security.SMACK64
    int ret = fsetxattr(fd, "security.SMACK64", label, strlen(label), 0);
    if (ret < 0) {
        perror("Lỗi khi gán nhãn Smack");
        return -1;
    }
    printf("Đã gán nhãn Smack: %s thành công.\n", label);
    return 0;
}
