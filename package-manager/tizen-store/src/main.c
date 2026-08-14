/*
 * Tizen App Store - Main Entry Point (GTK4 Application)
 * =============================================================================
 * Kho ứng dụng TizenOS Software Center: Giao diện Glassmorphism hiện đại,
 * tìm kiếm ứng dụng thời gian thực, 1-Click Cài đặt/Gỡ bỏ ứng dụng Debian & Tizen Native.
 * =============================================================================
 */

#include "app-store.h"
#include "tizen/glib-compat.h"   /* G_APPLICATION_DEFAULT_FLAGS trên GLib < 2.74 */
#include "tizen/theme.h"         /* Theme tối dùng chung cho mọi app TizenOS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *search_entry = NULL;
static char current_category[32] = "all";

static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    (void)user_data;
    const char *query = gtk_editable_get_text(GTK_EDITABLE(entry));
    refresh_app_store_catalog(current_category, query);
}

static void on_category_selected(GtkListBox *list, GtkListBoxRow *row, gpointer user_data) {
    (void)list; (void)user_data;
    /* "row-selected" bắn kèm row = NULL khi danh sách bị bỏ chọn (ví dụ lúc lọc
     * lại hoặc xoá hàng). gtk_list_box_row_get_index(NULL) là critical + trả -1. */
    if (!row) return;

    int idx = gtk_list_box_row_get_index(row);

    switch (idx) {
        case 0: strncpy(current_category, "all", sizeof(current_category)); break;
        case 1: strncpy(current_category, "internet", sizeof(current_category)); break;
        case 2: strncpy(current_category, "graphics", sizeof(current_category)); break;
        case 3: strncpy(current_category, "office", sizeof(current_category)); break;
        case 4: strncpy(current_category, "dev", sizeof(current_category)); break;
        case 5: strncpy(current_category, "games", sizeof(current_category)); break;
        case 6: strncpy(current_category, "system", sizeof(current_category)); break;
        default: strncpy(current_category, "all", sizeof(current_category)); break;
    }

    const char *query = search_entry ? gtk_editable_get_text(GTK_EDITABLE(search_entry)) : NULL;
    refresh_app_store_catalog(current_category, query);
}

static void on_refresh_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    /* Đây là nơi DUY NHẤT nên quét lại trạng thái cài đặt của toàn bộ danh mục.
     * Bản cũ quét trong mỗi lần vẽ lại, kể cả khi người dùng chỉ gõ tìm kiếm. */
    app_store_invalidate_states();
    const char *query = search_entry ? gtk_editable_get_text(GTK_EDITABLE(search_entry)) : NULL;
    refresh_app_store_catalog(current_category, query);
}

static void on_ota_update_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    GError *error = NULL;
    if (!g_spawn_command_line_async("tizenos-script-runner /usr/local/bin/tizenos-app-updater --check", &error)) {
        g_warning("Không thể khởi chạy Tizen App Updater: %s", error ? error->message : "Lỗi không xác định");
        if (error) g_error_free(error);
    }
}

static GtkWidget* create_category_sidebar(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(vbox, 230, -1);

    GtkWidget *lbl_title = gtk_label_new("Danh mục Kho App");
    gtk_widget_add_css_class(lbl_title, "title-large");
    gtk_widget_set_margin_top(lbl_title, 16);
    gtk_widget_set_margin_bottom(lbl_title, 16);
    gtk_box_append(GTK_BOX(vbox), lbl_title);

    GtkWidget *list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_SINGLE);

    /* Biểu tượng từ icon theme, không dùng emoji — xem tizen_button_new()
     * trong tizen/theme.h. THỨ TỰ PHẢI KHỚP với on_category_selected(). */
    static const struct { const char *icon; const char *label; } categories[] = {
        { "starred-symbolic",                   "Nổi bật (All Featured)"    },
        { "network-workgroup-symbolic",         "Trình duyệt & Mạng"        },
        { "applications-graphics-symbolic",     "Đồ họa & Đa phương tiện"   },
        { "x-office-document-symbolic",         "Văn phòng & Công việc"     },
        { "applications-engineering-symbolic",  "Công cụ Lập trình"         },
        { "applications-games-symbolic",        "Trò chơi & Giải trí"       },
        { "security-high-symbolic",             "Hệ thống & Bảo mật"        },
    };

    for (size_t i = 0; i < G_N_ELEMENTS(categories); i++) {
        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_start(row_box, 16);
        gtk_widget_set_margin_end(row_box, 16);
        gtk_widget_set_margin_top(row_box, 10);
        gtk_widget_set_margin_bottom(row_box, 10);

        gtk_box_append(GTK_BOX(row_box),
                       gtk_image_new_from_icon_name(categories[i].icon));

        GtkWidget *lbl = gtk_label_new(categories[i].label);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        gtk_box_append(GTK_BOX(row_box), lbl);

        gtk_list_box_append(GTK_LIST_BOX(list), row_box);
    }

    /* "row-selected" chứ không phải "row-activated": row-activated chỉ bắn khi
     * nhấp ĐÚP hoặc nhấn Enter, nên nhấp một lần vào danh mục không đổi gì cả. */
    g_signal_connect(list, "row-selected", G_CALLBACK(on_category_selected), NULL);
    gtk_box_append(GTK_BOX(vbox), list);

    return vbox;
}

static void load_custom_css(void) {
    /* Theme dùng chung cho mọi app TizenOS — xem tizen/theme.h.
     * Bản cũ dán CSS riêng ở đây (và hai bản sao gần-giống ở App Manager với
     * Album) nhưng KHÔNG bật gtk-application-prefer-dark-theme, nên Adwaita sáng
     * vẫn là theme nền: popover, tooltip, scrollbar, headerbar và .dim-label giữ
     * nguyên màu sáng giữa một app tối. tizen_theme_apply() bật cờ đó trước rồi
     * mới phủ token lên. */
    tizen_theme_apply();

    /* Papirus-Dark cho icon ứng dụng nhiều màu — nhưng CHỈ khi máy thực sự có.
     * Ép vô điều kiện như bản cũ làm mọi icon thành ô "image-missing" trên bản
     * cài không có gói papirus-icon-theme. */
    tizen_theme_set_icon_theme("Papirus-Dark");
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    load_custom_css();

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Tizen App Store");
    gtk_window_set_default_size(GTK_WINDOW(window), 1080, 720);

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header Controls Bar
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(header_box, 16);
    gtk_widget_set_margin_end(header_box, 16);
    gtk_widget_set_margin_top(header_box, 14);
    gtk_widget_set_margin_bottom(header_box, 10);

    // Title Logo
    GtkWidget *lbl_title = gtk_label_new("Tizen App Store");
    gtk_widget_add_css_class(lbl_title, "title-large");
    gtk_box_append(GTK_BOX(header_box), lbl_title);

    // Search Bar
    search_entry = gtk_search_entry_new();
    tizen_search_entry_set_placeholder(search_entry,
        "Tìm kiếm ứng dụng trong Kho TizenOS...");
    gtk_widget_set_hexpand(search_entry, TRUE);
    g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_append(GTK_BOX(header_box), search_entry);

    // Refresh Button
    GtkWidget *btn_refresh = tizen_button_new("view-refresh-symbolic", "Cập nhật kho");
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(on_refresh_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), btn_refresh);

    // OTA Git Update Button
    GtkWidget *btn_ota = tizen_button_new("software-update-available-symbolic", "Cập nhật OTA Git");
    gtk_widget_add_css_class(btn_ota, "suggested-action");
    g_signal_connect(btn_ota, "clicked", G_CALLBACK(on_ota_update_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), btn_ota);

    gtk_box_append(GTK_BOX(root_box), header_box);

    // Main Content (Sidebar + Catalog Grid)
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget *sidebar = create_category_sidebar();
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    GtkWidget *catalog_view = create_app_store_catalog_view("all", NULL);

    /* Lề mép cho vùng danh mục: không có nó thì lưới thẻ dính sát mép phải cửa
     * sổ và sát đường kẻ ngăn với sidebar. */
    gtk_widget_add_css_class(catalog_view, "gutter");

    gtk_box_append(GTK_BOX(main_box), sidebar);
    gtk_box_append(GTK_BOX(main_box), sep);
    gtk_box_append(GTK_BOX(main_box), catalog_view);

    gtk_box_append(GTK_BOX(root_box), main_box);

    gtk_window_set_child(GTK_WINDOW(window), root_box);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.tizenos.store", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
