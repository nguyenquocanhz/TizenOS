#include <stdio.h>
#include <glib.h>

extern void tpk_install(const char *path);
extern void wgt_install(const char *path);
extern void deb_bridge_install(const char *package_name);

// Daemon D-Bus chờ tín hiệu cài đặt từ UI/CLI
int main(int argc, char **argv) {
    printf("Khởi động Installer Daemon trên D-Bus...\n");
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    
    // Giả lập cài đặt gói khi có request D-Bus
    tpk_install("/opt/app.tpk");
    deb_bridge_install("nano");
    
    g_main_loop_run(loop);
    return 0;
}
