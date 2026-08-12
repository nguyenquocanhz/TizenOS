#ifndef __TIZEN_APP_MANAGER_H__
#define __TIZEN_APP_MANAGER_H__

#include <gtk/gtk.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Cấu trúc thông tin ứng dụng đã cài đặt
typedef struct {
    char name[256];
    char app_id[128];
    char exec[512];
    char icon[256];
    char type[64];        // "tpk", "deb", "wgt"
    char version[64];
    char smack_label[64]; // "User", "System"
} InstalledAppInfo;

// Cấu trúc thông tin file .deb tìm thấy trong hệ thống
typedef struct {
    char file_path[512];
    char file_name[256];
    char package_name[128];
    char version[64];
    char arch[32];
    char size_str[32];
    bool is_installed;
} DebFileInfo;

// Module 1: Quản lý Ứng dụng đã cài đặt
GtkWidget* create_installed_apps_view(void);
void refresh_installed_apps_list(const char *search_query, const char *filter_type);

// Module 2: Quản lý File .deb trong hệ thống
GtkWidget* create_deb_files_view(void);
void refresh_deb_files_list(void);

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_APP_MANAGER_H__
