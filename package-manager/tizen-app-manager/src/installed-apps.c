/*
 * Tizen App Manager - Installed Applications View Implementation
 * =============================================================================
 * Quản lý danh sách toàn bộ ứng dụng đã cài đặt trên TizenOS (.tpk & .deb),
 * lọc tìm kiếm thời gian thực, mở ứng dụng, gỡ cài đặt và xóa cache.
 * =============================================================================
 */

#include "app-manager.h"
#include "tizen/pkg.h"

#include "tizen/theme.h"         /* tizen_button_new — nút dùng icon theme */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define TAG "INSTALLED_APPS"
#define APPS_DIR "/usr/share/applications"
#define MAX_APPS 400

static InstalledAppInfo app_list[MAX_APPS];
static int total_apps = 0;
static GtkWidget *list_box_apps = NULL;

/* Callback mở ứng dụng */
static void on_launch_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *exec = (const char*)user_data;
    if (!exec) return;

    /* SEC-016: bản cũ nối "%s &" rồi g_spawn_command_line_async. Hàm đó không
     * chạy shell nên "&" thành tham số thật, và trường Exec= của .desktop còn
     * nguyên field code %U/%F cũng bị truyền cho chương trình.
     * tizen_app_launch_exec() loại field code rồi để GIO khởi chạy đúng cách. */
    g_autoptr(GError) err = NULL;
    if (!tizen_app_launch_exec(exec, &err))
        g_warning("[%s] Không mở được ứng dụng: %s",
                  TAG, err ? err->message : "lỗi không rõ");
}

/* -----------------------------------------------------------------------------
 * Bản đồ tệp .desktop -> tên gói Debian sở hữu
 * -----------------------------------------------------------------------------
 * Tên tệp .desktop KHÔNG phải tên gói. `org.gnome.Evince.desktop` thuộc gói
 * `evince`; `code.desktop` thuộc `code`. Bản cũ đưa thẳng app_id (tên tệp bỏ
 * đuôi) cho apt-get remove, nên với mọi ứng dụng có hai tên khác nhau thì nút
 * "Gỡ bỏ" hoặc bị từ chối vì tên không hợp lệ, hoặc chạy remove một gói không
 * tồn tại rồi báo thành công — gói thật vẫn nằm nguyên trên máy.
 *
 * Gọi `dpkg -S` MỘT lần cho cả thư mục thay vì một lần cho mỗi tệp: máy cài
 * nhiều app có hàng trăm .desktop, spawn từng tiến trình cho mỗi cái sẽ làm
 * treo giao diện mỗi lần bấm "Làm mới".
 *
 * Trả về bảng băm (đường dẫn -> tên gói), hoặc NULL nếu không có dpkg.
 * -------------------------------------------------------------------------- */
static GHashTable *build_desktop_owner_map(void)
{
    char *dpkg = g_find_program_in_path("dpkg");
    if (!dpkg) return NULL;

    /* Mẫu phải kèm phần khớp tên tệp, không được là thư mục trần.
     * Đưa vào thư mục trần thì dpkg khớp chính THƯ MỤC đó và trả về đúng một
     * dòng gộp mọi gói sở hữu nó ("pkg1, pkg2, ...: /usr/share/applications"),
     * không có dòng nào cho từng tệp — bảng băm rỗng và mọi thứ lặng lẽ lùi về
     * app_id, tức là lỗi vẫn còn nguyên.
     *
     * g_spawn_sync KHÔNG qua shell, nên ký tự đại diện đến tay dpkg nguyên vẹn
     * và chính dpkg làm việc khớp mẫu — đúng ý đồ. */
    char *argv[] = { dpkg, (char *)"-S", (char *)APPS_DIR "/*.desktop", NULL };
    char *out = NULL;
    int status = 0;

    gboolean ok = g_spawn_sync(NULL, argv, NULL, G_SPAWN_STDERR_TO_DEV_NULL,
                               NULL, NULL, &out, NULL, &status, NULL);
    g_free(dpkg);

    /* dpkg -S trả khác 0 khi không khớp gì — đó là trạng thái hợp lệ, không lỗi. */
    if (!ok || !out) {
        g_free(out);
        return NULL;
    }

    GHashTable *map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    char **lines = g_strsplit(out, "\n", -1);
    for (int i = 0; lines[i]; i++) {
        /* Mỗi dòng: "gói: /đường/dẫn"  — hoặc "gói1, gói2: /đường/dẫn" khi
         * nhiều gói cùng sở hữu (hiếm, thường do diversion). Lấy gói đầu. */
        char *sep = strstr(lines[i], ": /");
        if (!sep) continue;

        *sep = '\0';
        const char *path = sep + 2;

        char *comma = strchr(lines[i], ',');
        if (comma) *comma = '\0';

        /* Tên gói có thể kèm kiến trúc: "evince:amd64" -> bỏ phần sau dấu hai chấm. */
        char *colon = strchr(lines[i], ':');
        if (colon) *colon = '\0';

        char *pkg = g_strstrip(g_strdup(lines[i]));
        if (*pkg)
            g_hash_table_insert(map, g_strdup(g_strstrip((char *)path)), pkg);
        else
            g_free(pkg);
    }
    g_strfreev(lines);
    g_free(out);

    return map;
}

/* Chạy trên main loop sau khi apt-get remove kết thúc. */
static void on_uninstall_finished(bool ok, const char *output,
                                  const GError *error, gpointer user_data) {
    (void)output; (void)user_data;
    if (!ok)
        g_warning("[%s] Gỡ gói thất bại: %s", TAG,
                  error ? error->message : "người dùng huỷ xác thực hoặc apt lỗi");
    refresh_installed_apps_list(NULL, "all");
}

static void on_uninstall_dialog_response(GtkDialog *dlg, int resp, gpointer data) {
    char *id = (char*)data;
    if (resp == GTK_RESPONSE_YES) {
        /* SEC-019: system() ở đây chặn main loop suốt quá trình gỡ gói.
         * SEC-012: id lấy từ tên tệp .desktop nên là dữ liệu ngoài.
         * Chuỗi cũ còn kết thúc bằng "|| true" nên MỌI thất bại đều bị nuốt,
         * người dùng tưởng đã gỡ xong trong khi không có gì xảy ra. */
        if (!tizen_pkg_is_valid_name(id)) {
            g_warning("[%s] Từ chối gỡ: tên gói không hợp lệ '%s'", TAG, id);
        } else {
            tizen_pkg_remove_async(id, false, on_uninstall_finished, NULL);
        }
    }
    gtk_window_destroy(GTK_WINDOW(dlg));
    g_free(id);
}

/* Callback gỡ ứng dụng */
static void on_uninstall_clicked(GtkButton *btn, gpointer user_data) {
    const char *app_id = (const char*)user_data;
    if (!app_id) return;

    GtkWidget *parent_win = GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(btn)));

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(parent_win),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "🗑️ Bạn có chắc chắn muốn gỡ bỏ ứng dụng '%s' khỏi TizenOS?", app_id
    );

    g_signal_connect(dialog, "response", G_CALLBACK(on_uninstall_dialog_response), g_strdup(app_id));
    gtk_widget_set_visible(dialog, TRUE);
}

/* Callback xóa bộ nhớ đệm */
static void on_clear_cache_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *app_id = (const char*)user_data;
    if (!app_id) return;

    /* SEC-018: bản cũ chạy rm -rf '$HOME/.cache/<app>' — biến nằm TRONG nháy
     * đơn nên shell không mở rộng, lệnh cố xoá một thư mục tên literal
     * "$HOME/.cache/<app>" và thực tế KHÔNG xoá được gì. Nút này hoàn toàn giả.
     * Đã kiểm chứng bằng PoC. */
    if (tizen_app_clear_cache(app_id))
        g_print("[%s] Đã xóa bộ nhớ đệm của '%s'\n", TAG, app_id);
    else
        g_warning("[%s] Không xoá được bộ nhớ đệm của '%s'", TAG, app_id);
}

/* Quét danh sách ứng dụng từ tệp .desktop */
static void scan_installed_apps(void) {
    total_apps = 0;
    DIR *dir = opendir(APPS_DIR);
    if (!dir) return;

    /* Bản đồ đường-dẫn -> tên gói Debian, dựng MỘT lần cho cả lượt quét. */
    GHashTable *owner_map = build_desktop_owner_map();

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && total_apps < MAX_APPS) {
        if (strstr(entry->d_name, ".desktop")) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", APPS_DIR, entry->d_name);

            GKeyFile *kf = g_key_file_new();
            if (g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
                gboolean no_display = g_key_file_get_boolean(kf, "Desktop Entry", "NoDisplay", NULL);
                if (no_display) {
                    g_key_file_free(kf);
                    continue;
                }

                char *name = g_key_file_get_string(kf, "Desktop Entry", "Name", NULL);
                char *exec = g_key_file_get_string(kf, "Desktop Entry", "Exec", NULL);
                char *icon = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
                char *pkg_type = g_key_file_get_string(kf, "Desktop Entry", "X-Tizen-Package-Type", NULL);

                if (name && exec) {
                    InstalledAppInfo *app = &app_list[total_apps];
                    /* g_strlcpy chứ không phải strncpy.
                     * strncpy(dst, src, sizeof(dst)-1) KHÔNG kết thúc chuỗi khi
                     * nguồn bị cắt: nó ghi đúng n byte rồi dừng, để nguyên byte
                     * cuối. app_list là mảng dùng lại giữa các lần quét, nên byte
                     * đó có thể còn rác của mục dài hơn ở lượt trước — kết quả là
                     * một chuỗi không có ký tự kết thúc. app_id chỉ có 128 byte
                     * trong khi d_name tới 256, nên đây là đường thật sự chạm tới.
                     * g_strlcpy luôn kết thúc chuỗi. */
                    g_strlcpy(app->name, name, sizeof(app->name));
                    g_strlcpy(app->exec, exec, sizeof(app->exec));
                    g_strlcpy(app->icon, icon ? icon : "application-x-executable",
                              sizeof(app->icon));

                    g_strlcpy(app->app_id, entry->d_name, sizeof(app->app_id));
                    char *dot = strrchr(app->app_id, '.');
                    if (dot) *dot = '\0';

                    /* Tên gói thật để gỡ cài đặt. Không tra được (gói .tpk, hoặc
                     * tệp .desktop không thuộc gói nào) thì lùi về app_id. */
                    const char *owner = owner_map
                        ? g_hash_table_lookup(owner_map, path) : NULL;
                    g_strlcpy(app->pkg_name, owner ? owner : app->app_id,
                              sizeof(app->pkg_name));

                    snprintf(app->type, sizeof(app->type), "%s", pkg_type ? pkg_type : "deb");
                    snprintf(app->smack_label, sizeof(app->smack_label), "%s", pkg_type ? "User" : "System");
                    snprintf(app->version, sizeof(app->version), "1.0.0");

                    total_apps++;
                }

                g_free(name); g_free(exec); g_free(icon); g_free(pkg_type);
            }
            g_key_file_free(kf);
        }
    }
    closedir(dir);
    if (owner_map) g_hash_table_destroy(owner_map);
}

void refresh_installed_apps_list(const char *search_query, const char *filter_type) {
    if (!list_box_apps) return;

    scan_installed_apps();

    // Xóa các widget cũ an toàn
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(list_box_apps)) != NULL) {
        gtk_box_remove(GTK_BOX(list_box_apps), child);
    }

    char *query_fold = (search_query && strlen(search_query) > 0) ? g_utf8_casefold(search_query, -1) : NULL;

    int rendered_count = 0;
    for (int i = 0; i < total_apps; i++) {
        InstalledAppInfo *app = &app_list[i];

        // Type filter check
        if (filter_type && strcmp(filter_type, "all") != 0) {
            if (strcmp(filter_type, "tpk") == 0 && strcmp(app->type, "tpk") != 0) continue;
            if (strcmp(filter_type, "deb") == 0 && strcmp(app->type, "deb") != 0) continue;
        }

        // Search query check
        if (query_fold) {
            char *name_fold = g_utf8_casefold(app->name, -1);
            char *id_fold = g_utf8_casefold(app->app_id, -1);

            bool match = (strstr(name_fold, query_fold) != NULL || strstr(id_fold, query_fold) != NULL);
            g_free(name_fold); g_free(id_fold);

            if (!match) continue;
        }

        // Row Widget
        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_top(card, 6);
        gtk_widget_set_margin_bottom(card, 6);

        // Icon
        GtkWidget *img_icon = gtk_image_new_from_icon_name(app->icon);
        gtk_image_set_pixel_size(GTK_IMAGE(img_icon), 40);
        gtk_box_append(GTK_BOX(card), img_icon);

        /* Escape: app->name đến từ trường Name= của tệp .desktop bất kỳ trên máy.
         * Rất nhiều ứng dụng phổ biến có & trong tên ("Files & Folders",
         * "Sound & Video"). Không escape thì đúng những mục đó hiện ra dòng
         * trống — xem ghi chú chi tiết ở deb-files.c. */
        char *info_markup = g_markup_printf_escaped(
            "<b>%s</b>\n<small>ID: %s • Gói: <span foreground='#0069B4'>%s</span>"
            " • Smack MAC: <b>%s</b></small>",
            app->name, app->app_id,
            strcmp(app->type, "tpk") == 0 ? "Tizen Native (.tpk)" : "Debian 12 (.deb)",
            app->smack_label);

        GtkWidget *lbl_info = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lbl_info), info_markup);
        g_free(info_markup);
        gtk_label_set_xalign(GTK_LABEL(lbl_info), 0.0f);
        gtk_widget_set_hexpand(lbl_info, TRUE);
        gtk_box_append(GTK_BOX(card), lbl_info);

        // Buttons
        GtkWidget *btn_launch = tizen_button_new("media-playback-start-symbolic", "Mở App");
        g_signal_connect(btn_launch, "clicked", G_CALLBACK(on_launch_clicked), g_strdup(app->exec));
        gtk_box_append(GTK_BOX(card), btn_launch);

        GtkWidget *btn_cache = tizen_button_new("edit-clear-all-symbolic", "Cache");
        g_signal_connect(btn_cache, "clicked", G_CALLBACK(on_clear_cache_clicked), g_strdup(app->app_id));
        gtk_box_append(GTK_BOX(card), btn_cache);

        GtkWidget *btn_uninstall = tizen_button_new("user-trash-symbolic", "Gỡ bỏ");
        /* pkg_name, KHÔNG phải app_id — xem build_desktop_owner_map().
         * app_id là tên tệp .desktop; apt-get remove cần tên gói thật. */
        g_signal_connect(btn_uninstall, "clicked", G_CALLBACK(on_uninstall_clicked), g_strdup(app->pkg_name));
        gtk_box_append(GTK_BOX(card), btn_uninstall);

        gtk_box_append(GTK_BOX(list_box_apps), card);
        rendered_count++;
    }

    if (query_fold) g_free(query_fold);

    if (rendered_count == 0) {
        GtkWidget *empty_lbl = gtk_label_new("Không tìm thấy ứng dụng phù hợp.");
        gtk_widget_set_margin_top(empty_lbl, 16);
        gtk_box_append(GTK_BOX(list_box_apps), empty_lbl);
    }
}

GtkWidget* create_installed_apps_view(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(vbox, 16);
    gtk_widget_set_margin_end(vbox, 16);
    gtk_widget_set_margin_top(vbox, 16);

    // ListBox
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);

    list_box_apps = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box_apps);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    refresh_installed_apps_list(NULL, "all");

    return vbox;
}
