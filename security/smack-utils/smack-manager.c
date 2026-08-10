#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "smack-util.h"

/* === Samsung Tizen 10.0 Best Practice: Conditional Smack Loading === */
/* Kiểm tra Smack LSM availability trước khi ghi rules */
static int smack_is_available(void) {
    /* Phương pháp 1: Kiểm tra smackfs mounted */
    if (access("/sys/fs/smackfs/load", W_OK) != 0) {
        fprintf(stderr, "[SMACK] WARNING: /sys/fs/smackfs/load not accessible.\n");
        fprintf(stderr, "[SMACK] Smack LSM not enabled in kernel. Skipping rule loading.\n");
        fprintf(stderr, "[SMACK] To enable: compile kernel with CONFIG_SECURITY_SMACK=y\n");
        fprintf(stderr, "[SMACK] and add lsm=smack,capability to kernel cmdline.\n");
        return 0;
    }
    /* Phương pháp 2: Kiểm tra LSM list */
    FILE *f = fopen("/sys/kernel/security/lsm", "r");
    if (f) {
        char buf[256];
        if (fgets(buf, sizeof(buf), f)) {
            fclose(f);
            if (strstr(buf, "smack") == NULL) {
                fprintf(stderr, "[SMACK] WARNING: Smack not in active LSM list: %s", buf);
                fprintf(stderr, "[SMACK] Skipping Smack rule loading (no-smack mode).\n");
                return 0;
            }
            return 1; /* Smack available */
        }
        fclose(f);
    }
    return 0;
}

// Ghi rules trực tiếp vào /sys/fs/smackfs/load để nạp luật Smack
#define SMACK_LOAD_PATH "/sys/fs/smackfs/load"

int load_smack_rules(const char *rule_file) {
    // Mở file chứa các rule mặc định
    FILE *f = fopen(rule_file, "r");
    if (!f) {
        perror("Không thể mở file luật Smack");
        return -1;
    }

    // Mở file hệ thống smackfs để nạp luật
    int fd = open(SMACK_LOAD_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Không thể mở /sys/fs/smackfs/load");
        fclose(f);
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Ghi luật vào kernel
        if (write(fd, line, strlen(line)) < 0) {
            perror("Lỗi khi ghi luật Smack vào kernel");
        }
    }

    close(fd);
    fclose(f);
    printf("Đã nạp thành công các luật Smack.\n");
    return 0;
}

int main(int argc, char **argv) {
    if (!smack_is_available()) {
        printf("[SMACK] Running in no-smack mode (Tizen 10.0 compatible).\n");
        return 0; /* Exit gracefully instead of crashing */
    }
    // Nạp file cấu hình luật mặc định
    if (argc > 1) {
        load_smack_rules(argv[1]);
    } else {
        load_smack_rules("/etc/smack/smack-default.rules");
    }
    return 0;
}
