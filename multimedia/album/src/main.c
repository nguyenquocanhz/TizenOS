#include "tizen-album.h"
#include <glib/gstdio.h>
#include <tizen/glib-compat.h>
#include <tizen/theme.h>

AlbumApp g_album_app = {0};

static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    (void)user_data;
    g_free(g_album_app.search_query);
    g_album_app.search_query = g_strdup(gtk_editable_get_text(GTK_EDITABLE(entry)));
    album_apply_filter(g_album_app.current_filter);
}

/* -----------------------------------------------------------------------------
 * CHỌN THƯ MỤC — vì sao dùng GtkFileChooserNative chứ không phải GtkFileDialog
 * -----------------------------------------------------------------------------
 * GtkFileDialog chỉ có từ GTK 4.10. Debian 12 (Bookworm), hệ đích của TizenOS,
 * ship GTK 4.8.3 — nên bản cũ KHÔNG BIÊN DỊCH ĐƯỢC trên chính distro của mình.
 * Đó là lý do tizen-album không chạy: binary chưa từng được tạo ra, và
 * wsl-setup-and-build.sh chỉ khuyên tắt luôn -DBUILD_MULTIMEDIA=OFF.
 *
 * GtkFileChooserNative có từ GTK 3 và vẫn dùng được suốt vòng đời GTK4, nên
 * một code path duy nhất chạy trên mọi phiên bản. Bị đánh dấu deprecated từ
 * 4.10 nhưng vẫn hoạt động — không xoá cho tới GTK5.
 * -------------------------------------------------------------------------- */
static void on_folder_response(GtkNativeDialog *native, int response, gpointer user_data) {
    (void)user_data;

    if (response == GTK_RESPONSE_ACCEPT) {
        GFile *folder = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(native));
        if (folder) {
            char *path = g_file_get_path(folder);
            if (path) {
                album_scan_directory(path);
                g_free(path);
            }
            g_object_unref(folder);
        }
    }
    g_object_unref(native);
}

static void on_open_folder_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;

    GtkFileChooserNative *native = gtk_file_chooser_native_new(
        "Chọn thư mục Ảnh & Video",
        GTK_WINDOW(g_album_app.window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Chọn", "_Huỷ");

    g_signal_connect(native, "response", G_CALLBACK(on_folder_response), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

/* -----------------------------------------------------------------------------
 * Các thư mục media mặc định.
 * -----------------------------------------------------------------------------
 * g_get_user_special_dir() TRẢ VỀ NULL khi ~/.config/user-dirs.dirs chưa tồn tại
 * — đúng tình huống của phiên Live ISO và tài khoản vừa tạo, tức là chính lần
 * chạy đầu tiên của người dùng. Bản cũ đưa thẳng NULL vào g_build_filename();
 * NULL bị hiểu là dấu kết thúc danh sách đối số nên hàm trả về chuỗi rỗng, GLib
 * in critical, rồi app đi quét thư mục "". Fallback theo tên thư mục quy ước là
 * đủ và im lặng.
 * -------------------------------------------------------------------------- */
static char *media_dir(GUserDirectory which, const char *fallback_name)
{
    const char *dir = g_get_user_special_dir(which);
    if (dir && dir[0] != '\0')
        return g_strdup(dir);

    return g_build_filename(g_get_home_dir(), fallback_name, NULL);
}

static char *pictures_dir(void) { return media_dir(G_USER_DIRECTORY_PICTURES, "Pictures"); }
static char *videos_dir(void)   { return media_dir(G_USER_DIRECTORY_VIDEOS,   "Videos");   }

/* Quét toàn bộ nguồn mặc định vào MỘT thư viện. */
static void scan_all_default_roots(void)
{
    char *pics = pictures_dir();
    char *vids = videos_dir();

    const char *roots[] = { pics, vids, "/usr/share/backgrounds" };
    album_scan_roots(roots, G_N_ELEMENTS(roots));

    g_free(pics);
    g_free(vids);
}

/* -----------------------------------------------------------------------------
 * Sidebar trộn HAI loại hành động khác hẳn nhau trong cùng một danh sách:
 *      hàng 0-3 = BỘ LỌC   (lọc lại thư viện đang có, không đọc đĩa)
 *      hàng 4-6 = NGUỒN    (nạp một thư mục khác vào thư viện)
 *
 * Bản cũ để hai loại đó dùng chung biến current_filter mà không đồng bộ, sinh ra
 * lỗi chọn thể loại rất dễ gặp:
 *
 *      1. Bấm "🎬 Video"                 -> current_filter = FILTER_VIDEOS
 *      2. Bấm "🖼️ Thư mục Ảnh"           -> nạp thư mục Ảnh rồi áp lại
 *                                           current_filter (vẫn là VIDEOS)
 *      3. Lưới TRỐNG TRƠN, dù thư mục đầy ảnh.
 *
 * Người dùng thấy hàng "Thư mục Ảnh" đang sáng highlight nhưng không có gì hiện
 * ra, và không có manh mối nào cho biết bộ lọc Video mới là thủ phạm.
 *
 * Sửa: chọn một NGUỒN luôn đặt lại bộ lọc về FILTER_ALL. Nạp thư mục mới nghĩa
 * là "cho tôi xem thư mục này", không phải "giữ nguyên bộ lọc cũ".
 * -------------------------------------------------------------------------- */
static void on_sidebar_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box; (void)user_data;
    if (!row) return;

    int index = gtk_list_box_row_get_index(row);

    switch (index) {
    /* --- Bộ lọc: chỉ lọc lại thư viện hiện có --- */
    case 1: album_apply_filter(FILTER_PHOTOS);    return;
    case 2: album_apply_filter(FILTER_VIDEOS);    return;
    case 3: album_apply_filter(FILTER_FAVORITES); return;

    /* --- Nguồn: nạp lại thư viện, luôn đặt bộ lọc về ALL --- */
    case 0:
        /* "Tất cả phương tiện" phải nạp lại MỌI nguồn, không chỉ lọc lại thư mục
         * đang mở. Bản cũ chỉ gọi album_apply_filter(FILTER_ALL) nên sau khi
         * người dùng mở một thư mục lẻ thì mục này không bao giờ đưa họ về được
         * thư viện đầy đủ nữa. */
        g_album_app.current_filter = FILTER_ALL;
        scan_all_default_roots();
        return;

    case 4: {
        g_album_app.current_filter = FILTER_ALL;
        char *path = pictures_dir();
        album_scan_directory(path);
        g_free(path);
        return;
    }
    case 5: {
        g_album_app.current_filter = FILTER_ALL;
        char *path = videos_dir();
        album_scan_directory(path);
        g_free(path);
        return;
    }
    case 6:
        g_album_app.current_filter = FILTER_ALL;
        album_scan_directory("/usr/share/backgrounds");
        return;

    default:
        return;
    }
}

static void on_zoom_in_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_zoom(1.25); }
static void on_zoom_out_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_zoom(0.8); }
static void on_zoom_fit_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_zoom_fit(); }
static void on_prev_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_prev_media(); }
static void on_next_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_next_media(); }
static void on_back_grid_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_show_grid(); }
static void on_slideshow_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_toggle_slideshow(); }
static void on_favorite_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_toggle_favorite(); }
static void on_info_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_toggle_info(); }
static void on_fullscreen_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_toggle_fullscreen(); }
static void on_crop_clicked(GtkButton *b, gpointer d) { (void)b; (void)d; album_show_editor(); }

// Editor Callback Functions
static void on_edit_rot_left(GtkButton *b, gpointer d) { (void)b; (void)d; album_editor_rotate(-90); }
static void on_edit_rot_right(GtkButton *b, gpointer d) { (void)b; (void)d; album_editor_rotate(90); }
static void on_edit_flip_h(GtkButton *b, gpointer d) { (void)b; (void)d; album_editor_flip_h(); }
static void on_edit_flip_v(GtkButton *b, gpointer d) { (void)b; (void)d; album_editor_flip_v(); }
static void on_edit_save(GtkButton *b, gpointer d) { (void)b; (void)d; album_editor_save(FALSE); }
static void on_edit_save_as(GtkButton *b, gpointer d) { (void)b; (void)d; album_editor_save(TRUE); }
static void on_edit_cancel(GtkButton *b, gpointer d) { (void)b; (void)d; album_editor_cancel(); }

static void on_aspect_btn_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AspectRatioType aspect = (AspectRatioType)GPOINTER_TO_INT(user_data);
    album_editor_set_aspect(aspect);
}

static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    (void)controller; (void)keycode; (void)state; (void)user_data;
    const char *page = gtk_stack_get_visible_child_name(GTK_STACK(g_album_app.stack));
    if (g_strcmp0(page, "viewer") == 0) {
        if (keyval == GDK_KEY_Left) { album_prev_media(); return TRUE; }
        if (keyval == GDK_KEY_Right) { album_next_media(); return TRUE; }
        if (keyval == GDK_KEY_Escape) { album_show_grid(); return TRUE; }
        if (keyval == GDK_KEY_space) { album_toggle_slideshow(); return TRUE; }
        if (keyval == GDK_KEY_Delete) { album_delete_current(); return TRUE; }
    }
    return FALSE;
}

static void build_main_window(GtkApplication *app) {
    tizen_theme_apply();

    g_album_app.window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(g_album_app.window), "Tizen Album - Thư viện Ảnh & Video");
    gtk_window_set_default_size(GTK_WINDOW(g_album_app.window), 1100, 720);

    GtkWidget *root_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // 1. Header Bar Controls (Matching Reference Screenshot 1)
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(header_box, 16);
    gtk_widget_set_margin_end(header_box, 16);
    gtk_widget_set_margin_top(header_box, 12);
    gtk_widget_set_margin_bottom(header_box, 10);

    GtkWidget *btn_fullscreen = tizen_button_new("view-fullscreen-symbolic", NULL);
    g_signal_connect(btn_fullscreen, "clicked", G_CALLBACK(on_fullscreen_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), btn_fullscreen);

    GtkWidget *app_icon = gtk_image_new_from_icon_name("multimedia-photo-viewer");
    gtk_image_set_pixel_size(GTK_IMAGE(app_icon), 24);
    gtk_box_append(GTK_BOX(header_box), app_icon);

    GtkWidget *lbl_title = gtk_label_new("Tizen Album");
    gtk_widget_add_css_class(lbl_title, "title-large");
    gtk_box_append(GTK_BOX(header_box), lbl_title);

    GtkWidget *btn_grid = tizen_button_new("view-grid-symbolic", "Grid");
    g_signal_connect(btn_grid, "clicked", G_CALLBACK(on_back_grid_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), btn_grid);

    GtkWidget *btn_open_folder = tizen_button_new("folder-open-symbolic", "Mở thư mục");
    g_signal_connect(btn_open_folder, "clicked", G_CALLBACK(on_open_folder_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), btn_open_folder);

    g_album_app.search_entry = gtk_search_entry_new();
    /* GTK 4.8 chưa có set_placeholder_text cho GtkSearchEntry — xem glib-compat.h */
    tizen_search_entry_set_placeholder(g_album_app.search_entry, "Tìm kiếm ảnh, video...");
    gtk_widget_set_hexpand(g_album_app.search_entry, TRUE);
    g_signal_connect(g_album_app.search_entry, "search-changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_append(GTK_BOX(header_box), g_album_app.search_entry);

    GtkWidget *btn_crop = tizen_button_new("edit-cut-symbolic", "Cắt/Sửa");
    g_signal_connect(btn_crop, "clicked", G_CALLBACK(on_crop_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), btn_crop);

    g_album_app.btn_info_toggle = tizen_button_new("dialog-information-symbolic", "Thông tin");
    g_signal_connect(g_album_app.btn_info_toggle, "clicked", G_CALLBACK(on_info_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), g_album_app.btn_info_toggle);

    g_album_app.btn_slideshow = tizen_button_new("media-playback-start-symbolic", "Slideshow");
    gtk_widget_add_css_class(g_album_app.btn_slideshow, "suggested-action");
    g_signal_connect(g_album_app.btn_slideshow, "clicked", G_CALLBACK(on_slideshow_clicked), NULL);
    gtk_box_append(GTK_BOX(header_box), g_album_app.btn_slideshow);

    gtk_box_append(GTK_BOX(root_vbox), header_box);

    // 2. Main Content Area (Sidebar + Stack)
    GtkWidget *content_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(content_hbox, TRUE);

    // Sidebar Navigation
    GtkWidget *sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_size_request(sidebar_box, 220, -1);
    gtk_widget_set_margin_start(sidebar_box, 12);
    gtk_widget_set_margin_end(sidebar_box, 12);
    gtk_widget_set_margin_top(sidebar_box, 8);

    GtkWidget *sidebar_lbl = gtk_label_new("<b>DANH MỤC</b>");
    gtk_label_set_use_markup(GTK_LABEL(sidebar_lbl), TRUE);
    gtk_widget_set_halign(sidebar_lbl, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(sidebar_box), sidebar_lbl);

    GtkWidget *listbox = gtk_list_box_new();
    gtk_box_append(GTK_BOX(sidebar_box), listbox);

    /* Biểu tượng lấy từ icon theme theo Icon Naming Specification, không dùng
     * emoji — xem ghi chú ở tizen_button_new() trong tizen/theme.h.
     * THỨ TỰ PHẢI KHỚP với các case trong on_sidebar_row_selected(). */
    static const struct { const char *icon; const char *label; } menu_items[] = {
        { "view-paged-symbolic",       "Tất cả phương tiện"       },
        { "image-x-generic-symbolic",  "Hình ảnh"                 },
        { "video-x-generic-symbolic",  "Video"                    },
        { "starred-symbolic",          "Yêu thích"                },
        { "folder-pictures-symbolic",  "Thư mục Ảnh (Pictures)"   },
        { "folder-videos-symbolic",    "Thư mục Video (Videos)"   },
        { "preferences-desktop-wallpaper-symbolic", "Phông nền (Backgrounds)" },
    };

    for (guint i = 0; i < G_N_ELEMENTS(menu_items); i++) {
        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_start(row_box, 8);
        gtk_widget_set_margin_end(row_box, 8);
        gtk_widget_set_margin_top(row_box, 8);
        gtk_widget_set_margin_bottom(row_box, 8);

        GtkWidget *row_icon = gtk_image_new_from_icon_name(menu_items[i].icon);
        gtk_box_append(GTK_BOX(row_box), row_icon);

        GtkWidget *row_lbl = gtk_label_new(menu_items[i].label);
        gtk_widget_set_halign(row_lbl, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(row_box), row_lbl);

        gtk_list_box_append(GTK_LIST_BOX(listbox), row_box);
    }

    g_signal_connect(listbox, "row-selected", G_CALLBACK(on_sidebar_row_selected), NULL);
    gtk_box_append(GTK_BOX(content_hbox), sidebar_box);

    // Main Stack (Page 0: Grid, Page 1: Viewer, Page 2: Editor)
    g_album_app.stack = gtk_stack_new();
    gtk_widget_set_hexpand(g_album_app.stack, TRUE);
    gtk_widget_set_vexpand(g_album_app.stack, TRUE);

    // Stack Page 0: Grid View
    GtkWidget *scroll_grid = gtk_scrolled_window_new();
    /* Lề mép — xem lớp .gutter trong tizen/theme.h. */
    gtk_widget_add_css_class(scroll_grid, "gutter");
    g_album_app.flowbox = gtk_flow_box_new();
    gtk_widget_set_valign(GTK_WIDGET(g_album_app.flowbox), GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(g_album_app.flowbox), 10);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(g_album_app.flowbox), GTK_SELECTION_NONE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_grid), g_album_app.flowbox);
    gtk_stack_add_named(GTK_STACK(g_album_app.stack), scroll_grid, "grid");

    // Stack Page 1: Media Viewer View (Overlay floating controls matching Screenshot 3)
    GtkWidget *viewer_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *viewer_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(viewer_header, 16);
    gtk_widget_set_margin_end(viewer_header, 16);
    gtk_widget_set_margin_top(viewer_header, 8);
    gtk_widget_set_margin_bottom(viewer_header, 8);

    g_album_app.lbl_viewer_title = gtk_label_new("File Name");
    gtk_widget_add_css_class(g_album_app.lbl_viewer_title, "title-medium");
    gtk_widget_set_hexpand(g_album_app.lbl_viewer_title, TRUE);
    gtk_box_append(GTK_BOX(viewer_header), g_album_app.lbl_viewer_title);

    /* Nút Yêu thích. Trước đây btn_favorite không bao giờ được TẠO: album-viewer.c
     * có đọc nó (luôn thấy NULL), on_favorite_clicked() được định nghĩa nhưng
     * không nối vào đâu — trình biên dịch báo hàm thừa. Hệ quả là mục lọc
     * "⭐ Yêu thích" ở sidebar vĩnh viễn rỗng vì không có cách nào đánh dấu. */
    g_album_app.btn_favorite = tizen_button_new("non-starred-symbolic", "Yêu thích");
    g_signal_connect(g_album_app.btn_favorite, "clicked", G_CALLBACK(on_favorite_clicked), NULL);
    gtk_box_append(GTK_BOX(viewer_header), g_album_app.btn_favorite);

    GtkWidget *btn_viewer_back = tizen_button_new("go-previous-symbolic", "Quay lại lưới");
    g_signal_connect(btn_viewer_back, "clicked", G_CALLBACK(on_back_grid_clicked), NULL);
    gtk_box_append(GTK_BOX(viewer_header), btn_viewer_back);

    gtk_box_append(GTK_BOX(viewer_vbox), viewer_header);

    GtkWidget *viewer_center_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(viewer_center_hbox, TRUE);

    // Main Overlay Image Container
    GtkWidget *media_overlay = gtk_overlay_new();
    gtk_widget_set_hexpand(media_overlay, TRUE);
    gtk_widget_set_vexpand(media_overlay, TRUE);

    g_album_app.picture_view = gtk_picture_new();
    tizen_picture_set_contain(g_album_app.picture_view);

    g_album_app.video_view = gtk_video_new();

    gtk_overlay_set_child(GTK_OVERLAY(media_overlay), g_album_app.picture_view);
    gtk_overlay_add_overlay(GTK_OVERLAY(media_overlay), g_album_app.video_view);

    // Overlay Bottom-Left Floating Pill (Previous / Next controls matching Screenshot 3)
    GtkWidget *overlay_left_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_add_css_class(overlay_left_box, "overlay-pill");
    gtk_widget_set_halign(overlay_left_box, GTK_ALIGN_START);
    gtk_widget_set_valign(overlay_left_box, GTK_ALIGN_END);
    gtk_widget_set_margin_start(overlay_left_box, 20);
    gtk_widget_set_margin_bottom(overlay_left_box, 20);

    GtkWidget *btn_overlay_prev = tizen_button_new("go-previous-symbolic", NULL);
    gtk_widget_add_css_class(btn_overlay_prev, "pill-btn");
    g_signal_connect(btn_overlay_prev, "clicked", G_CALLBACK(on_prev_clicked), NULL);
    gtk_box_append(GTK_BOX(overlay_left_box), btn_overlay_prev);

    GtkWidget *btn_overlay_next = tizen_button_new("go-next-symbolic", NULL);
    gtk_widget_add_css_class(btn_overlay_next, "pill-btn");
    g_signal_connect(btn_overlay_next, "clicked", G_CALLBACK(on_next_clicked), NULL);
    gtk_box_append(GTK_BOX(overlay_left_box), btn_overlay_next);

    gtk_overlay_add_overlay(GTK_OVERLAY(media_overlay), overlay_left_box);

    // Overlay Bottom-Center Floating Pill (Zoom - + 🔍 controls matching Screenshot 3)
    GtkWidget *overlay_center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_add_css_class(overlay_center_box, "overlay-pill");
    gtk_widget_set_halign(overlay_center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(overlay_center_box, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(overlay_center_box, 20);

    GtkWidget *btn_overlay_zoom_out = tizen_button_new("zoom-out-symbolic", NULL);
    gtk_widget_add_css_class(btn_overlay_zoom_out, "pill-btn");
    g_signal_connect(btn_overlay_zoom_out, "clicked", G_CALLBACK(on_zoom_out_clicked), NULL);
    gtk_box_append(GTK_BOX(overlay_center_box), btn_overlay_zoom_out);

    GtkWidget *btn_overlay_zoom_in = tizen_button_new("zoom-in-symbolic", NULL);
    gtk_widget_add_css_class(btn_overlay_zoom_in, "pill-btn");
    g_signal_connect(btn_overlay_zoom_in, "clicked", G_CALLBACK(on_zoom_in_clicked), NULL);
    gtk_box_append(GTK_BOX(overlay_center_box), btn_overlay_zoom_in);

    GtkWidget *btn_overlay_zoom_fit = tizen_button_new("zoom-fit-best-symbolic", NULL);
    gtk_widget_add_css_class(btn_overlay_zoom_fit, "pill-btn");
    g_signal_connect(btn_overlay_zoom_fit, "clicked", G_CALLBACK(on_zoom_fit_clicked), NULL);
    gtk_box_append(GTK_BOX(overlay_center_box), btn_overlay_zoom_fit);

    gtk_overlay_add_overlay(GTK_OVERLAY(media_overlay), overlay_center_box);

    gtk_box_append(GTK_BOX(viewer_center_hbox), media_overlay);

    // Viewer Info Side Panel (Matching Screenshot 3)
    g_album_app.info_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(g_album_app.info_panel, "card");
    gtk_widget_set_size_request(g_album_app.info_panel, 280, -1);
    gtk_widget_set_margin_start(g_album_app.info_panel, 12);
    gtk_widget_set_margin_end(g_album_app.info_panel, 12);
    gtk_widget_set_margin_top(g_album_app.info_panel, 12);
    gtk_widget_set_margin_bottom(g_album_app.info_panel, 12);

    GtkWidget *lbl_info_header = gtk_label_new("<b>Thông tin Phương tiện</b>");
    gtk_label_set_use_markup(GTK_LABEL(lbl_info_header), TRUE);
    gtk_box_append(GTK_BOX(g_album_app.info_panel), lbl_info_header);

    g_album_app.lbl_info_details = gtk_label_new("No details");
    gtk_label_set_use_markup(GTK_LABEL(g_album_app.lbl_info_details), TRUE);
    gtk_label_set_wrap(GTK_LABEL(g_album_app.lbl_info_details), TRUE);
    gtk_box_append(GTK_BOX(g_album_app.info_panel), g_album_app.lbl_info_details);

    gtk_box_append(GTK_BOX(viewer_center_hbox), g_album_app.info_panel);
    gtk_box_append(GTK_BOX(viewer_vbox), viewer_center_hbox);

    gtk_stack_add_named(GTK_STACK(g_album_app.stack), viewer_vbox, "viewer");

    // Stack Page 2: Editor / Crop View (Matching Reference Screenshot 2)
    GtkWidget *editor_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *edit_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(edit_header, 16);
    gtk_widget_set_margin_end(edit_header, 16);
    gtk_widget_set_margin_top(edit_header, 12);
    gtk_widget_set_margin_bottom(edit_header, 12);

    GtkWidget *btn_edit_cancel = tizen_button_new("process-stop-symbolic", "Hủy bỏ");
    g_signal_connect(btn_edit_cancel, "clicked", G_CALLBACK(on_edit_cancel), NULL);
    gtk_box_append(GTK_BOX(edit_header), btn_edit_cancel);

    g_album_app.lbl_edit_title = gtk_label_new("Edit Image.jpg");
    gtk_widget_add_css_class(g_album_app.lbl_edit_title, "title-medium");
    gtk_widget_set_hexpand(g_album_app.lbl_edit_title, TRUE);
    gtk_box_append(GTK_BOX(edit_header), g_album_app.lbl_edit_title);

    GtkWidget *btn_edit_save = tizen_button_new("document-save-symbolic", "Lưu lại");
    gtk_widget_add_css_class(btn_edit_save, "suggested-action");
    g_signal_connect(btn_edit_save, "clicked", G_CALLBACK(on_edit_save), NULL);
    gtk_box_append(GTK_BOX(edit_header), btn_edit_save);

    GtkWidget *btn_edit_save_as = tizen_button_new("document-save-as-symbolic", "Lưu bản sao");
    g_signal_connect(btn_edit_save_as, "clicked", G_CALLBACK(on_edit_save_as), NULL);
    gtk_box_append(GTK_BOX(edit_header), btn_edit_save_as);

    gtk_box_append(GTK_BOX(editor_vbox), edit_header);

    GtkWidget *edit_main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(edit_main_hbox, TRUE);

    g_album_app.edit_picture_view = gtk_picture_new();
    tizen_picture_set_contain(g_album_app.edit_picture_view);
    gtk_widget_set_hexpand(g_album_app.edit_picture_view, TRUE);
    gtk_box_append(GTK_BOX(edit_main_hbox), g_album_app.edit_picture_view);

    // Edit Controls Side Panel (Matching Screenshot 2)
    GtkWidget *edit_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_add_css_class(edit_panel, "card");
    gtk_widget_set_size_request(edit_panel, 300, -1);
    gtk_widget_set_margin_start(edit_panel, 12);
    gtk_widget_set_margin_end(edit_panel, 16);
    gtk_widget_set_margin_top(edit_panel, 12);
    gtk_widget_set_margin_bottom(edit_panel, 16);

    GtkWidget *lbl_aspect_header = gtk_label_new("<b>Tỷ lệ khung hình (Aspect Ratio)</b>");
    gtk_label_set_use_markup(GTK_LABEL(lbl_aspect_header), TRUE);
    gtk_widget_set_halign(lbl_aspect_header, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(edit_panel), lbl_aspect_header);

    // Grid of Aspect Ratio buttons
    GtkWidget *aspect_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(aspect_grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(aspect_grid), 8);

    struct { const char *label; AspectRatioType type; int col; int row; } aspect_btns[] = {
        { "Tùy chỉnh (Free)", ASPECT_FREE, 0, 0 },
        { "Gốc (Original)", ASPECT_ORIGINAL, 1, 0 },
        { "Vuông (1:1)", ASPECT_SQUARE, 0, 1 },
        { "5:4", ASPECT_5_4, 1, 1 },
        { "4:3", ASPECT_4_3, 0, 2 },
        { "3:2", ASPECT_3_2, 1, 2 },
        { "16:9", ASPECT_16_9, 0, 3 },
    };

    for (int i = 0; i < 7; i++) {
        GtkWidget *abtn = gtk_button_new_with_label(aspect_btns[i].label);
        g_signal_connect(abtn, "clicked", G_CALLBACK(on_aspect_btn_clicked), GINT_TO_POINTER(aspect_btns[i].type));
        gtk_grid_attach(GTK_GRID(aspect_grid), abtn, aspect_btns[i].col, aspect_btns[i].row, 1, 1);
    }
    gtk_box_append(GTK_BOX(edit_panel), aspect_grid);

    GtkWidget *lbl_rot_header = gtk_label_new("<b>Xoay hình (Rotate)</b>");
    gtk_label_set_use_markup(GTK_LABEL(lbl_rot_header), TRUE);
    gtk_widget_set_halign(lbl_rot_header, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(edit_panel), lbl_rot_header);

    GtkWidget *rot_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *btn_erot_l = tizen_button_new("object-rotate-left-symbolic", "Xoay trái 90°");
    g_signal_connect(btn_erot_l, "clicked", G_CALLBACK(on_edit_rot_left), NULL);
    gtk_box_append(GTK_BOX(rot_box), btn_erot_l);

    GtkWidget *btn_erot_r = tizen_button_new("object-rotate-right-symbolic", "Xoay phải 90°");
    g_signal_connect(btn_erot_r, "clicked", G_CALLBACK(on_edit_rot_right), NULL);
    gtk_box_append(GTK_BOX(rot_box), btn_erot_r);

    gtk_box_append(GTK_BOX(edit_panel), rot_box);

    GtkWidget *lbl_flip_header = gtk_label_new("<b>Lật hình (Flip)</b>");
    gtk_label_set_use_markup(GTK_LABEL(lbl_flip_header), TRUE);
    gtk_widget_set_halign(lbl_flip_header, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(edit_panel), lbl_flip_header);

    GtkWidget *flip_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *btn_eflip_h = tizen_button_new("object-flip-horizontal-symbolic", "Lật ngang");
    g_signal_connect(btn_eflip_h, "clicked", G_CALLBACK(on_edit_flip_h), NULL);
    gtk_box_append(GTK_BOX(flip_box), btn_eflip_h);

    GtkWidget *btn_eflip_v = tizen_button_new("object-flip-vertical-symbolic", "Lật dọc");
    g_signal_connect(btn_eflip_v, "clicked", G_CALLBACK(on_edit_flip_v), NULL);
    gtk_box_append(GTK_BOX(flip_box), btn_eflip_v);

    gtk_box_append(GTK_BOX(edit_panel), flip_box);

    gtk_box_append(GTK_BOX(edit_main_hbox), edit_panel);
    gtk_box_append(GTK_BOX(editor_vbox), edit_main_hbox);

    gtk_stack_add_named(GTK_STACK(g_album_app.stack), editor_vbox, "editor");

    gtk_box_append(GTK_BOX(content_hbox), g_album_app.stack);
    gtk_box_append(GTK_BOX(root_vbox), content_hbox);

    // Keyboard controller
    GtkEventController *key_ctrl = gtk_event_controller_key_new();
    g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), NULL);
    gtk_widget_add_controller(g_album_app.window, key_ctrl);

    gtk_window_set_child(GTK_WINDOW(g_album_app.window), root_vbox);
    gtk_window_present(GTK_WINDOW(g_album_app.window));
}

/* Dựng cửa sổ đúng một lần.
 * GApplication phát "activate" mỗi khi có tiến trình thứ hai được khởi chạy
 * (single-instance); dựng lại giao diện ở lần thứ hai sẽ bỏ rơi cửa sổ cũ. */
static void ensure_main_window(GtkApplication *app) {
    if (!g_album_app.window)
        build_main_window(app);
    else
        gtk_window_present(GTK_WINDOW(g_album_app.window));
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    ensure_main_window(app);
    scan_all_default_roots();
}

/* -----------------------------------------------------------------------------
 * MỞ TỆP TỪ BÊN NGOÀI — vì sao thiếu nó thì app "không khởi động"
 * -----------------------------------------------------------------------------
 * tizen-album.desktop khai báo:
 *      Exec=tizen-album %U
 *      MimeType=image/jpeg;image/png;...;video/mp4;...
 *
 * Nghĩa là tizen-album được đăng ký làm trình xử lý mặc định cho ảnh và video,
 * và mỗi lần người dùng bấm đúp một tấm ảnh, %U được thay bằng URI của tệp đó.
 *
 * Nhưng main() lại tạo GtkApplication với G_APPLICATION_DEFAULT_FLAGS, tức KHÔNG
 * có cờ G_APPLICATION_HANDLES_OPEN. GApplication khi nhận đối số tệp mà không
 * khai báo cờ này sẽ bỏ ngay lập tức:
 *
 *      GLib-GIO-CRITICAL: This application can not open files.
 *      exit code 1
 *
 * "activate" không bao giờ được phát, cửa sổ không bao giờ hiện. Chạy tay
 * `tizen-album` không đối số thì bình thường, nên lỗi trông như ngẫu nhiên —
 * thực ra nó xảy ra CHÍNH XÁC ở đường mà người dùng hay dùng nhất: bấm vào ảnh
 * trong trình quản lý tệp.
 *
 * Đã kiểm chứng lại sau khi sửa: xem phần build & test.
 * -------------------------------------------------------------------------- */
static void on_open(GApplication *app, GFile **files, int n_files,
                    const char *hint, gpointer user_data) {
    (void)hint; (void)user_data;

    ensure_main_window(GTK_APPLICATION(app));

    int index = album_load_files(files, n_files);
    if (index >= 0) {
        album_show_viewer_at(index);
    } else {
        /* Toàn URI không phải tệp cục bộ (smb://, http://...) hoặc định dạng
         * không đọc được -> vẫn mở app với thư viện mặc định thay vì thoát. */
        scan_all_default_roots();
    }
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.tizenos.album",
                                              G_APPLICATION_HANDLES_OPEN);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    g_signal_connect(app, "open", G_CALLBACK(on_open), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
