/*
 * Tizen App Manager - Main Entry Point (GTK4 Application)
 * =============================================================================
 * Quản lý ứng dụng TizenOS: Xem ứng dụng đã cài đặt (.tpk & .deb), gỡ cài đặt,
 * quét & cài đặt tệp .deb 1-Click trong hệ thống, tìm kiếm ứng dụng thời gian thực.
 * =============================================================================
 */

#include "app-manager.h"
#include "tizen/glib-compat.h"   /* G_APPLICATION_DEFAULT_FLAGS trên GLib < 2.74 */
#include "tizen/theme.h"         /* Theme tối dùng chung cho mọi app TizenOS */
#include <stdio.h>
#include <stdlib.h>

static GtkWidget *search_entry = NULL;
static GtkWidget *filter_combo = NULL;

static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    (void)user_data;
    const char *query = gtk_editable_get_text(GTK_EDITABLE(entry));
    const char *type = "all";

    if (filter_combo) {
        guint active = gtk_drop_down_get_selected(GTK_DROP_DOWN(filter_combo));
        if (active == 1) type = "deb";
        else if (active == 2) type = "tpk";
    }

    refresh_installed_apps_list(query, type);
}

static void on_filter_changed(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    GtkDropDown *combo = GTK_DROP_DOWN(gobject);
    const char *query = search_entry ? gtk_editable_get_text(GTK_EDITABLE(search_entry)) : NULL;
    guint active = gtk_drop_down_get_selected(combo);

    const char *type = "all";
    if (active == 1) type = "deb";
    else if (active == 2) type = "tpk";

    refresh_installed_apps_list(query, type);
}

static void on_refresh_all_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    refresh_installed_apps_list(NULL, "all");
    refresh_deb_files_list();
}

/* Nhãn tab cho GtkNotebook: biểu tượng icon theme + chữ.
 * Tách riêng vì tizen_button_new() dựng ra một GtkButton — bỏ nguyên một nút
 * vào làm nhãn tab sẽ lồng nút trong nút, hỏng cả giao diện lẫn thao tác bàn phím. */
static GtkWidget *tab_label_new(const char *icon_name, const char *label)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(box), gtk_image_new_from_icon_name(icon_name));
    gtk_box_append(GTK_BOX(box), gtk_label_new(label));
    return box;
}

static void load_custom_css(void) {
    /* Theme dùng chung — xem tizen/theme.h và ghi chú ở tizen-store/src/main.c. */
    tizen_theme_apply();
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    load_custom_css();

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Tizen App Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 980, 680);

    // Root VBox
    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header Controls Box
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(header_box, 16);
    gtk_widget_set_margin_end(header_box, 16);
    gtk_widget_set_margin_top(header_box, 16);
    gtk_widget_set_margin_bottom(header_box, 8);

    // Title — biểu tượng lấy từ icon theme, không dùng emoji (xem tizen/theme.h)
    GtkWidget *app_icon = gtk_image_new_from_icon_name("tizen-app-manager");
    gtk_image_set_pixel_size(GTK_IMAGE(app_icon), 24);
    gtk_box_append(GTK_BOX(header_box), app_icon);

    GtkWidget *lbl_title = gtk_label_new("Tizen App Manager");
    gtk_widget_add_css_class(lbl_title, "title-large");
    gtk_box_append(GTK_BOX(header_box), lbl_title);

    // Search Entry
    search_entry = gtk_search_entry_new();
    tizen_search_entry_set_placeholder(search_entry,
        "Tìm kiếm ứng dụng...");
    gtk_widget_set_hexpand(search_entry, TRUE);
    g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_append(GTK_BOX(header_box), search_entry);

    // Type Filter DropDown (Modern GTK4 API)
    const char *filter_options[] = { "Tất cả loại gói", "Debian (.deb)", "Tizen Native (.tpk)", NULL };
    filter_combo = gtk_drop_down_new_from_strings(filter_options);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(filter_combo), 0);
    g_signal_connect(filter_combo, "notify::selected", G_CALLBACK(on_filter_changed), NULL);
    gtk_box_append(GTK_BOX(header_box), filter_combo);

    // Refresh Button
    GtkWidget *btn_refresh = tizen_button_new("view-refresh-symbolic", "Làm mới");
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(on_refresh_all_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), btn_refresh);

    gtk_box_append(GTK_BOX(root_box), header_box);

    // Notebook (Tabs)
    GtkWidget *notebook = gtk_notebook_new();
    gtk_widget_set_vexpand(notebook, TRUE);

    /* Nhãn tab: biểu tượng icon theme + chữ, thay cho emoji.
     * gtk_notebook_append_page() nhận một widget bất kỳ làm nhãn tab, nên dùng
     * chung một hộp icon+label như các nút khác. */
    GtkWidget *view_installed = create_installed_apps_view();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), view_installed,
                             tab_label_new("package-x-generic-symbolic",
                                           "Ứng dụng đã cài đặt"));

    GtkWidget *view_deb = create_deb_files_view();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), view_deb,
                             tab_label_new("document-open-symbolic",
                                           "Tệp .deb trong hệ thống"));

    gtk_box_append(GTK_BOX(root_box), notebook);

    gtk_window_set_child(GTK_WINDOW(window), root_box);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.tizenos.appmanager", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
