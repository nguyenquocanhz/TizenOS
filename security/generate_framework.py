import os

def write_file(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content.strip() + '\n')

base_dir = r"d:\TizenOS\security"

# ==============================================================================
# 1. security/smack-utils
# ==============================================================================
smack_dir = os.path.join(base_dir, "smack-utils")

write_file(os.path.join(smack_dir, "smack-util.h"), """
#ifndef SMACK_UTIL_H
#define SMACK_UTIL_H

// Khai báo các hàm tiện ích cho việc quản lý Smack MAC
// Smack là hệ thống Mandatory Access Control trong nhân Linux

int load_smack_rules(const char *rule_file);
int apply_smack_label(int fd, const char *label);

#endif // SMACK_UTIL_H
""")

write_file(os.path.join(smack_dir, "smack-manager.c"), """
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
        if (line[0] == '#' || line[0] == '\\n') continue;
        
        // Ghi luật vào kernel
        if (write(fd, line, strlen(line)) < 0) {
            perror("Lỗi khi ghi luật Smack vào kernel");
        }
    }

    close(fd);
    fclose(f);
    printf("Đã nạp thành công các luật Smack.\\n");
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
""")

write_file(os.path.join(smack_dir, "smack-labels.c"), """
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
    printf("Đã gán nhãn Smack: %s thành công.\\n", label);
    return 0;
}
""")

write_file(os.path.join(smack_dir, "config", "smack-default.rules"), """
# Cấu hình luật Smack mặc định cho TizenOS
# Định dạng: Chủ_thể Đối_tượng Quyền (rwxat)
# r: read, w: write, x: execute, a: append, t: transmute

# Cho phép System truy cập mọi thứ của App
System App rwxat
# App chỉ được quyền đọc tài nguyên System
App System r
# User có thể tương tác với App
User App rw
# App có thể đọc/ghi thư mục của User
App User rw
""")

write_file(os.path.join(smack_dir, "CMakeLists.txt"), """
cmake_minimum_required(VERSION 3.10)
project(smack-utils)

add_library(smack-util SHARED smack-labels.c)
add_executable(smack-manager smack-manager.c)
target_link_libraries(smack-manager smack-util)

install(TARGETS smack-manager DESTINATION bin)
install(TARGETS smack-util DESTINATION lib)
install(FILES config/smack-default.rules DESTINATION /etc/smack/)
""")

write_file(os.path.join(smack_dir, "tizen-smack.service"), """
[Unit]
Description=TizenOS Smack Initialization Service
Before=sysinit.target
DefaultDependencies=no

[Service]
Type=oneshot
ExecStart=/usr/bin/smack-manager /etc/smack/smack-default.rules
RemainAfterExit=yes

[Install]
WantedBy=sysinit.target
""")

write_file(os.path.join(smack_dir, "debian", "control"), """
Source: smack-utils
Maintainer: TizenOS Security Team <security@tizen.org>
Section: admin
Priority: optional
Build-Depends: debhelper-compat (= 13), cmake

Package: smack-utils
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}
Description: TizenOS Smack Utilities
 Công cụ quản lý luật Smack MAC cho TizenOS.
""")

write_file(os.path.join(smack_dir, "debian", "rules"), """#!/usr/bin/make -f
%:
	dh $@ --buildsystem=cmake
""")

write_file(os.path.join(smack_dir, "debian", "changelog"), """
smack-utils (1.0-1) unstable; urgency=medium

  * Initial release for TizenOS Security Framework.

 -- TizenOS Security Team <security@tizen.org>  Thu, 01 Jan 2026 00:00:00 +0000
""")


# ==============================================================================
# 2. security/cynara
# ==============================================================================
cynara_dir = os.path.join(base_dir, "cynara")

write_file(os.path.join(cynara_dir, "include", "cynara", "cynara-client.h"), """
#ifndef CYNARA_CLIENT_H
#define CYNARA_CLIENT_H

// API Client cho Cynara: Kiểm tra quyền (privilege) của ứng dụng hoặc user
int cynara_check_privilege(const char *client_label, const char *user, const char *privilege);

#endif
""")

write_file(os.path.join(cynara_dir, "include", "cynara", "cynara-admin.h"), """
#ifndef CYNARA_ADMIN_H
#define CYNARA_ADMIN_H

// API Admin cho Cynara: Thiết lập chính sách (ALLOW/DENY)
int cynara_set_policy(const char *client_label, const char *user, const char *privilege, int result);

#endif
""")

write_file(os.path.join(cynara_dir, "cynara-client.c"), """
#include <stdio.h>
#include "cynara/cynara-client.h"

// Hàm truy vấn D-Bus đến cynara-daemon để kiểm tra quyền
// Trả về 1 nếu ALLOW, 0 nếu DENY
int cynara_check_privilege(const char *client_label, const char *user, const char *privilege) {
    // Giả lập truy vấn D-Bus tới service quản lý chính sách
    printf("[Client] Kiểm tra quyền: client=%s, user=%s, priv=%s\\n", client_label, user, privilege);
    return 1; // Mặc định cho phép trong giả lập
}
""")

write_file(os.path.join(cynara_dir, "cynara-admin.c"), """
#include <stdio.h>
#include "cynara/cynara-admin.h"

// Công cụ CLI để set chính sách vào cơ sở dữ liệu (SQLite) thông qua cynara-daemon
int cynara_set_policy(const char *client_label, const char *user, const char *privilege, int result) {
    printf("[Admin] Thiết lập chính sách: client=%s, user=%s, priv=%s -> %s\\n",
           client_label, user, privilege, result ? "ALLOW" : "DENY");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Sử dụng: %s <client> <user> <privilege> <1/0>\\n", argv[0]);
        return 1;
    }
    // Thiết lập chính sách (Ví dụ: 1 là ALLOW, 0 là DENY)
    cynara_set_policy(argv[1], argv[2], argv[3], argv[4][0] == '1');
    return 0;
}
""")

write_file(os.path.join(cynara_dir, "cynara-daemon.c"), """
#include <stdio.h>
#include <unistd.h>

// Daemon lắng nghe truy vấn D-Bus và tra cứu trong cơ sở dữ liệu SQLite
// Quyết định xem có cấp quyền truy cập theo policy hay không
int main() {
    printf("[Daemon] Cynara policy engine đang chạy...\\n");
    while (1) {
        // Vòng lặp lắng nghe D-Bus (giả lập)
        sleep(10);
    }
    return 0;
}
""")

write_file(os.path.join(cynara_dir, "policies", "default-policy.conf"), """
# Cấu hình chính sách mặc định Cynara
# Format: Client_Label User Privilege Result

System * http://tizen.org/privilege/internet ALLOW
App user http://tizen.org/privilege/camera DENY
""")

write_file(os.path.join(cynara_dir, "schema.sql"), """
-- Lược đồ cơ sở dữ liệu SQLite cho Cynara Policy DB
CREATE TABLE policies (
    client_label TEXT,
    user TEXT,
    privilege TEXT,
    result INTEGER, -- 1 for ALLOW, 0 for DENY
    PRIMARY KEY (client_label, user, privilege)
);

-- Dữ liệu mẫu khởi tạo
INSERT INTO policies VALUES ('System', '*', 'http://tizen.org/privilege/internet', 1);
""")

write_file(os.path.join(cynara_dir, "dbus", "org.tizen.cynara.conf"), """
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <policy user="root">
    <allow own="org.tizen.cynara"/>
    <allow send_destination="org.tizen.cynara"/>
  </policy>
  <policy context="default">
    <allow send_destination="org.tizen.cynara"/>
  </policy>
</busconfig>
""")

write_file(os.path.join(cynara_dir, "CMakeLists.txt"), """
cmake_minimum_required(VERSION 3.10)
project(cynara)

include_directories(include)

add_library(cynara-client SHARED cynara-client.c)
add_executable(cynara-admin cynara-admin.c)
target_link_libraries(cynara-admin cynara-client)

add_executable(cynara-daemon cynara-daemon.c)

install(TARGETS cynara-daemon cynara-admin DESTINATION bin)
install(TARGETS cynara-client DESTINATION lib)
install(DIRECTORY include/cynara DESTINATION include)
""")

write_file(os.path.join(cynara_dir, "tizen-cynara.service"), """
[Unit]
Description=Cynara Policy Engine Daemon
After=dbus.service

[Service]
ExecStart=/usr/bin/cynara-daemon
Restart=always
User=root

[Install]
WantedBy=multi-user.target
""")

write_file(os.path.join(cynara_dir, "debian", "control"), """
Source: cynara
Maintainer: TizenOS Security Team <security@tizen.org>
Section: admin
Priority: optional
Build-Depends: debhelper-compat (= 13), cmake, libsqlite3-dev, libdbus-1-dev

Package: cynara
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}
Description: TizenOS Cynara Policy Engine
 Hệ thống quản lý quyền truy cập và phân giải chính sách (policy engine).
""")

write_file(os.path.join(cynara_dir, "debian", "rules"), """#!/usr/bin/make -f
%:
	dh $@ --buildsystem=cmake
""")

write_file(os.path.join(cynara_dir, "debian", "changelog"), """
cynara (1.0-1) unstable; urgency=medium

  * Initial release for TizenOS Security Framework.

 -- TizenOS Security Team <security@tizen.org>  Thu, 01 Jan 2026 00:00:00 +0000
""")


# ==============================================================================
# 3. security/security-manager
# ==============================================================================
secman_dir = os.path.join(base_dir, "security-manager")

write_file(os.path.join(secman_dir, "security-manager.h"), """
#ifndef SECURITY_MANAGER_H
#define SECURITY_MANAGER_H

// Quản lý ứng dụng: Gắn kết App ID, thiết lập label Smack, đăng ký privileges
int install_app_security(const char *app_id);
int set_process_credentials(int pid, const char *app_id);

#endif
""")

write_file(os.path.join(secman_dir, "security-manager.c"), """
#include <stdio.h>
#include <stdlib.h>
#include "security-manager.h"

// Cài đặt thông tin bảo mật cho ứng dụng mới (Manifest)
// Tạo label Smack tương ứng và đăng ký đặc quyền vào Cynara DB
int install_app_security(const char *app_id) {
    printf("[SecurityManager] Đang thiết lập bảo mật cho ứng dụng: %s\\n", app_id);
    // TODO: Gọi các hàm từ smack-utils và cynara-admin để tạo policies
    return 0;
}

// Cấu hình credential (Smack label) cho process khi ứng dụng khởi chạy
int set_process_credentials(int pid, const char *app_id) {
    printf("[SecurityManager] Đang áp dụng nhãn Smack cho PID %d (App: %s)\\n", pid, app_id);
    return 0;
}

int main(int argc, char **argv) {
    printf("TizenOS Security Manager khởi động.\\n");
    // Chờ yêu cầu cài đặt manifest từ Package Manager (giả lập)
    if(argc > 1) {
        install_app_security(argv[1]);
    }
    return 0;
}
""")

write_file(os.path.join(secman_dir, "CMakeLists.txt"), """
cmake_minimum_required(VERSION 3.10)
project(security-manager)

add_executable(security-manager security-manager.c)

install(TARGETS security-manager DESTINATION bin)
install(FILES security-manager.h DESTINATION include)
""")

write_file(os.path.join(secman_dir, "tizen-security-manager.service"), """
[Unit]
Description=TizenOS Security Manager
After=tizen-cynara.service tizen-smack.service

[Service]
ExecStart=/usr/bin/security-manager
Restart=always

[Install]
WantedBy=multi-user.target
""")

write_file(os.path.join(secman_dir, "debian", "control"), """
Source: security-manager
Maintainer: TizenOS Security Team <security@tizen.org>
Section: admin
Priority: optional
Build-Depends: debhelper-compat (= 13), cmake, smack-utils-dev, cynara-dev

Package: security-manager
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}
Description: TizenOS Security Manager
 Trình quản lý bảo mật ứng dụng: thiết lập Smack, đăng ký Privilege thông qua Cynara.
""")

write_file(os.path.join(secman_dir, "debian", "rules"), """#!/usr/bin/make -f
%:
	dh $@ --buildsystem=cmake
""")

write_file(os.path.join(secman_dir, "debian", "changelog"), """
security-manager (1.0-1) unstable; urgency=medium

  * Initial release for TizenOS Security Framework.

 -- TizenOS Security Team <security@tizen.org>  Thu, 01 Jan 2026 00:00:00 +0000
""")

print("Successfully generated all files for TizenOS Security Framework.")
