#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "smack-util.h"

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
    // Nạp file cấu hình luật mặc định
    if (argc > 1) {
        load_smack_rules(argv[1]);
    } else {
        load_smack_rules("/etc/smack/smack-default.rules");
    }
    return 0;
}
