#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>

// Daemon chính của hệ thống quản lý gói TizenOS
// Lắng nghe các yêu cầu cài đặt/gỡ bỏ qua D-Bus

extern void pkgmgr_dbus_init();
extern void pkgmgr_db_init();

int main(int argc, char *argv[]) {
    printf("Khởi động Tizen Package Manager Daemon...\n");
    
    // Khởi tạo cơ sở dữ liệu SQLite
    pkgmgr_db_init();
    
    // Khởi tạo D-Bus interface
    pkgmgr_dbus_init();
    
    // Chạy main loop của glib
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    
    return 0;
}
