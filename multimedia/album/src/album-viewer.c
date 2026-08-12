#include "tizen-album.h"
#include <tizen/glib-compat.h>
#include <tizen/theme.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* GTK4 bỏ gtk_dialog_run(): hộp thoại phải tự huỷ trong handler "response".
 * Dùng hàm riêng thay vì ép kiểu gtk_window_destroy — chữ ký khác nhau
 * (GtkWindow* vs GtkDialog*,int,gpointer) và -Wcast-function-type sẽ kêu. */
static void on_dialog_dismiss(GtkDialog *dlg, int response, gpointer data) {
    (void)response; (void)data;
    gtk_window_destroy(GTK_WINDOW(dlg));
}

static char *format_file_size(goffset bytes) {
    if (bytes < 1024) return g_strdup_printf("%" G_GOFFSET_FORMAT " B", bytes);
    if (bytes < 1024 * 1024) return g_strdup_printf("%.1f KB", (double)bytes / 1024.0);
    if (bytes < 1024 * 1024 * 1024) return g_strdup_printf("%.1f MB", (double)bytes / (1024.0 * 1024.0));
    return g_strdup_printf("%.2f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
}

void album_apply_filter(FilterType filter) {
    g_album_app.current_filter = filter;

    if (g_album_app.filtered_list) {
        g_list_free(g_album_app.filtered_list);
        g_album_app.filtered_list = NULL;
    }

    const char *query = g_album_app.search_query;

    /* Hạ chữ thường CHUỖI TÌM KIẾM đúng một lần, ngoài vòng lặp.
     * Bản cũ gọi g_utf8_strdown(query) lại cho từng tệp: gõ một ký tự vào ô tìm
     * kiếm trên thư viện 5000 ảnh là 5000 lần cấp phát thừa, và vì "search-changed"
     * bắn theo mỗi phím nên độ trễ dồn lại thấy rõ khi gõ. */
    char *query_fold = NULL;
    if (query && query[0] != '\0')
        query_fold = g_utf8_casefold(query, -1);

    for (GList *l = g_album_app.media_list; l != NULL; l = l->next) {
        MediaItem *item = (MediaItem *)l->data;

        // Filter Type Check
        if (filter == FILTER_PHOTOS && item->type != MEDIA_TYPE_PHOTO) continue;
        if (filter == FILTER_VIDEOS && item->type != MEDIA_TYPE_VIDEO) continue;
        if (filter == FILTER_FAVORITES && !item->is_favorite) continue;

        // Search Query Check
        if (query_fold) {
            /* casefold thay vì strdown: so khớp không phân biệt hoa thường đúng
             * với tiếng Việt có dấu, thứ mà strdown xử lý không nhất quán. */
            char *name_fold = g_utf8_casefold(item->filename, -1);
            gboolean matches = (strstr(name_fold, query_fold) != NULL);
            g_free(name_fold);
            if (!matches) continue;
        }

        /* prepend O(1) rồi đảo ngược một lần ở cuối, thay cho append O(n) mỗi
         * lượt (tức O(n²) cho cả danh sách). */
        g_album_app.filtered_list = g_list_prepend(g_album_app.filtered_list, item);
    }

    g_album_app.filtered_list = g_list_reverse(g_album_app.filtered_list);
    g_free(query_fold);

    album_refresh_grid();
}

/* =============================================================================
 * NẠP ẢNH THU NHỎ Ở LUỒNG NỀN
 * =============================================================================
 * Bản cũ gọi gdk_pixbuf_new_from_file_at_scale() ngay trong vòng lặp dựng lưới,
 * trên luồng giao diện. Mỗi ảnh tốn hàng chục tới hàng trăm mili-giây để giải
 * mã, nên một thư mục 300 ảnh làm đóng băng toàn bộ cửa sổ hàng chục giây — cửa
 * sổ không vẽ lại, không nhận chuột, và trình quản lý cửa sổ báo "ứng dụng không
 * phản hồi". Đây là lý do app trông như "không khởi động" dù tiến trình vẫn sống.
 *
 * Bây giờ lưới hiện NGAY với biểu tượng giữ chỗ, còn ảnh thật do một nhóm luồng
 * giải mã rồi gắn vào sau.
 *
 * AN TOÀN VÒNG ĐỜI: luồng nền KHÔNG chạm vào MediaItem — nó chỉ nhận một bản sao
 * đường dẫn và một tham chiếu tới widget đích. Nhờ vậy album_remove_item() hay
 * album_clear_media() có chạy giữa chừng cũng không thể gây use-after-free.
 * Bộ đếm "generation" loại bỏ kết quả về muộn thuộc về lưới cũ.
 * ========================================================================== */
#define THUMB_W 160
#define THUMB_H 110
#define THUMB_THREADS 4

/* Trần bộ nhớ đệm thumbnail.
 * Mỗi pixbuf 160x110 RGBA chiếm ~70 KB. Trần cũ 2000 tấm nghĩa là riêng phần
 * đệm đã có thể ngốn ~140 MB RSS — quá nhiều cho một trình xem ảnh, nhất là
 * trên máy cấu hình thấp mà TizenOS nhắm tới. 512 tấm (~36 MB) vẫn thừa sức
 * phủ vài màn hình cuộn, và nhờ có đệm trên đĩa bên dưới, tấm bị đẩy ra khỏi
 * RAM cũng chỉ tốn một lần đọc PNG nhỏ chứ không phải giải mã lại ảnh gốc. */
#define THUMB_CACHE_MAX 512

/* =============================================================================
 * ĐỆM THUMBNAIL TRÊN ĐĨA — freedesktop.org Thumbnail Managing Standard
 * =============================================================================
 * Đường dẫn: $XDG_CACHE_HOME/thumbnails/large/<md5 của URI>.png   (tối đa 256px)
 *
 * Vì sao theo chuẩn thay vì tự bịa thư mục đệm:
 *   - Dùng CHUNG với Nautilus, Thunar, Dolphin, tizenos-files... Ảnh đã được
 *     trình quản lý tệp tạo thumbnail thì Album mở ra là có ngay, không phải
 *     giải mã lại lần nào.
 *   - Cùng lý do ngược lại: thumbnail Album tạo ra được các app khác dùng lại.
 *   - Có sẵn công cụ dọn dẹp theo chuẩn, không để rác đệm phình vô hạn.
 *
 * Chuẩn bắt buộc ghi kèm hai khoá tEXt trong PNG:
 *      Thumb::URI    URI gốc
 *      Thumb::MTime  mtime của tệp gốc (giây, thập phân)
 * Đọc lại phải đối chiếu MTime; lệch nghĩa là ảnh gốc đã đổi -> bỏ đệm, tạo lại.
 * Bỏ qua bước đối chiếu này là lỗi kinh điển: sửa ảnh xong mà lưới vẫn hiện
 * bản cũ mãi mãi.
 * ========================================================================== */
#define THUMB_DISK_MAX 256

static char *thumb_cache_path(const char *filepath, char **out_uri)
{
    char *uri = g_filename_to_uri(filepath, NULL, NULL);
    if (!uri)
        return NULL;

    char *digest = g_compute_checksum_for_string(G_CHECKSUM_MD5, uri, -1);
    char *name = g_strconcat(digest, ".png", NULL);
    char *path = g_build_filename(g_get_user_cache_dir(), "thumbnails", "large",
                                  name, NULL);
    g_free(digest);
    g_free(name);

    if (out_uri)
        *out_uri = uri;
    else
        g_free(uri);

    return path;
}

/* Trả về pixbuf từ đệm đĩa nếu còn hợp lệ, ngược lại NULL. */
static GdkPixbuf *thumb_disk_load(const char *filepath)
{
    char *path = thumb_cache_path(filepath, NULL);
    if (!path)
        return NULL;

    GdkPixbuf *pix = gdk_pixbuf_new_from_file(path, NULL);
    g_free(path);
    if (!pix)
        return NULL;

    /* Đối chiếu mtime — xem ghi chú ở trên. */
    const char *cached_mtime = gdk_pixbuf_get_option(pix, "tEXt::Thumb::MTime");
    GStatBuf st;
    if (!cached_mtime || g_stat(filepath, &st) != 0 ||
        g_ascii_strtoll(cached_mtime, NULL, 10) != (gint64)st.st_mtime) {
        g_object_unref(pix);
        return NULL;
    }
    return pix;
}

static void thumb_disk_store(const char *filepath, GdkPixbuf *pix)
{
    char *uri = NULL;
    char *path = thumb_cache_path(filepath, &uri);
    if (!path) {
        g_free(uri);
        return;
    }

    char *dir = g_path_get_dirname(path);
    /* 0700 theo chuẩn: thư mục đệm có thể chứa xem trước của ảnh riêng tư,
     * không được để người dùng khác trên cùng máy đọc. */
    g_mkdir_with_parents(dir, 0700);
    g_free(dir);

    GStatBuf st;
    if (g_stat(filepath, &st) == 0) {
        char mtime_str[32];
        g_snprintf(mtime_str, sizeof(mtime_str), "%" G_GINT64_FORMAT,
                   (gint64)st.st_mtime);

        /* Ghi ra tệp tạm rồi rename: hai tiến trình cùng tạo thumbnail cho một
         * ảnh sẽ không đọc phải tệp PNG mới ghi được một nửa. */
        char *tmp = g_strdup_printf("%s.tmp-%d", path, (int)getpid());
        if (gdk_pixbuf_save(pix, tmp, "png", NULL,
                            "tEXt::Thumb::URI", uri,
                            "tEXt::Thumb::MTime", mtime_str, NULL)) {
            g_rename(tmp, path);
            g_chmod(path, 0600);
        } else {
            g_unlink(tmp);
        }
        g_free(tmp);
    }

    g_free(path);
    g_free(uri);
}

static GThreadPool *thumb_pool = NULL;
static GHashTable  *thumb_cache = NULL;   /* char* path -> GdkPixbuf* (cả hai đều sở hữu) */
static guint        thumb_generation = 0;

typedef struct {
    char      *path;
    GtkWidget *target;      /* GtkPicture, đã giữ tham chiếu */
    guint      generation;
    MediaType  type;
    GdkPixbuf *result;      /* luồng nền ghi, luồng chính đọc */
} ThumbJob;

static void thumb_job_free(ThumbJob *job)
{
    g_free(job->path);
    if (job->target) g_object_unref(job->target);
    if (job->result) g_object_unref(job->result);
    g_free(job);
}

/* Chạy trên LUỒNG CHÍNH. */
static gboolean thumb_apply_cb(gpointer data)
{
    ThumbJob *job = (ThumbJob *)data;

    /* Lưới đã bị thay -> kết quả này thuộc về lượt trước, bỏ đi. */
    if (job->generation == thumb_generation && job->result) {
        if (!thumb_cache) {
            thumb_cache = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                g_free, g_object_unref);
        }
        /* Chặn trần bộ nhớ đệm. Duyệt hết thư viện ảnh lớn mà giữ mọi thumbnail
         * thì riêng phần đệm đã nuốt hàng trăm MB. */
        if (g_hash_table_size(thumb_cache) >= THUMB_CACHE_MAX)
            g_hash_table_remove_all(thumb_cache);

        g_hash_table_insert(thumb_cache, g_strdup(job->path),
                            g_object_ref(job->result));

        if (GTK_IS_PICTURE(job->target))
            gtk_picture_set_pixbuf(GTK_PICTURE(job->target), job->result);
    }

    thumb_job_free(job);
    return G_SOURCE_REMOVE;
}

/* -----------------------------------------------------------------------------
 * Ảnh xem trước cho VIDEO
 * -----------------------------------------------------------------------------
 * gdk-pixbuf không đọc được video, nên trước đây mọi video trong lưới chỉ là
 * một biểu tượng cuộn phim giống hệt nhau. Thư mục 50 clip cho ra 50 ô y hệt —
 * không cách nào biết clip nào là clip nào, coi như lưới vô dụng với video.
 *
 * Trích một khung hình bằng công cụ thumbnailer có sẵn của hệ thống. Chạy trong
 * luồng nền nên tiến trình con không làm nghẽn giao diện. Không có công cụ nào
 * thì lặng lẽ quay về biểu tượng — tuyệt đối không để app chết vì thiếu nó.
 * -------------------------------------------------------------------------- */
static GdkPixbuf *video_thumbnail(const char *filepath)
{
    static const char *tools[] = { "ffmpegthumbnailer", "totem-video-thumbnailer", NULL };

    for (int i = 0; tools[i]; i++) {
        char *tool = g_find_program_in_path(tools[i]);
        if (!tool)
            continue;

        char *tmpl = g_build_filename(g_get_tmp_dir(), "tizen-album-vthumb-XXXXXX.png", NULL);
        int fd = g_mkstemp(tmpl);
        if (fd < 0) {
            g_free(tool);
            g_free(tmpl);
            continue;
        }
        close(fd);

        char size_str[16];
        g_snprintf(size_str, sizeof(size_str), "%d", THUMB_DISK_MAX);

        char *argv_ffmpeg[] = { tool, (char *)"-i", (char *)filepath,
                                (char *)"-o", tmpl, (char *)"-s", size_str,
                                (char *)"-q", (char *)"8", NULL };
        char *argv_totem[]  = { tool, (char *)"-s", size_str,
                                (char *)filepath, tmpl, NULL };
        char **argv = (i == 0) ? argv_ffmpeg : argv_totem;

        int status = 0;
        gboolean ok = g_spawn_sync(NULL, argv, NULL,
                                   G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                                   NULL, NULL, NULL, NULL, &status, NULL);

        GdkPixbuf *pix = NULL;
        if (ok && g_spawn_check_wait_status(status, NULL))
            pix = gdk_pixbuf_new_from_file_at_scale(tmpl, THUMB_W, THUMB_H, TRUE, NULL);

        g_unlink(tmpl);
        g_free(tmpl);
        g_free(tool);

        if (pix)
            return pix;
    }
    return NULL;
}

/* Chạy trên LUỒNG NỀN. */
static void thumb_worker(gpointer data, gpointer user_data)
{
    (void)user_data;
    ThumbJob *job = (ThumbJob *)data;

    /* Kiểm tra generation trước khi giải mã: hàng đợi có thể còn tồn hàng trăm
     * việc của thư mục vừa bị đóng, không cần tốn CPU giải mã chúng. */
    if (job->generation == thumb_generation) {

        /* 1. Đệm trên đĩa trước — rẻ hơn hẳn việc giải mã lại ảnh gốc, và có
         *    thể do trình quản lý tệp tạo sẵn từ trước. */
        job->result = thumb_disk_load(job->path);

        if (!job->result) {
            /* 2. Tạo mới. */
            if (job->type == MEDIA_TYPE_VIDEO)
                job->result = video_thumbnail(job->path);
            else
                job->result = gdk_pixbuf_new_from_file_at_scale(
                    job->path, THUMB_W, THUMB_H, TRUE, NULL);

            /* 3. Ghi lại vào đệm đĩa cho lần mở sau và cho app khác. */
            if (job->result)
                thumb_disk_store(job->path, job->result);
        }
    }

    g_idle_add(thumb_apply_cb, job);
}

/* Bộ đếm lượt dựng lưới + nguồn idle đang dựng dở. Khai báo ở đây vì
 * album_cancel_thumbnails() phải huỷ được cả hai cùng lúc. */
static guint grid_generation = 0;
static guint grid_fill_source = 0;

void album_cancel_thumbnails(void)
{
    /* Không huỷ pool — chỉ vô hiệu hoá mọi việc đang bay. Việc còn dở sẽ tự
     * thoát sớm ở thumb_worker, kết quả về muộn bị thumb_apply_cb loại bỏ. */
    thumb_generation++;

    /* Huỷ luôn lượt dựng lưới dở dang. BẮT BUỘC: nguồn idle giữ con trỏ đi
     * giữa filtered_list, mà album_clear_media() sắp giải phóng danh sách đó.
     * Bỏ sót chỗ này là đọc vào bộ nhớ đã free ngay lần đổi thư mục kế tiếp. */
    grid_generation++;
    if (grid_fill_source != 0) {
        g_source_remove(grid_fill_source);
        grid_fill_source = 0;
    }
}

static void thumb_request(const char *path, GtkWidget *target, MediaType type)
{
    if (!thumb_pool) {
        thumb_pool = g_thread_pool_new(thumb_worker, NULL, THUMB_THREADS, FALSE, NULL);
        if (!thumb_pool) return;      /* không tạo được luồng -> giữ nguyên icon */
    }

    ThumbJob *job = g_new0(ThumbJob, 1);
    job->path = g_strdup(path);
    job->target = GTK_WIDGET(g_object_ref(target));
    job->generation = thumb_generation;
    job->type = type;

    if (!g_thread_pool_push(thumb_pool, job, NULL))
        thumb_job_free(job);
}

/* Biểu tượng giữ chỗ dưới dạng GdkPaintable, để ô giữ chỗ và ảnh thật dùng
 * CHUNG một widget GtkPicture — không phải tháo lắp widget khi thumbnail về. */
static GdkPaintable *placeholder_paintable(MediaType type)
{
    GtkIconTheme *theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    if (!theme) return NULL;

    const char *icon_name = (type == MEDIA_TYPE_VIDEO)
                            ? "video-x-generic" : "image-x-generic";

    GtkIconPaintable *icon = gtk_icon_theme_lookup_icon(
        theme, icon_name, NULL, 64, 1, GTK_TEXT_DIR_LTR, 0);
    return icon ? GDK_PAINTABLE(icon) : NULL;
}

static void on_card_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
    (void)gesture; (void)n_press; (void)x; (void)y;
    int index = GPOINTER_TO_INT(user_data);
    album_show_viewer_at(index);
}

/* Dựng một thẻ trong lưới. Tách riêng để vòng lặp theo lô ở dưới gọn gàng. */
static GtkWidget *create_media_card(MediaItem *item, int index)
{
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(card, "card");
    gtk_widget_set_size_request(card, 180, 160);
    gtk_widget_set_margin_start(card, 6);
    gtk_widget_set_margin_end(card, 6);
    gtk_widget_set_margin_top(card, 6);
    gtk_widget_set_margin_bottom(card, 6);

    /* Ô ảnh dựng NGAY với biểu tượng giữ chỗ; ảnh thật do luồng nền gắn vào
     * sau. Lưới vì thế hiện tức thì kể cả với thư mục vài nghìn ảnh. */
    GtkWidget *img_widget = gtk_picture_new();
    tizen_picture_set_contain(img_widget);
    gtk_widget_set_size_request(img_widget, THUMB_W, THUMB_H);

    GdkPixbuf *cached = thumb_cache
        ? g_hash_table_lookup(thumb_cache, item->filepath) : NULL;

    if (cached) {
        gtk_picture_set_pixbuf(GTK_PICTURE(img_widget), cached);
    } else {
        GdkPaintable *ph = placeholder_paintable(item->type);
        if (ph) {
            gtk_picture_set_paintable(GTK_PICTURE(img_widget), ph);
            g_object_unref(ph);
        }
        /* Cả ảnh LẪN video đều được đặt hàng xem trước. Video đi qua
         * video_thumbnail() để trích một khung hình. */
        thumb_request(item->filepath, img_widget, item->type);
    }

    gtk_box_append(GTK_BOX(card), img_widget);

    // Label Container (Title & Badge)
    GtkWidget *lbl_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(lbl_box, 8);
    gtk_widget_set_margin_end(lbl_box, 8);
    gtk_widget_set_margin_bottom(lbl_box, 6);

    GtkWidget *badge = gtk_image_new_from_icon_name(
        (item->type == MEDIA_TYPE_VIDEO) ? "video-x-generic-symbolic"
                                         : "image-x-generic-symbolic");
    gtk_box_append(GTK_BOX(lbl_box), badge);

    GtkWidget *lbl_name = gtk_label_new(item->filename);
    gtk_label_set_ellipsize(GTK_LABEL(lbl_name), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(lbl_name, TRUE);
    gtk_box_append(GTK_BOX(lbl_box), lbl_name);

    gtk_box_append(GTK_BOX(card), lbl_box);

    // Click Gesture
    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(on_card_clicked), GINT_TO_POINTER(index));
    gtk_widget_add_controller(card, GTK_EVENT_CONTROLLER(click));

    return card;
}

/* =============================================================================
 * DỰNG LƯỚI THEO LÔ
 * =============================================================================
 * Nạp thumbnail ở luồng nền mới giải quyết được một nửa vấn đề. Nửa còn lại là
 * việc DỰNG WIDGET: mỗi thẻ gồm GtkBox + GtkPicture + GtkBox + 2 GtkLabel +
 * GtkGestureClick, tức 6 GObject. Thư mục 5000 ảnh là 30.000 GObject được tạo
 * trong MỘT vòng lặp không nhả điều khiển — luồng chính kẹt cứng vài giây, cửa
 * sổ không vẽ lại được, và người dùng thấy app "đứng hình" y như cũ dù ảnh đã
 * giải mã ở nơi khác.
 *
 * Chia thành từng lô GRID_BATCH thẻ mỗi vòng idle: lô đầu tiên hiện ra ngay
 * trong khung hình kế tiếp, phần còn lại lấp dần mà chuột và cuộn vẫn mượt.
 * Kèm lợi ích phụ: thumbnail chỉ được đặt hàng khi thẻ tương ứng ra đời, nên
 * hàng đợi giải mã không bị dồn 5000 việc ngay lập tức.
 * ========================================================================== */
#define GRID_BATCH 32

typedef struct {
    GList *cursor;
    int    index;
    guint  generation;
} GridFill;

static gboolean grid_fill_cb(gpointer data)
{
    GridFill *fill = (GridFill *)data;

    /* Lưới đã bị thay giữa chừng -> dừng, đừng đụng vào filtered_list cũ. */
    if (fill->generation != grid_generation) {
        grid_fill_source = 0;
        return G_SOURCE_REMOVE;
    }

    for (int n = 0; n < GRID_BATCH && fill->cursor != NULL; n++) {
        MediaItem *item = (MediaItem *)fill->cursor->data;
        GtkWidget *card = create_media_card(item, fill->index);
        gtk_flow_box_append(GTK_FLOW_BOX(g_album_app.flowbox), card);

        fill->cursor = fill->cursor->next;
        fill->index++;
    }

    if (fill->cursor == NULL) {
        grid_fill_source = 0;
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

void album_refresh_grid(void) {
    /* Vô hiệu hoá thumbnail VÀ lượt dựng lưới cũ trước khi dựng lưới mới, nếu
     * không ảnh về muộn sẽ gắn nhầm vào ô của tệp khác. */
    album_cancel_thumbnails();

    // Clear Flowbox children safely
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(g_album_app.flowbox)) != NULL) {
        gtk_flow_box_remove(GTK_FLOW_BOX(g_album_app.flowbox), child);
    }

    /* Dựng lưới theo TỪNG LÔ qua idle, không dựng hết trong một vòng lặp.
     * Xem grid_fill_cb() bên dưới. */
    GridFill *fill = g_new0(GridFill, 1);
    fill->cursor = g_album_app.filtered_list;
    fill->index = 0;
    fill->generation = grid_generation;

    grid_fill_source = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                                       grid_fill_cb, fill, g_free);
}

void album_show_grid(void) {
    if (g_album_app.is_slideshow) {
        album_toggle_slideshow(); // Stop slideshow when returning to grid
    }
    gtk_stack_set_visible_child_name(GTK_STACK(g_album_app.stack), "grid");
}

void album_toggle_info(void) {
    g_album_app.is_info_open = !g_album_app.is_info_open;
    gtk_widget_set_visible(g_album_app.info_panel, g_album_app.is_info_open);
}

void album_toggle_fullscreen(void) {
    g_album_app.is_fullscreen = !g_album_app.is_fullscreen;
    if (g_album_app.is_fullscreen) {
        gtk_window_fullscreen(GTK_WINDOW(g_album_app.window));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(g_album_app.window));
    }
}

/* -----------------------------------------------------------------------------
 * CÓ PHÁT ĐƯỢC VIDEO KHÔNG? — vì sao phải kiểm tra trước
 * -----------------------------------------------------------------------------
 * gtk_video_set_file() đi qua GtkMediaFile -> backend GStreamer. Nếu backend nạp
 * được nhưng phần tử `playbin3` không có trong registry, thư viện GstPlay gọi
 * g_error():
 *
 *      GStreamer-Play-ERROR: GstPlay: 'playbin3' element not found,
 *                            please check your setup
 *
 * g_error() KHÔNG phải cảnh báo — nó gọi abort(). Toàn bộ tiến trình chết ngay
 * với SIGABRT (exit 134), không có cách nào bắt lại sau khi đã gọi. Người dùng
 * bấm vào một video là mất sạch app cùng mọi thứ đang mở.
 *
 * Không thể try/catch, nên phải KHÔNG BAO GIỜ gọi tới đường đó khi thiếu
 * playbin3. Kiểm tra bằng gst-inspect-1.0 trong một tiến trình con: nó thất bại
 * một cách lịch sự, còn ta thì không chết theo.
 *
 * Gói .deb đã khai báo gstreamer1.0-plugins-base và gstreamer1.0-tools nên bình
 * thường luôn có sẵn; hàm này là lớp phòng vệ cho máy bị gỡ bớt gói.
 * -------------------------------------------------------------------------- */
static gboolean video_playback_available(void)
{
    static int cached = -1;
    if (cached >= 0)
        return cached != 0;

    cached = 0;
    char *inspect = g_find_program_in_path("gst-inspect-1.0");
    if (inspect) {
        char *argv[] = { inspect, (char *)"playbin3", NULL };
        int status = 0;
        if (g_spawn_sync(NULL, argv, NULL,
                         G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                         NULL, NULL, NULL, NULL, &status, NULL) &&
            g_spawn_check_wait_status(status, NULL)) {
            cached = 1;
        }
        g_free(inspect);
    }
    return cached != 0;
}

/* Thay chỗ phát video bằng lời giải thích, thay vì để app chết. */
static void show_video_unavailable(MediaItem *item)
{
    gtk_widget_set_visible(g_album_app.video_view, FALSE);
    gtk_widget_set_visible(g_album_app.picture_view, TRUE);

    /* Vẫn hiện được ảnh xem trước đã trích từ video, nếu có trong đệm. */
    GdkPixbuf *pix = thumb_disk_load(item->filepath);
    if (pix) {
        gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.picture_view), pix);
        g_object_unref(pix);
    } else {
        gtk_picture_set_paintable(GTK_PICTURE(g_album_app.picture_view), NULL);
    }

    char *msg = g_markup_printf_escaped(
        "<b>Không phát được video</b>\n\n"
        "Thiếu phần tử GStreamer <tt>playbin3</tt>.\n"
        "Cài đặt bằng:\n"
        "<tt>sudo apt install gstreamer1.0-plugins-base \\\n"
        "    gstreamer1.0-plugins-good gstreamer1.0-libav \\\n"
        "    libgtk-4-media-gstreamer</tt>\n\n"
        "Tệp: %s", item->filename);
    gtk_label_set_markup(GTK_LABEL(g_album_app.lbl_info_details), msg);
    g_free(msg);

    /* Mở panel thông tin để thông báo không bị giấu mất. */
    if (!g_album_app.is_info_open) {
        g_album_app.is_info_open = TRUE;
        gtk_widget_set_visible(g_album_app.info_panel, TRUE);
    }
}

void album_show_viewer_at(int index) {
    guint total_u = g_list_length(g_album_app.filtered_list);
    if (total_u == 0) return;
    int total = (int)total_u;
    if (index < 0 || index >= total) return;

    g_album_app.current_index = index;
    MediaItem *item = (MediaItem *)g_list_nth_data(g_album_app.filtered_list, (guint)index);
    if (!item) return;

    // Reset zoom and rotation
    g_album_app.current_zoom = 1.0;
    g_album_app.current_rotation = 0;

    // Update Title
    gtk_label_set_text(GTK_LABEL(g_album_app.lbl_viewer_title), item->filename);

    if (g_album_app.btn_favorite) {
        tizen_button_set_icon_label(g_album_app.btn_favorite,
            item->is_favorite ? "starred-symbolic" : "non-starred-symbolic",
            item->is_favorite ? "Đã yêu thích" : "Yêu thích");
    }

    if (item->type == MEDIA_TYPE_PHOTO) {
        gtk_widget_set_visible(g_album_app.picture_view, TRUE);
        gtk_widget_set_visible(g_album_app.video_view, FALSE);

        GError *err = NULL;
        GdkPixbuf *pix = gdk_pixbuf_new_from_file(item->filepath, &err);
        if (pix) {
            item->width = gdk_pixbuf_get_width(pix);
            item->height = gdk_pixbuf_get_height(pix);
            gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.picture_view), pix);
            g_object_unref(pix);
        } else {
            if (err) g_error_free(err);
            gtk_picture_set_filename(GTK_PICTURE(g_album_app.picture_view), item->filepath);
        }
    } else { // Video
        /* KHÔNG chạm vào GtkVideo khi thiếu playbin3 — xem
         * video_playback_available(): GstPlay gọi g_error() và giết cả tiến
         * trình chứ không chỉ báo lỗi. */
        if (!video_playback_available()) {
            show_video_unavailable(item);
            gtk_stack_set_visible_child_name(GTK_STACK(g_album_app.stack), "viewer");
            return;
        }

        gtk_widget_set_visible(g_album_app.picture_view, FALSE);
        gtk_widget_set_visible(g_album_app.video_view, TRUE);

        GFile *gfile = g_file_new_for_path(item->filepath);
        gtk_video_set_file(GTK_VIDEO(g_album_app.video_view), gfile);
        gtk_video_set_autoplay(GTK_VIDEO(g_album_app.video_view), TRUE);
        g_object_unref(gfile);
    }

    // Format Detailed EXIF Metadata (Matching GNOME Loupe / Reference Screenshot 3)
    char mtime_str[128] = "N/A";
    if (item->mtime) {
        char *formatted_date = g_date_time_format(item->mtime, "%d.%m.%Y %H:%M:%S");
        if (formatted_date) {
            snprintf(mtime_str, sizeof(mtime_str), "%s", formatted_date);
            g_free(formatted_date);
        }
    }

    char *size_str = format_file_size(item->filesize);

    /* -------------------------------------------------------------------------
     * PHẢI escape mọi chuỗi do người dùng kiểm soát trước khi đưa vào markup.
     * gtk_label_set_markup() phân tích Pango markup, nên một file tên
     * "Ảnh A&B.jpg" hay "note<1>.png" làm hỏng cú pháp -> Pango bỏ toàn bộ
     * chuỗi và panel thông tin trống trơn. Rất dễ gặp, chỉ cần một dấu &.
     * ---------------------------------------------------------------------- */
    char *safe_path = g_markup_escape_text(item->filepath, -1);
    char *safe_dir  = g_markup_escape_text(
        g_album_app.current_dir ? g_album_app.current_dir : "—", -1);

    /* Kích thước ảnh chỉ có thật khi pixbuf đã giải mã được. Bản cũ mặc định
     * 1920x1080 khi width==0, nên MỌI video đều khai man là Full HD. Không
     * biết thì nói không biết. */
    char dim_str[64];
    if (item->width > 0 && item->height > 0) {
        snprintf(dim_str, sizeof(dim_str), "%d × %d", item->width, item->height);
    } else {
        snprintf(dim_str, sizeof(dim_str), "—");
    }

    /* Các trường EXIF (Aperture / Exposure / ISO / Maker) trước đây hardcode
     * "f/2.2", "1/33 s", "ISO 32", "TizenOS Camera HD Engine" cho MỌI tệp, kể
     * cả ảnh chụp màn hình và video. Panel thông tin mà bịa số thì tệ hơn là
     * không có panel. Đã gỡ bỏ; muốn có EXIF thật thì phải đọc bằng libexif
     * hoặc gexiv2 chứ không đoán. */
    char *info_markup = g_strdup_printf(
        "<span size='small' foreground='#a6adc8'>Sửa đổi lần cuối</span>\n"
        "<b>%s</b>\n\n"
        "<span size='small' foreground='#a6adc8'>Thư mục</span>\n"
        "<b>%s</b>\n\n"
        "<span size='small' foreground='#a6adc8'>Kích thước &amp; Định dạng</span>\n"
        "<b>%s (%s)</b>\n\n"
        "<span size='small' foreground='#a6adc8'>Dung lượng tệp</span>\n"
        "<b>%s</b>\n\n"
        "<span size='small' foreground='#a6adc8'>Đường dẫn</span>\n"
        "<small>%s</small>",
        mtime_str,
        safe_dir,
        dim_str,
        (item->type == MEDIA_TYPE_VIDEO) ? "Video" : "Ảnh",
        size_str,
        safe_path
    );

    gtk_label_set_markup(GTK_LABEL(g_album_app.lbl_info_details), info_markup);
    g_free(info_markup);
    g_free(safe_path);
    g_free(safe_dir);
    g_free(size_str);
    gtk_stack_set_visible_child_name(GTK_STACK(g_album_app.stack), "viewer");
}

void album_next_media(void) {
    guint total_u = g_list_length(g_album_app.filtered_list);
    if (total_u == 0) return;
    int total = (int)total_u;
    int next_idx = (g_album_app.current_index + 1) % total;
    album_show_viewer_at(next_idx);
}

void album_prev_media(void) {
    guint total_u = g_list_length(g_album_app.filtered_list);
    if (total_u == 0) return;
    int total = (int)total_u;
    int prev_idx = (g_album_app.current_index - 1 + total) % total;
    album_show_viewer_at(prev_idx);
}

void album_zoom(double factor) {
    g_album_app.current_zoom *= factor;
    if (g_album_app.current_zoom < 0.2) g_album_app.current_zoom = 0.2;
    if (g_album_app.current_zoom > 5.0) g_album_app.current_zoom = 5.0;

    MediaItem *item = (MediaItem *)g_list_nth_data(g_album_app.filtered_list, (guint)g_album_app.current_index);
    if (item && item->type == MEDIA_TYPE_PHOTO) {
        int scaled_w = (int)(item->width * g_album_app.current_zoom);
        int scaled_h = (int)(item->height * g_album_app.current_zoom);
        if (scaled_w > 50 && scaled_h > 50) {
            GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_scale(item->filepath, scaled_w, scaled_h, TRUE, NULL);
            if (pix) {
                gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.picture_view), pix);
                g_object_unref(pix);
            }
        }
    }
}

void album_zoom_fit(void) {
    g_album_app.current_zoom = 1.0;
    MediaItem *item = (MediaItem *)g_list_nth_data(g_album_app.filtered_list, (guint)g_album_app.current_index);
    if (item && item->type == MEDIA_TYPE_PHOTO) {
        gtk_picture_set_filename(GTK_PICTURE(g_album_app.picture_view), item->filepath);
    }
}

void album_rotate(int degrees) {
    g_album_app.current_rotation = (g_album_app.current_rotation + degrees + 360) % 360;
    MediaItem *item = (MediaItem *)g_list_nth_data(g_album_app.filtered_list, (guint)g_album_app.current_index);
    if (item && item->type == MEDIA_TYPE_PHOTO) {
        GdkPixbuf *pix = gdk_pixbuf_new_from_file(item->filepath, NULL);
        if (pix) {
            GdkPixbufRotation rot = GDK_PIXBUF_ROTATE_NONE;
            if (g_album_app.current_rotation == 90) rot = GDK_PIXBUF_ROTATE_CLOCKWISE;
            else if (g_album_app.current_rotation == 180) rot = GDK_PIXBUF_ROTATE_UPSIDEDOWN;
            else if (g_album_app.current_rotation == 270) rot = GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE;

            GdkPixbuf *rotated = gdk_pixbuf_rotate_simple(pix, rot);
            if (rotated) {
                gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.picture_view), rotated);
                g_object_unref(rotated);
            }
            g_object_unref(pix);
        }
    }
}

static gboolean slideshow_timer_cb(gpointer user_data) {
    (void)user_data;
    if (g_album_app.is_slideshow) {
        album_next_media();
        return G_SOURCE_CONTINUE;
    }
    /* Nguồn tự huỷ ở đây, nên PHẢI xoá id đang lưu. Bản cũ giữ nguyên id đã
     * chết; lần bật slideshow sau thấy id != 0 nên không tạo timer mới (slideshow
     * đứng im), còn g_source_remove() trên id đã chết thì in critical warning —
     * hoặc tệ hơn, gỡ nhầm một nguồn khác vừa tái sử dụng đúng id đó. */
    g_album_app.slideshow_timer_id = 0;
    return G_SOURCE_REMOVE;
}

void album_toggle_slideshow(void) {
    g_album_app.is_slideshow = !g_album_app.is_slideshow;
    if (g_album_app.is_slideshow) {
        if (g_album_app.btn_slideshow) {
            tizen_button_set_icon_label(g_album_app.btn_slideshow,
                                        "media-playback-pause-symbolic", "Dừng slideshow");
        }
        if (g_album_app.slideshow_timer_id == 0) {
            g_album_app.slideshow_timer_id = g_timeout_add(3000, slideshow_timer_cb, NULL);
        }
    } else {
        if (g_album_app.btn_slideshow) {
            tizen_button_set_icon_label(g_album_app.btn_slideshow,
                                        "media-playback-start-symbolic", "Slideshow");
        }
        if (g_album_app.slideshow_timer_id > 0) {
            g_source_remove(g_album_app.slideshow_timer_id);
            g_album_app.slideshow_timer_id = 0;
        }
    }
}

void album_toggle_favorite(void) {
    MediaItem *item = (MediaItem *)g_list_nth_data(g_album_app.filtered_list, (guint)g_album_app.current_index);
    if (item) {
        item->is_favorite = !item->is_favorite;
        if (g_album_app.btn_favorite) {
            tizen_button_set_icon_label(g_album_app.btn_favorite,
            item->is_favorite ? "starred-symbolic" : "non-starred-symbolic",
            item->is_favorite ? "Đã yêu thích" : "Yêu thích");
        }
    }
}

void album_delete_current(void) {
    guint total_u = g_list_length(g_album_app.filtered_list);
    if (total_u == 0) return;

    MediaItem *item = (MediaItem *)g_list_nth_data(g_album_app.filtered_list, (guint)g_album_app.current_index);
    if (!item) return;

    int idx = g_album_app.current_index;

    /* Bản cũ chỉ g_list_remove khỏi hai danh sách rồi bỏ mặc MediaItem —
     * filepath, filename và GDateTime rò rỉ mỗi lần xoá. album_remove_item()
     * gỡ khỏi cả chỉ mục băm lẫn hai danh sách rồi giải phóng đúng cách. */
    album_remove_item(item);

    guint remaining_u = g_list_length(g_album_app.filtered_list);
    if (remaining_u == 0) {
        album_show_grid();
        return;
    }

    /* Bản cũ dùng `current_index % remaining`: xoá ảnh CUỐI (idx = n-1) cho ra
     * (n-1) % (n-1) = 0 và nhảy về ảnh đầu tiên. Đúng ra phải lùi lại một bước
     * và ở nguyên chỗ cũ. */
    if (idx >= (int)remaining_u) idx = (int)remaining_u - 1;
    album_show_viewer_at(idx);
}

// Editor / Crop Functions (Matching Screenshot 2)
void album_show_editor(void) {
    MediaItem *item = (MediaItem *)g_list_nth_data(g_album_app.filtered_list, (guint)g_album_app.current_index);
    if (!item || item->type != MEDIA_TYPE_PHOTO) return;

    if (g_album_app.working_pixbuf) {
        g_object_unref(g_album_app.working_pixbuf);
        g_album_app.working_pixbuf = NULL;
    }

    g_album_app.working_pixbuf = gdk_pixbuf_new_from_file(item->filepath, NULL);
    if (!g_album_app.working_pixbuf) return;

    g_album_app.edit_rotation = 0;
    g_album_app.flip_h = FALSE;
    g_album_app.flip_v = FALSE;
    g_album_app.selected_aspect = ASPECT_FREE;

    gtk_label_set_text(GTK_LABEL(g_album_app.lbl_edit_title), item->filename);
    gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.edit_picture_view), g_album_app.working_pixbuf);
    gtk_stack_set_visible_child_name(GTK_STACK(g_album_app.stack), "editor");
}

void album_editor_rotate(int degrees) {
    if (!g_album_app.working_pixbuf) return;
    GdkPixbufRotation rot = (degrees == -90) ? GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE : GDK_PIXBUF_ROTATE_CLOCKWISE;
    GdkPixbuf *new_pix = gdk_pixbuf_rotate_simple(g_album_app.working_pixbuf, rot);
    if (new_pix) {
        g_object_unref(g_album_app.working_pixbuf);
        g_album_app.working_pixbuf = new_pix;
        gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.edit_picture_view), g_album_app.working_pixbuf);
    }
}

void album_editor_flip_h(void) {
    if (!g_album_app.working_pixbuf) return;
    GdkPixbuf *flipped = gdk_pixbuf_flip(g_album_app.working_pixbuf, TRUE);
    if (flipped) {
        g_object_unref(g_album_app.working_pixbuf);
        g_album_app.working_pixbuf = flipped;
        gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.edit_picture_view), g_album_app.working_pixbuf);
    }
}

void album_editor_flip_v(void) {
    if (!g_album_app.working_pixbuf) return;
    GdkPixbuf *flipped = gdk_pixbuf_flip(g_album_app.working_pixbuf, FALSE);
    if (flipped) {
        g_object_unref(g_album_app.working_pixbuf);
        g_album_app.working_pixbuf = flipped;
        gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.edit_picture_view), g_album_app.working_pixbuf);
    }
}

void album_editor_set_aspect(AspectRatioType aspect) {
    g_album_app.selected_aspect = aspect;
    if (!g_album_app.working_pixbuf) return;

    int w = gdk_pixbuf_get_width(g_album_app.working_pixbuf);
    int h = gdk_pixbuf_get_height(g_album_app.working_pixbuf);
    int target_w = w;
    int target_h = h;

    if (aspect == ASPECT_SQUARE) {
        target_w = target_h = MIN(w, h);
    } else if (aspect == ASPECT_16_9) {
        target_w = w;
        target_h = (w * 9) / 16;
        if (target_h > h) { target_h = h; target_w = (h * 16) / 9; }
    } else if (aspect == ASPECT_4_3) {
        target_w = w;
        target_h = (w * 3) / 4;
        if (target_h > h) { target_h = h; target_w = (h * 4) / 3; }
    } else if (aspect == ASPECT_3_2) {
        target_w = w;
        target_h = (w * 2) / 3;
        if (target_h > h) { target_h = h; target_w = (h * 3) / 2; }
    } else if (aspect == ASPECT_5_4) {
        target_w = w;
        target_h = (w * 4) / 5;
        if (target_h > h) { target_h = h; target_w = (h * 5) / 4; }
    }

    int src_x = (w - target_w) / 2;
    int src_y = (h - target_h) / 2;

    if (target_w > 0 && target_h > 0 && src_x >= 0 && src_y >= 0) {
        GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf(g_album_app.working_pixbuf, src_x, src_y, target_w, target_h);
        if (sub) {
            GdkPixbuf *copy = gdk_pixbuf_copy(sub);
            g_object_unref(sub);
            g_object_unref(g_album_app.working_pixbuf);
            g_album_app.working_pixbuf = copy;
            gtk_picture_set_pixbuf(GTK_PICTURE(g_album_app.edit_picture_view), g_album_app.working_pixbuf);
        }
    }
}

/* Chọn định dạng ghi theo ĐUÔI FILE, không ép cứng JPEG.
 * Trả về tên định dạng của gdk-pixbuf, hoặc NULL nếu không ghi được kiểu đó. */
static const char *pixbuf_format_for_path(const char *path) {
    char *lower = g_utf8_strdown(path, -1);
    const char *fmt = NULL;

    if (g_str_has_suffix(lower, ".png"))                                  fmt = "png";
    else if (g_str_has_suffix(lower, ".jpg") || g_str_has_suffix(lower, ".jpeg")) fmt = "jpeg";
    else if (g_str_has_suffix(lower, ".bmp"))                             fmt = "bmp";
    else if (g_str_has_suffix(lower, ".tif") || g_str_has_suffix(lower, ".tiff"))  fmt = "tiff";
    /* gdk-pixbuf ĐỌC được webp/gif/svg/heic nhưng không GHI được -> NULL */

    g_free(lower);
    return fmt;
}

void album_editor_save(gboolean save_as) {
    MediaItem *item = (MediaItem *)g_list_nth_data(g_album_app.filtered_list, (guint)g_album_app.current_index);
    if (!item || !g_album_app.working_pixbuf) return;

    /* --- Xác định đường dẫn đích --------------------------------------------
     * Bản cũ nối "_edited.jpg" vào SAU cả phần mở rộng: "anh.png" ->
     * "anh.png_edited.jpg". Giờ thay đúng phần mở rộng. */
    char *target_path = NULL;
    if (save_as) {
        char *dot = strrchr(item->filepath, '.');
        char *stem = dot ? g_strndup(item->filepath, (gsize)(dot - item->filepath))
                         : g_strdup(item->filepath);
        const char *ext = dot ? dot : ".jpg";
        target_path = g_strdup_printf("%s_edited%s", stem, ext);
        g_free(stem);
    } else {
        target_path = g_strdup(item->filepath);
    }

    /* --- Chọn định dạng ------------------------------------------------------
     * LỖI CŨ NGHIÊM TRỌNG: luôn ghi "jpeg" bất kể đuôi file. Lưu đè một ảnh
     * .png sẽ ghi dữ liệu JPEG vào file vẫn mang tên .png — ảnh gốc của người
     * dùng bị thay bằng bản nén mất dữ liệu, sai cả định dạng, và mọi kênh alpha
     * bị xoá trắng. Đây là phá dữ liệu, không phải lỗi hiển thị. */
    const char *fmt = pixbuf_format_for_path(target_path);
    if (!fmt) {
        /* Không ghi được định dạng này -> lưu cạnh bên dưới dạng PNG không mất
         * dữ liệu, TUYỆT ĐỐI không đụng vào file gốc. */
        char *dot  = strrchr(target_path, '.');
        char *stem = dot ? g_strndup(target_path, (gsize)(dot - target_path))
                         : g_strdup(target_path);
        char *png_path = g_strdup_printf("%s_edited.png", stem);
        g_free(stem);
        g_free(target_path);
        target_path = png_path;
        fmt = "png";
    }

    GError *err = NULL;
    gboolean ok;
    if (g_strcmp0(fmt, "jpeg") == 0) {
        ok = gdk_pixbuf_save(g_album_app.working_pixbuf, target_path, fmt, &err,
                             "quality", "95", NULL);
    } else {
        ok = gdk_pixbuf_save(g_album_app.working_pixbuf, target_path, fmt, &err, NULL);
    }

    if (ok) {
        album_add_file(target_path);   /* no-op nếu ghi đè file đã có trong list */
    } else {
        /* Bản cũ truyền NULL cho GError và bỏ qua giá trị trả về: lưu thất bại
         * thì editor vẫn đóng như thể đã lưu xong. Người dùng mất sạch chỉnh
         * sửa mà không hề được báo. */
        g_warning("[ALBUM] Lưu thất bại '%s': %s",
                  target_path, err ? err->message : "lỗi không rõ");

        GtkWidget *dlg = gtk_message_dialog_new(
            GTK_WINDOW(g_album_app.window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            "Không lưu được ảnh");
        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(dlg), "%s\n\n%s",
            target_path, err ? err->message : "Lỗi không xác định");
        g_signal_connect(dlg, "response", G_CALLBACK(on_dialog_dismiss), NULL);
        gtk_window_present(GTK_WINDOW(dlg));
    }

    g_clear_error(&err);
    g_free(target_path);   /* bản cũ chỉ free trong nhánh thành công -> rò rỉ khi lỗi */

    /* Chỉ rời editor khi đã lưu được; lưu lỗi thì giữ nguyên để người dùng thử lại */
    if (ok) album_show_viewer_at(g_album_app.current_index);
}

void album_editor_cancel(void) {
    album_show_viewer_at(g_album_app.current_index);
}
