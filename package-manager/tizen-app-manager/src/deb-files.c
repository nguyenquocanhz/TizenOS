/*
 * Tizen App Manager - System .deb File Manager Implementation
 * =============================================================================
 * Quét toàn bộ hệ thống (Downloads, Home, Cache, Tmp) để tìm các tệp gói .deb,
 * hiển thị thông tin chi tiết, hỗ trợ cài đặt 1-Click và dọn dẹp bộ nhớ đệm .deb.
 * =============================================================================
 */

#include "app-manager.h"
#include "tizen/pkg.h"

#include "tizen/theme.h"         /* tizen_button_new — nút dùng icon theme */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define TAG "DEB_FILES_MGR"
#define MAX_DEB_FILES 300

static DebFileInfo deb_files[MAX_DEB_FILES];
static int total_deb_files = 0;
static GtkWidget *list_box_deb = NULL;

/* SEC-027: bản cũ popen() với buffer 1024 byte, đọc một lần nên cắt cụt im
 * lặng, lại bỏ qua mã thoát của pclose(). Nay dùng argv qua libtizen-core. */
static char* run_argv(const char *const *argv) {
    char *out = NULL;
    tizen_exec_sync(argv, &out, NULL, NULL);
    return out ? out : g_strdup("");
}

/* Cài đặt tệp .deb */
static void on_install_deb_file_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *file_path = (const char*)user_data;
    if (!file_path) return;

    /* SEC-016: bản cũ dựng "tizenos-deb-installer '<path>' &" rồi đưa cho
     * g_spawn_command_line_async. Hàm đó không chạy shell nên dấu & thành
     * argv[2] thật, còn tên tệp chứa dấu nháy đơn (rất phổ biến, ví dụ
     * "John's driver.deb") làm g_shell_parse_argv thất bại và GError bị bỏ
     * qua — bấm Cài đặt mà không có gì xảy ra.
     *
     * file_path đến từ việc quét thư mục Downloads nên là dữ liệu do người
     * dùng (hoặc kẻ tấn công gửi file) đặt tên. */
    const char *argv[] = { "tizenos-deb-installer", "--", file_path, NULL };
    g_autoptr(GError) err = NULL;
    if (!g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_NONE, &err))
        g_warning("[%s] Không khởi chạy được trình cài đặt: %s",
                  TAG, err ? err->message : "lỗi không rõ");
}

static void on_delete_dialog_response(GtkDialog *dlg, int resp, gpointer data) {
    char *path = (char*)data;
    if (resp == GTK_RESPONSE_YES) {
        unlink(path);
        refresh_deb_files_list();
    }
    gtk_window_destroy(GTK_WINDOW(dlg));
    g_free(path);
}

/* Xóa tệp .deb */
static void on_delete_deb_file_clicked(GtkButton *btn, gpointer user_data) {
    const char *file_path = (const char*)user_data;
    if (!file_path) return;

    GtkWidget *parent_win = GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(btn)));

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(parent_win),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_YES_NO,
        "🗑️ Bạn có chắc chắn muốn xóa tệp .deb:\n'%s'?", file_path
    );

    g_signal_connect(dialog, "response", G_CALLBACK(on_delete_dialog_response), g_strdup(file_path));
    gtk_widget_set_visible(dialog, TRUE);
}

/* Quét thư mục tìm tệp .deb */
static void scan_directory_for_debs(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && total_deb_files < MAX_DEB_FILES) {
        if (strstr(entry->d_name, ".deb")) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
                DebFileInfo *info = &deb_files[total_deb_files];
                strncpy(info->file_path, full_path, sizeof(info->file_path) - 1);
                strncpy(info->file_name, entry->d_name, sizeof(info->file_name) - 1);

                // Read package name, version, arch via dpkg-deb
                /* SEC-012: full_path là tên tệp trong thư mục Downloads —
                 * một tệp tên `x';lệnh;'.deb` sẽ chèn lệnh qua popen. */
                const char *meta_argv[] = { "dpkg-deb", "-f", "--", full_path,
                                            "Package", "Version", "Architecture", NULL };
                char *meta = run_argv(meta_argv);

                char pkg[128] = "N/A", ver[64] = "N/A", arch[32] = "N/A";
                sscanf(meta, "Package: %127s\nVersion: %63s\nArchitecture: %31s", pkg, ver, arch);
                g_free(meta);

                strncpy(info->package_name, pkg, sizeof(info->package_name) - 1);
                strncpy(info->version, ver, sizeof(info->version) - 1);
                strncpy(info->arch, arch, sizeof(info->arch) - 1);

                // Format size
                double size_mb = (double)st.st_size / (1024.0 * 1024.0);
                snprintf(info->size_str, sizeof(info->size_str), "%.1f MB", size_mb);

                // Check if already installed
                /* Dùng hàm dùng chung: so sánh BẰNG với "installed" thay vì
                 * strstr. (Ở đây bản cũ đã đúng vì so cả cụm ba trường
                 * "install ok installed", nhưng thống nhất một chỗ vẫn tốt hơn
                 * và tránh lặp lại lỗi SEC-015 ở App Store.) */
                info->is_installed = tizen_pkg_is_installed(pkg);

                total_deb_files++;
            }
        }
    }
    closedir(dir);
}

void refresh_deb_files_list(void) {
    if (!list_box_deb) return;

    total_deb_files = 0;

    // Quét các thư mục phổ biến
    const char *home = g_get_home_dir();
    char downloads_dir[512];
    snprintf(downloads_dir, sizeof(downloads_dir), "%s/Downloads", home);

    scan_directory_for_debs(downloads_dir);
    scan_directory_for_debs(home);
    scan_directory_for_debs("/tmp");

    // Clear old widgets safely
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(list_box_deb)) != NULL) {
        gtk_box_remove(GTK_BOX(list_box_deb), child);
    }

    for (int i = 0; i < total_deb_files; i++) {
        DebFileInfo *deb = &deb_files[i];

        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_top(card, 6);
        gtk_widget_set_margin_bottom(card, 6);

        // Package Icon
        GtkWidget *img_icon = gtk_image_new_from_icon_name("package-x-generic");
        gtk_image_set_pixel_size(GTK_IMAGE(img_icon), 36);
        gtk_box_append(GTK_BOX(card), img_icon);

        /* ---------------------------------------------------------------------
         * Escape mọi giá trị lấy từ hệ thống tệp trước khi ghép vào markup.
         * ---------------------------------------------------------------------
         * Tên gói và ĐƯỜNG DẪN ở đây do máy người dùng quyết định, không phải do
         * ta. Chỉ cần một tệp nằm trong thư mục kiểu "Tải về & lưu trữ/" là dấu
         * & trần phá vỡ cú pháp Pango, và khi đó Pango bỏ TOÀN BỘ chuỗi: cả dòng
         * thông tin của gói đó biến mất khỏi danh sách, không còn thấy tên lẫn
         * nút thao tác nào.
         *
         * Màu trạng thái tách riêng vì g_markup_printf_escaped() sẽ escape luôn
         * cả thẻ <span> nếu truyền chúng qua %s.
         * ------------------------------------------------------------------ */
        const char *state_color = deb->is_installed ? "#40a02b" : "#fe640b";
        const char *state_text  = deb->is_installed ? "Đã cài đặt" : "Chưa cài đặt";

        char *info_str = g_markup_printf_escaped(
            "<b>%s</b> <small>(v%s • %s • %s)</small>\n"
            "<small><span foreground='#6c7086'>Path: %s • Trạng thái: </span>"
            "<span foreground='%s'><b>%s</b></span></small>",
            deb->package_name, deb->version, deb->arch, deb->size_str,
            deb->file_path, state_color, state_text);

        GtkWidget *lbl_info = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl_info), info_str);
        g_free(info_str);
        gtk_label_set_xalign(GTK_LABEL(lbl_info), 0.0f);
        gtk_widget_set_hexpand(lbl_info, TRUE);
        gtk_box_append(GTK_BOX(card), lbl_info);

        // Actions
        GtkWidget *btn_install = tizen_button_new("folder-download-symbolic", "Cài Đặt 1-Click");
        g_signal_connect(btn_install, "clicked", G_CALLBACK(on_install_deb_file_clicked), g_strdup(deb->file_path));
        gtk_box_append(GTK_BOX(card), btn_install);

        GtkWidget *btn_del = tizen_button_new("user-trash-symbolic", "Xóa File");
        g_signal_connect(btn_del, "clicked", G_CALLBACK(on_delete_deb_file_clicked), g_strdup(deb->file_path));
        gtk_box_append(GTK_BOX(card), btn_del);

        gtk_box_append(GTK_BOX(list_box_deb), card);
    }

    if (total_deb_files == 0) {
        GtkWidget *empty_lbl = gtk_label_new("Không tìm thấy tệp gói .deb nào trong ~/Downloads, /tmp, hoặc /var/cache/apt/archives.");
        gtk_widget_set_margin_top(empty_lbl, 16);
        gtk_box_append(GTK_BOX(list_box_deb), empty_lbl);
    }
}

GtkWidget* create_deb_files_view(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(vbox, 16);
    gtk_widget_set_margin_end(vbox, 16);
    gtk_widget_set_margin_top(vbox, 16);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);

    list_box_deb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box_deb);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    refresh_deb_files_list();

    return vbox;
}
