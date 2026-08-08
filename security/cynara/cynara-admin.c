#include <stdio.h>
#include "cynara/cynara-admin.h"

// Công cụ CLI để set chính sách vào cơ sở dữ liệu (SQLite) thông qua cynara-daemon
int cynara_set_policy(const char *client_label, const char *user, const char *privilege, int result) {
    printf("[Admin] Thiết lập chính sách: client=%s, user=%s, priv=%s -> %s\n",
           client_label, user, privilege, result ? "ALLOW" : "DENY");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Sử dụng: %s <client> <user> <privilege> <1/0>\n", argv[0]);
        return 1;
    }
    // Thiết lập chính sách (Ví dụ: 1 là ALLOW, 0 là DENY)
    cynara_set_policy(argv[1], argv[2], argv[3], argv[4][0] == '1');
    return 0;
}
