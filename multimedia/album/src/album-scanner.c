#include "tizen-album.h"
#include <gio/gio.h>
#include <string.h>

static const char *PHOTO_EXTS[] = {
    "png", "jpg", "jpeg", "webp", "gif",
    "bmp", "svg", "tiff", "tif", "heic",
    "avif", "ico", NULL
};

static const char *VIDEO_EXTS[] = {
    "mp4", "mkv", "avi", "mov", "webm",
    "flv", "m4v", "3gp", "ogv", "wmv",
    "ts", NULL
};

/* -----------------------------------------------------------------------------
 * Nhận dạng phần mở rộng — tra bảng băm thay vì quét tuyến tính
 * -----------------------------------------------------------------------------
 * Bản cũ gọi has_matching_ext() hai lần cho mỗi tệp (một lần cho danh sách ảnh,
 * một lần cho video). Mỗi lần lại g_utf8_strdown() TOÀN BỘ đường dẫn rồi
 * g_str_has_suffix() qua tối đa 12 hậu tố. Tức 2 lần cấp phát + tới 23 lần so
 * chuỗi trên mỗi tệp.
 *
 * Ở đây chỉ hạ chữ thường phần mở rộng (vài byte, nằm trên stack) và tra bảng
 * băm dựng một lần: O(1), không cấp phát heap.
 * -------------------------------------------------------------------------- */
static GHashTable *ext_table = NULL;   /* "jpg" -> GINT_TO_POINTER(MediaType + 1) */

static void ensure_ext_table(void)
{
    if (ext_table)
        return;

    ext_table = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; PHOTO_EXTS[i]; i++)
        g_hash_table_insert(ext_table, (gpointer)PHOTO_EXTS[i],
                            GINT_TO_POINTER(MEDIA_TYPE_PHOTO + 1));
    for (int i = 0; VIDEO_EXTS[i]; i++)
        g_hash_table_insert(ext_table, (gpointer)VIDEO_EXTS[i],
                            GINT_TO_POINTER(MEDIA_TYPE_VIDEO + 1));
}

/* Trả về TRUE và ghi *out_type nếu @filename là tệp media nhận dạng được. */
static gboolean classify_media(const char *filename, MediaType *out_type)
{
    if (!filename)
        return FALSE;

    const char *dot = strrchr(filename, '.');
    if (!dot || dot[1] == '\0')
        return FALSE;

    /* Phần mở rộng dài bất thường thì chắc chắn không phải đuôi tệp media. */
    size_t len = strlen(dot + 1);
    if (len == 0 || len >= 16)
        return FALSE;

    char ext[16];
    for (size_t i = 0; i < len; i++)
        ext[i] = (char)g_ascii_tolower(dot[1 + i]);
    ext[len] = '\0';

    ensure_ext_table();
    gpointer val = g_hash_table_lookup(ext_table, ext);
    if (!val)
        return FALSE;

    if (out_type)
        *out_type = (MediaType)(GPOINTER_TO_INT(val) - 1);
    return TRUE;
}

void free_media_item(gpointer data) {
    MediaItem *item = (MediaItem *)data;
    if (!item) return;
    g_free(item->filepath);
    g_free(item->filename);
    if (item->mtime) g_date_time_unref(item->mtime);
    if (item->thumbnail) g_object_unref(item->thumbnail);
    g_free(item);
}

/* -----------------------------------------------------------------------------
 * Chỉ mục đường dẫn để chống trùng lặp.
 * -----------------------------------------------------------------------------
 * Bản cũ duyệt tuyến tính toàn bộ media_list cho MỖI file được thêm vào — O(n²).
 * Một thư mục 5000 ảnh tốn 12,5 triệu lần so chuỗi và làm treo UI khi quét.
 * Bảng băm đưa về O(1). Bảng KHÔNG sở hữu dữ liệu: khoá là con trỏ filepath của
 * chính MediaItem, giá trị là MediaItem*. Vòng đời do media_list quyết định.
 * -------------------------------------------------------------------------- */
static GHashTable *path_index = NULL;

/* Con trỏ đuôi danh sách.
 * g_list_append() phải đi hết danh sách để tìm nút cuối -> O(n) mỗi lần thêm,
 * O(n²) cho cả lượt quét. Với 5000 ảnh là 12,5 triệu bước con trỏ chỉ để nối
 * đuôi. Giữ sẵn đuôi cho phép thêm O(1) mà vẫn giữ đúng thứ tự chèn. */
static GList *media_tail = NULL;

static void ensure_path_index(void) {
    if (!path_index) {
        path_index = g_hash_table_new(g_str_hash, g_str_equal);
    }
}

/* Giải phóng toàn bộ thư viện hiện tại.
 * THỨ TỰ QUAN TRỌNG: filtered_list chỉ MƯỢN con trỏ từ media_list, nên phải xoá
 * nó trước. Giải phóng item trước sẽ để lại con trỏ treo trong filtered_list và
 * cú refresh grid kế tiếp đọc vào bộ nhớ đã free. */
void album_clear_media(void) {
    /* Huỷ mọi tác vụ nạp thumbnail đang chạy TRƯỚC khi giải phóng MediaItem.
     * Thread nền giữ con trỏ tới item; free trước khi huỷ là use-after-free
     * kinh điển, và nó chỉ nổ khi người dùng đổi thư mục lúc lưới đang nạp. */
    album_cancel_thumbnails();

    g_list_free(g_album_app.filtered_list);
    g_album_app.filtered_list = NULL;

    /* Dọn bảng băm TRƯỚC khi giải phóng item: khoá của bảng chính là con trỏ
     * item->filepath. Giải phóng item trước sẽ để bảng ôm một loạt khoá treo. */
    if (path_index) g_hash_table_remove_all(path_index);

    g_list_free_full(g_album_app.media_list, free_media_item);
    g_album_app.media_list = NULL;
    media_tail = NULL;

    g_album_app.current_index = 0;
}

/* Chèn item đã dựng sẵn vào thư viện. Dùng chung bởi album_add_file() (đường
 * chậm, tự stat) và scan_one_directory() (đường nhanh, đã có sẵn metadata). */
static void library_insert(MediaItem *item)
{
    GList *node = g_list_alloc();
    node->data = item;
    node->prev = media_tail;
    node->next = NULL;

    if (media_tail)
        media_tail->next = node;
    else
        g_album_app.media_list = node;

    media_tail = node;
    g_hash_table_insert(path_index, item->filepath, item);
}

void album_add_file(const char *filepath) {
    if (!filepath) return;

    MediaType type;
    if (!classify_media(filepath, &type)) return;

    ensure_path_index();
    if (g_hash_table_contains(path_index, filepath)) return;   /* trùng -> bỏ qua */

    /* Một lần query_info thay cho g_file_test(EXISTS) + query_info riêng lẻ:
     * tệp không tồn tại thì query_info trả NULL, dùng luôn kết quả đó. */
    GFile *gfile = g_file_new_for_path(filepath);
    GFileInfo *info = g_file_query_info(
        gfile,
        G_FILE_ATTRIBUTE_STANDARD_SIZE "," G_FILE_ATTRIBUTE_TIME_MODIFIED,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);
    g_object_unref(gfile);

    if (!info) return;

    MediaItem *item = g_new0(MediaItem, 1);
    item->filepath = g_strdup(filepath);
    item->filename = g_path_get_basename(filepath);
    item->type = type;
    item->is_favorite = FALSE;
    item->filesize = g_file_info_get_size(info);
    /* g_file_info_get_modification_date_time() trả về THAM CHIẾU MỚI
     * (transfer full). Bản cũ ref thêm một lần nữa -> refcount = 2, trong
     * khi free_media_item chỉ unref một lần -> mỗi file quét được rò rỉ
     * vĩnh viễn một GDateTime. */
    item->mtime = g_file_info_get_modification_date_time(info);
    g_object_unref(info);

    library_insert(item);
}

/* -----------------------------------------------------------------------------
 * Quét một thư mục — một lượt liệt kê, không stat từng tệp
 * -----------------------------------------------------------------------------
 * Bản cũ dùng GDir rồi với MỖI tên tệp lại:
 *      g_file_test(IS_REGULAR)   -> 1 stat()
 *      g_file_test(EXISTS)       -> 1 stat()
 *      g_file_query_info()       -> 1 statx()
 * tức 3 lần chạm đĩa cho mỗi tệp. Trên ổ mạng hoặc HDD, một thư mục 2000 ảnh
 * tốn 6000 syscall và treo UI vài giây.
 *
 * GFileEnumerator lấy type + size + mtime + tên NGAY TRONG một lượt liệt kê
 * (kernel trả kèm khi readdir), nên còn 0 syscall phụ cho mỗi tệp.
 * -------------------------------------------------------------------------- */
static void scan_one_directory(const char *dirpath)
{
    if (!dirpath || dirpath[0] == '\0')
        return;

    GFile *dir = g_file_new_for_path(dirpath);
    GFileEnumerator *en = g_file_enumerate_children(
        dir,
        G_FILE_ATTRIBUTE_STANDARD_NAME ","
        G_FILE_ATTRIBUTE_STANDARD_TYPE ","
        G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
        G_FILE_ATTRIBUTE_STANDARD_SIZE ","
        G_FILE_ATTRIBUTE_TIME_MODIFIED,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);

    if (!en) {
        g_object_unref(dir);
        return;
    }

    ensure_path_index();

    GFileInfo *info;
    while ((info = g_file_enumerator_next_file(en, NULL, NULL)) != NULL) {
        const char *name = g_file_info_get_name(info);

        if (!name || name[0] == '.' || g_file_info_get_is_hidden(info)) {
            g_object_unref(info);
            continue;
        }
        /* Theo symlink: STANDARD_TYPE đã phân giải sẵn, nên thư mục ảnh trỏ
         * bằng symlink vẫn bị loại đúng còn tệp thật vẫn được nhận. */
        if (g_file_info_get_file_type(info) != G_FILE_TYPE_REGULAR) {
            g_object_unref(info);
            continue;
        }

        MediaType type;
        if (!classify_media(name, &type)) {
            g_object_unref(info);
            continue;
        }

        char *full_path = g_build_filename(dirpath, name, NULL);
        if (g_hash_table_contains(path_index, full_path)) {
            g_free(full_path);
            g_object_unref(info);
            continue;
        }

        MediaItem *item = g_new0(MediaItem, 1);
        item->filepath = full_path;            /* chuyển quyền sở hữu */
        item->filename = g_strdup(name);
        item->type = type;
        item->filesize = g_file_info_get_size(info);
        item->mtime = g_file_info_get_modification_date_time(info);

        library_insert(item);
        g_object_unref(info);
    }

    g_file_enumerator_close(en, NULL, NULL);
    g_object_unref(en);
    g_object_unref(dir);
}

/* Mới nhất lên đầu — thứ tự mà mọi trình xem ảnh đều dùng.
 * Bản cũ giữ nguyên thứ tự g_dir_read_name() trả về, tức thứ tự inode: với
 * người dùng là ngẫu nhiên hoàn toàn. Ảnh vừa chụp có thể nằm lọt giữa lưới. */
static gint compare_by_mtime_desc(gconstpointer a, gconstpointer b)
{
    const MediaItem *ia = (const MediaItem *)a;
    const MediaItem *ib = (const MediaItem *)b;

    if (ia->mtime && ib->mtime) {
        /* g_date_time_compare cho thứ tự tăng dần; đảo lại để mới nhất lên đầu. */
        gint cmp = g_date_time_compare(ib->mtime, ia->mtime);
        if (cmp != 0)
            return cmp;
    } else if (ia->mtime) {
        return -1;
    } else if (ib->mtime) {
        return 1;
    }

    /* Cùng mốc thời gian (ảnh chụp liên thanh, tệp giải nén) -> xếp theo tên để
     * thứ tự ổn định giữa các lần chạy. */
    return g_strcmp0(ia->filename, ib->filename);
}

static void finish_scan(void)
{
    g_album_app.media_list = g_list_sort(g_album_app.media_list, compare_by_mtime_desc);
    media_tail = g_list_last(g_album_app.media_list);
    album_apply_filter(g_album_app.current_filter);
}

void album_scan_directory(const char *dirpath) {
    if (!dirpath || !g_file_test(dirpath, G_FILE_TEST_IS_DIR)) return;

    /* THAY THẾ thư viện chứ không cộng dồn.
     * Bản cũ chỉ append: mở thư mục B sau thư mục A cho ra danh sách A+B trong
     * khi current_dir lại ghi là B — giao diện nói một đằng, dữ liệu một nẻo.
     * Kèm theo đó media_list không bao giờ được giải phóng (free_media_item
     * được định nghĩa nhưng không nơi nào gọi), nên bộ nhớ chỉ có tăng. */
    album_clear_media();

    g_free(g_album_app.current_dir);
    g_album_app.current_dir = g_strdup(dirpath);

    scan_one_directory(dirpath);
    finish_scan();
}

/* -----------------------------------------------------------------------------
 * Quét nhiều thư mục vào MỘT thư viện.
 * -----------------------------------------------------------------------------
 * Đây là lỗi khiến "Tất cả phương tiện" gần như luôn trống lúc mới mở app.
 * activate() gọi liên tiếp:
 *      album_scan_directory(Pictures);
 *      album_scan_directory(Videos);        <- xoá sạch kết quả Pictures
 *      album_scan_directory(backgrounds);   <- xoá sạch kết quả Videos
 * nên chỉ thư mục CUỐI còn lại. Máy nào không có /usr/share/backgrounds thì
 * thư viện rỗng hoàn toàn dù thư mục Ảnh đầy ắp.
 * -------------------------------------------------------------------------- */
void album_scan_roots(const char *const *dirs, guint n_dirs)
{
    if (!dirs || n_dirs == 0) return;

    album_clear_media();

    const char *first_valid = NULL;
    for (guint i = 0; i < n_dirs; i++) {
        if (!dirs[i] || dirs[i][0] == '\0') continue;
        if (!g_file_test(dirs[i], G_FILE_TEST_IS_DIR)) continue;

        if (!first_valid) first_valid = dirs[i];
        scan_one_directory(dirs[i]);
    }

    g_free(g_album_app.current_dir);
    g_album_app.current_dir = first_valid ? g_strdup(first_valid) : NULL;

    finish_scan();
}

/* -----------------------------------------------------------------------------
 * Nạp các tệp truyền qua dòng lệnh / .desktop %U.
 * -----------------------------------------------------------------------------
 * Cũng quét luôn thư mục cha của chúng: người dùng bấm đúp một tấm ảnh thì vẫn
 * muốn mũi tên Trước/Sau lướt được cả thư mục, chứ không phải một album 1 ảnh.
 * -------------------------------------------------------------------------- */
int album_load_files(GFile **files, int n_files)
{
    if (!files || n_files <= 0) return -1;

    album_clear_media();

    /* Gom thư mục cha, khử trùng lặp: mở 20 ảnh cùng một thư mục chỉ quét 1 lần. */
    GHashTable *parents = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    char *first_path = NULL;

    for (int i = 0; i < n_files; i++) {
        char *path = g_file_get_path(files[i]);
        if (!path) continue;                    /* URI không phải tệp cục bộ */

        if (!first_path) first_path = g_strdup(path);

        char *parent = g_path_get_dirname(path);
        if (!g_hash_table_contains(parents, parent)) {
            g_hash_table_add(parents, parent);  /* bảng giữ quyền sở hữu */
            scan_one_directory(parent);
        } else {
            g_free(parent);
        }

        /* Quét thư mục cha có thể đã bỏ qua tệp này (đuôi lạ, hoặc tệp ẩn).
         * Thêm thẳng để thứ người dùng yêu cầu luôn mở được. */
        album_add_file(path);
        g_free(path);
    }

    g_free(g_album_app.current_dir);
    g_album_app.current_dir = first_path ? g_path_get_dirname(first_path) : NULL;

    g_hash_table_destroy(parents);
    finish_scan();

    /* Tìm vị trí tệp đầu tiên trong danh sách ĐÃ lọc và sắp xếp. */
    int index = -1;
    if (first_path) {
        int i = 0;
        for (GList *l = g_album_app.filtered_list; l; l = l->next, i++) {
            MediaItem *item = (MediaItem *)l->data;
            if (g_strcmp0(item->filepath, first_path) == 0) {
                index = i;
                break;
            }
        }
        g_free(first_path);
    }
    return index;
}

/* Gỡ một item khỏi thư viện và giải phóng nó. Dùng bởi album_delete_current().
 * Tách ra đây để chỉ mục băm và danh sách không bao giờ lệch nhau. */
void album_remove_item(MediaItem *item) {
    if (!item) return;

    if (path_index) g_hash_table_remove(path_index, item->filepath);
    g_album_app.filtered_list = g_list_remove(g_album_app.filtered_list, item);
    g_album_app.media_list    = g_list_remove(g_album_app.media_list, item);
    /* media_list vừa đổi hình dạng -> đuôi đã lưu có thể là nút vừa bị gỡ. */
    media_tail = g_list_last(g_album_app.media_list);

    free_media_item(item);
}
