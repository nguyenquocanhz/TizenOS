#ifndef __TIZEN_APP_STORE_H__
#define __TIZEN_APP_STORE_H__

#include <gtk/gtk.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Cấu trúc ứng dụng trong Kho App Store
typedef struct {
    char id[128];
    char name[256];
    char category[64];      // "internet", "graphics", "office", "dev", "games", "system"
    char developer[128];
    char icon[256];
    char description[512];
    char package_name[128]; // package apt hoặc tpk
    char size_str[32];
    char rating[16];        // "4.8 ★"
    bool is_installed;
} AppStoreItem;

// Functions
GtkWidget* create_app_store_catalog_view(const char *category_filter, const char *search_query);
void refresh_app_store_catalog(const char *category_filter, const char *search_query);

/**
 * Đánh dấu bộ nhớ đệm trạng thái cài đặt là đã cũ, buộc lần refresh kế tiếp
 * hỏi lại dpkg.
 *
 * Trạng thái được lưu cache vì trước đây refresh_app_store_catalog() gọi
 * dpkg-query cho từng app trong danh mục, mà refresh chạy theo signal
 * "search-changed" — tức 60+ tiến trình con cho MỖI ký tự gõ vào ô tìm kiếm.
 * Gọi hàm này sau khi cài hoặc gỡ gói, hoặc khi người dùng bấm "Cập nhật kho".
 */
void app_store_invalidate_states(void);

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_APP_STORE_H__
