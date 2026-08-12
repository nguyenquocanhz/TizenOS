#ifndef TIZEN_ALBUM_H
#define TIZEN_ALBUM_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef enum {
    MEDIA_TYPE_PHOTO,
    MEDIA_TYPE_VIDEO
} MediaType;

typedef struct {
    char *filepath;
    char *filename;
    MediaType type;
    goffset filesize;
    GDateTime *mtime;
    int width;
    int height;
    gboolean is_favorite;

    /* Ảnh thu nhỏ nạp nền — xem album_refresh_grid(). NULL cho tới khi thread
     * giải mã xong. Thuộc sở hữu của MediaItem. */
    GdkPixbuf *thumbnail;
    gboolean thumb_requested;
} MediaItem;

typedef enum {
    FILTER_ALL,
    FILTER_PHOTOS,
    FILTER_VIDEOS,
    FILTER_FAVORITES
} FilterType;

typedef enum {
    ASPECT_FREE,
    ASPECT_ORIGINAL,
    ASPECT_SQUARE,
    ASPECT_5_4,
    ASPECT_4_3,
    ASPECT_3_2,
    ASPECT_16_9
} AspectRatioType;

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;           // Page 0: Grid, Page 1: Viewer, Page 2: Edit/Crop
    GtkWidget *flowbox;         // Grid thumbnails
    GtkWidget *sidebar_list;
    GtkWidget *search_entry;

    // Viewer Widgets
    GtkWidget *picture_view;
    GtkWidget *video_view;
    GtkWidget *lbl_viewer_title;
    GtkWidget *lbl_info_details;
    GtkWidget *info_panel;
    GtkWidget *btn_info_toggle;
    GtkWidget *btn_favorite;
    GtkWidget *btn_slideshow;
    GtkWidget *overlay_controls_left;
    GtkWidget *overlay_controls_center;

    // Edit / Crop Widgets
    GtkWidget *edit_picture_view;
    GtkWidget *lbl_edit_title;
    GdkPixbuf *working_pixbuf;
    AspectRatioType selected_aspect;
    int edit_rotation;
    gboolean flip_h;
    gboolean flip_v;

    // Data State
    GList *media_list;          // List of MediaItem*
    GList *filtered_list;       // Filtered GList of MediaItem*
    int current_index;
    FilterType current_filter;
    char *current_dir;
    char *search_query;

    // View Adjustments
    double current_zoom;
    int current_rotation;       // 0, 90, 180, 270 degrees
    gboolean is_slideshow;
    gboolean is_fullscreen;
    gboolean is_info_open;
    guint slideshow_timer_id;
} AlbumApp;

// App Instance
extern AlbumApp g_album_app;

// Function Prototypes
void album_scan_directory(const char *dirpath);
/* Quét NHIỀU thư mục vào cùng một thư viện.
 * album_scan_directory() có ngữ nghĩa THAY THẾ (xoá thư viện cũ rồi quét), nên
 * gọi nó ba lần liên tiếp lúc khởi động khiến hai lần đầu bị xoá sạch. Dùng hàm
 * này khi cần gộp Pictures + Videos + Backgrounds vào một thư viện. */
void album_scan_roots(const char *const *dirs, guint n_dirs);
/* Nạp các tệp chỉ định (đối số dòng lệnh / .desktop %U). Quét kèm thư mục cha
 * để nút Trước/Sau vẫn duyệt được cả album. Trả về chỉ số của tệp đầu tiên
 * trong filtered_list, hoặc -1 nếu không nạp được gì. */
int  album_load_files(GFile **files, int n_files);
void album_add_file(const char *filepath);
void album_clear_media(void);
void album_cancel_thumbnails(void);
void album_remove_item(MediaItem *item);
void free_media_item(gpointer data);
void album_refresh_grid(void);
void album_show_viewer_at(int index);
void album_show_grid(void);
void album_show_editor(void);
void album_next_media(void);
void album_prev_media(void);
void album_zoom(double factor);
void album_zoom_fit(void);
void album_rotate(int degrees);
void album_toggle_slideshow(void);
void album_toggle_favorite(void);
void album_toggle_info(void);
void album_toggle_fullscreen(void);
void album_delete_current(void);
void album_apply_filter(FilterType filter);

// Editor Functions
void album_editor_rotate(int degrees);
void album_editor_flip_h(void);
void album_editor_flip_v(void);
void album_editor_set_aspect(AspectRatioType aspect);
void album_editor_save(gboolean save_as);
void album_editor_cancel(void);

#endif // TIZEN_ALBUM_H
