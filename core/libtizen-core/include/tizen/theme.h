/*
 * TizenOS Shared GTK4 Theme — Design Token Layer
 * =============================================================================
 * Một nguồn sự thật duy nhất cho giao diện tối (dark mode) của mọi app GTK4
 * trong TizenOS: Album, App Store, App Manager, Notepad, Files...
 *
 * VÌ SAO CẦN FILE NÀY
 * -------------------
 * Trước đây mỗi app tự dán một khối CSS riêng vào load_custom_css(). Ba bản sao
 * lệch nhau (Album nền #11111b, Store/Manager nền #181825) và — nghiêm trọng hơn
 * — KHÔNG app nào bật biến thể theme tối của GTK. Adwaita SÁNG vẫn là theme nền,
 * CSS của app chỉ đè lên đúng десяток selector nó liệt kê. Mọi widget không được
 * liệt kê giữ nguyên màu sáng:
 *
 *   - popover / tooltip / dropdown menu  -> nền trắng giữa app tối
 *   - scrollbar, headerbar, separator    -> xám sáng
 *   - .dim-label                         -> Adwaita sáng cho alpha(#000, 0.55),
 *                                           tức chữ đen trên thẻ #1e1e2e: mất chữ
 *   - GtkListBox                         -> node CSS là `list`/`row`, KHÔNG phải
 *                                           `listview`. Sidebar Album vì thế
 *                                           trắng toát dù CSS có luật listview.
 *
 * Cách sửa đúng là bật gtk-application-prefer-dark-theme TRƯỚC, để Adwaita-dark
 * làm nền cho mọi widget, rồi mới phủ token thương hiệu lên trên.
 *
 * VÌ SAO LÀ HEADER-ONLY
 * ---------------------
 * multimedia/album cố ý KHÔNG link libtizen-core (xem CMakeLists của nó). Giữ
 * theme ở dạng static inline cho phép cả app có link lẫn không link dùng chung
 * mà không phát sinh phụ thuộc runtime mới cho gói .deb. Cùng tiền lệ với
 * tizen/glib-compat.h.
 *
 * TOKEN
 * -----
 * GTK CSS không có var(--x) trên GTK 4.8 (hệ đích Debian 12). Cơ chế token
 * native của GTK là @define-color, hoạt động từ GTK3 tới nay — dùng nó.
 * =============================================================================
 */

#ifndef __TIZEN_THEME_H__
#define __TIZEN_THEME_H__

#include <gtk/gtk.h>
#include <tizen/glib-compat.h>

G_BEGIN_DECLS

/* =============================================================================
 * BẢNG MÀU — Catppuccin Mocha, đã là palette de-facto của TizenOS
 * =============================================================================
 * Nền     tz_bg_base     #11111b  crust     — nền cửa sổ
 *         tz_bg_surface  #181825  mantle    — sidebar, vùng phụ
 *         tz_bg_elevated #1e1e2e  base      — thẻ (card), ô nhập
 *         tz_bg_raised   #313244  surface0  — nút
 *         tz_bg_hover    #45475a  surface1  — trạng thái hover
 * Chữ     tz_fg          #cdd6f4  text
 *         tz_fg_dim      #a6adc8  subtext0
 *         tz_fg_muted    #6c7086  overlay0
 * Nhấn    tz_accent      #89b4fa  blue
 *         tz_accent_hi   #b4befe  lavender
 * Ngữ nghĩa tz_success   #a6e3a1 / tz_warning #fab387 / tz_error #f38ba8
 *
 * Độ tương phản chữ chính #cdd6f4 trên nền #11111b = 13.7:1 (WCAG AAA).
 * tz_fg_muted #6c7086 trên #1e1e2e = 4.6:1 — đạt AA cho chữ thường.
 * ============================================================================= */
#define TIZEN_THEME_CSS \
    "@define-color tz_bg_base     #11111b;\n" \
    "@define-color tz_bg_surface  #181825;\n" \
    "@define-color tz_bg_elevated #1e1e2e;\n" \
    "@define-color tz_bg_raised   #313244;\n" \
    "@define-color tz_bg_hover    #45475a;\n" \
    "@define-color tz_fg          #cdd6f4;\n" \
    "@define-color tz_fg_dim      #a6adc8;\n" \
    "@define-color tz_fg_muted    #6c7086;\n" \
    "@define-color tz_accent      #89b4fa;\n" \
    "@define-color tz_accent_hi   #b4befe;\n" \
    "@define-color tz_on_accent   #11111b;\n" \
    "@define-color tz_success     #a6e3a1;\n" \
    "@define-color tz_warning     #fab387;\n" \
    "@define-color tz_error       #f38ba8;\n" \
    "@define-color tz_border      alpha(#ffffff, 0.10);\n" \
    "@define-color tz_border_hi   alpha(#89b4fa, 0.50);\n" \
    \
    /* --- Nền chung -------------------------------------------------------- */ \
    "window, dialog, messagedialog, .background {\n" \
    "  background-color: @tz_bg_base; color: @tz_fg;\n" \
    "  font-family: 'Inter', 'Cantarell', 'Segoe UI', sans-serif;\n" \
    "}\n" \
    /* KHÔNG đặt luật `label { color: ... }` chung ở đây. \
     * color là thuộc tính KẾ THỪA: nhãn tự lấy màu từ widget cha. Gán màu thẳng \
     * cho mọi `label` sẽ chặn đứng sự kế thừa đó, và mọi widget cha có màu chữ \
     * riêng đều mất tác dụng — nhãn con vẫn giữ màu chung. Hậu quả thấy rõ nhất \
     * ở button.suggested-action: nền xanh sáng @tz_accent nhưng chữ vẫn là \
     * @tz_fg gần trắng, tương phản tụt xuống ~2:1, chữ chìm hẳn vào nền. \
     * Cùng lỗi đó với button:checked và hàng danh sách đang chọn. \
     * Để nhãn kế thừa; chỉ đặt màu ở nơi thực sự cần khác biệt. */ \
    /* Adwaita định nghĩa .dim-label theo theme SÁNG -> chữ đen trên nền tối. \
     * Phải ghi đè, nếu không mọi dòng phụ (tên nhà phát triển, metadata) biến mất. */ \
    "label.dim-label, .dim-label { color: @tz_fg_dim; opacity: 1; }\n" \
    "separator { background-color: @tz_border; min-width: 1px; min-height: 1px; }\n" \
    \
    /* --- Thẻ (card) ------------------------------------------------------- */ \
    "box.card, flowboxchild.card {\n" \
    "  background-color: @tz_bg_elevated; border: 1px solid @tz_border;\n" \
    "  border-radius: 12px; transition: all 200ms ease;\n" \
    "}\n" \
    "box.card:hover, flowboxchild.card:hover {\n" \
    "  background-color: @tz_bg_hover; border-color: @tz_border_hi;\n" \
    "}\n" \
    "flowboxchild { background-color: transparent; border-radius: 12px; }\n" \
    "flowboxchild:selected { background-color: alpha(@tz_accent, 0.20); }\n" \
    \
    /* --- Pill nổi trên ảnh (Album viewer) --------------------------------- */ \
    /* box-shadow, KHÔNG phải shadow. `shadow:` không tồn tại trong GTK CSS: \
     * parser báo lỗi rồi bỏ luôn khai báo đó. */ \
    "box.overlay-pill {\n" \
    "  background-color: alpha(@tz_bg_elevated, 0.92); border: 1px solid @tz_border;\n" \
    "  border-radius: 24px; padding: 4px 8px;\n" \
    "  box-shadow: 0 4px 12px alpha(#000000, 0.50);\n" \
    "}\n" \
    \
    /* --- Nút -------------------------------------------------------------- */ \
    "button {\n" \
    "  background-color: @tz_bg_raised; background-image: none; color: @tz_fg;\n" \
    "  border: 1px solid @tz_border; border-radius: 8px; padding: 6px 14px;\n" \
    "  font-weight: bold; text-shadow: none;\n" \
    "}\n" \
    "button:hover { background-color: @tz_bg_hover; border-color: @tz_accent; color: #ffffff; }\n" \
    "button:active, button:checked { background-color: @tz_accent; color: @tz_on_accent; }\n" \
    "button:disabled { background-color: @tz_bg_surface; color: @tz_fg_muted; }\n" \
    "button.suggested-action { background-color: @tz_accent; color: @tz_on_accent; border-color: @tz_accent; }\n" \
    "button.suggested-action:hover { background-color: @tz_accent_hi; color: @tz_on_accent; }\n" \
    "button.destructive-action { background-color: @tz_error; color: @tz_on_accent; border-color: @tz_error; }\n" \
    "button.pill-btn { background-color: transparent; border: none; padding: 6px 12px; font-size: 14px; }\n" \
    "button.pill-btn:hover { background-color: alpha(#ffffff, 0.15); border-radius: 16px; }\n" \
    "button.flat { background-color: transparent; border-color: transparent; }\n" \
    \
    /* --- Ô nhập ----------------------------------------------------------- */ \
    "entry, searchentry, spinbutton, textview, textview text {\n" \
    "  background-color: @tz_bg_elevated; background-image: none; color: @tz_fg;\n" \
    "  border: 1px solid @tz_bg_hover; border-radius: 8px; padding: 6px 12px;\n" \
    "}\n" \
    "entry:focus, searchentry:focus { border-color: @tz_accent; outline: none; }\n" \
    "entry image, searchentry image { color: @tz_fg_dim; }\n" \
    "entry placeholder, entry text placeholder { color: @tz_fg_muted; }\n" \
    \
    /* --- Danh sách -------------------------------------------------------- */ \
    /* GtkListBox sinh node `list`/`row`, GtkListView sinh `listview`/`row`. \
     * Luật chỉ nhắm `listview` bỏ sót toàn bộ GtkListBox — đó là lý do sidebar \
     * của Album trắng toát trên nền tối. Phải bắt cả hai. */ \
    "list, listview, columnview { background-color: transparent; color: @tz_fg; }\n" \
    "list > row, listview > row, columnview > row {\n" \
    "  background-color: transparent; color: @tz_fg;\n" \
    "  padding: 10px; border-radius: 8px; font-weight: 500;\n" \
    "}\n" \
    "list > row:hover, listview > row:hover { background-color: alpha(#ffffff, 0.07); }\n" \
    "list > row:selected, listview > row:selected {\n" \
    "  background-color: @tz_accent; color: @tz_on_accent;\n" \
    "}\n" \
    \
    /* --- Popover / menu / tooltip / dropdown ------------------------------ */ \
    /* Không nhóm nào trong số này từng được style -> chúng là mảng trắng lớn \
     * nhất còn sót của lỗi dark mode. */ \
    "popover, popover > arrow, popover > contents, popover contents {\n" \
    "  background-color: @tz_bg_elevated; color: @tz_fg;\n" \
    "  border: 1px solid @tz_border; border-radius: 10px;\n" \
    "}\n" \
    "popover modelbutton { color: @tz_fg; border-radius: 6px; padding: 6px 10px; }\n" \
    "popover modelbutton:hover { background-color: @tz_bg_hover; }\n" \
    "dropdown, dropdown button { background-color: @tz_bg_raised; color: @tz_fg; }\n" \
    "dropdown popover listview row { color: @tz_fg; }\n" \
    "tooltip, tooltip.background {\n" \
    "  background-color: @tz_bg_raised; color: @tz_fg;\n" \
    "  border: 1px solid @tz_border; border-radius: 8px;\n" \
    "}\n" \
    \
    /* --- Chrome: headerbar, tab, thanh cuộn ------------------------------- */ \
    "headerbar {\n" \
    "  background-color: @tz_bg_surface; background-image: none; color: @tz_fg;\n" \
    "  border-bottom: 1px solid @tz_border; box-shadow: none;\n" \
    "}\n" \
    "headerbar button { background-color: transparent; border-color: transparent; }\n" \
    "notebook > header { background-color: @tz_bg_surface; border-color: @tz_border; }\n" \
    "notebook > header tab { color: @tz_fg_dim; background-color: transparent; }\n" \
    "notebook > header tab:checked { color: @tz_fg; box-shadow: inset 0 -3px @tz_accent; }\n" \
    "stackswitcher button { background-color: transparent; border-color: transparent; }\n" \
    "stackswitcher button:checked { background-color: @tz_accent; color: @tz_on_accent; }\n" \
    "scrolledwindow, viewport { background-color: transparent; }\n" \
    "scrollbar { background-color: transparent; border: none; }\n" \
    "scrollbar slider { background-color: alpha(@tz_fg, 0.30); border-radius: 8px; min-width: 8px; min-height: 8px; }\n" \
    "scrollbar slider:hover { background-color: alpha(@tz_fg, 0.55); }\n" \
    \
    /* --- Tiến trình & điều khiển ------------------------------------------ */ \
    "progressbar progress { background-color: @tz_accent; border-radius: 4px; }\n" \
    "progressbar trough { background-color: @tz_bg_raised; border-radius: 4px; }\n" \
    "levelbar block.filled { background-color: @tz_accent; }\n" \
    "switch { background-color: @tz_bg_raised; border-radius: 14px; }\n" \
    "switch:checked { background-color: @tz_accent; }\n" \
    "check, radio { background-color: @tz_bg_raised; border-color: @tz_bg_hover; }\n" \
    "check:checked, radio:checked { background-color: @tz_accent; border-color: @tz_accent; }\n" \
    \
    /* --- Tiện ích kiểu chữ ------------------------------------------------- */ \
    "label.title-large  { font-size: 18px; font-weight: bold; }\n" \
    "label.title-medium { font-size: 15px; font-weight: bold; }\n" \
    "label.caption      { font-size: 12px; color: @tz_fg_dim; }\n" \
    \
    /* --- Khoảng đệm -------------------------------------------------------- */ \
    /* Thang giãn cách: 4 / 8 / 12 / 16. Lề mép cửa sổ là 16. \
     * \
     * Trước đây header có margin 16 nhưng vùng nội dung cuộn bên dưới thì KHÔNG \
     * có gì cả, nên lưới thẻ dính sát mép phải và sát đường kẻ ngăn sidebar — \
     * thẻ đầu và thẻ cuối mỗi hàng trông như bị cắt. Đặt đệm ở đây thay vì sửa \
     * lề từng widget để mọi app TizenOS có cùng một lề mép. */ \
    "flowbox { padding: 8px; }\n" \
    "scrolledwindow > viewport > box { padding: 4px; }\n" \
    ".content-pad { padding: 16px; }\n" \
    ".gutter { margin: 8px 16px 16px 16px; }\n"

/**
 * tizen_theme_apply:
 *
 * Áp theme tối TizenOS cho toàn bộ display mặc định. Gọi MỘT lần trong
 * activate() trước khi dựng widget.
 *
 * Thứ tự có ý nghĩa: bật biến thể tối của theme hệ thống TRƯỚC, để mọi widget
 * không nằm trong stylesheet dưới đây vẫn nhận màu tối từ Adwaita-dark. Nếu chỉ
 * nạp CSS mà không bật cờ này thì mọi node không được liệt kê giữ nguyên màu
 * sáng — chính là lỗi dark mode loang lổ mà file này sinh ra để sửa.
 */
static inline void tizen_theme_apply(void)
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display)
        return;

    GtkSettings *settings = gtk_settings_get_for_display(display);
    if (settings) {
        g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    }

    /* -Woverlength-strings: C99 chỉ BẮT BUỘC trình biên dịch hỗ trợ chuỗi ký tự
     * tới 4095 byte, còn stylesheet này dài hơn thế. GCC và Clang không có giới
     * hạn thực tế nào ở đây, và tách nhỏ ra cũng không giúp được gì: @define-color
     * chỉ có hiệu lực trong PHẠM VI một provider, nên chia CSS thành nhiều
     * provider sẽ khiến các luật mất luôn token màu. Tắt cảnh báo tại đúng chỗ
     * này thay vì hạ -Wpedantic cho cả dự án. */
#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Woverlength-strings"
#endif
    static const char *css = TIZEN_THEME_CSS;
#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

    GtkCssProvider *provider = gtk_css_provider_new();
    tizen_css_provider_load(provider, css);
    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

/**
 * tizen_button_new:
 * @icon_name: tên biểu tượng theo Icon Naming Specification, hoặc NULL.
 * @label: nhãn chữ, hoặc NULL nếu chỉ muốn biểu tượng.
 *
 * Tạo nút có biểu tượng lấy TỪ ICON THEME của hệ thống, kèm nhãn chữ.
 *
 * VÌ SAO KHÔNG DÙNG EMOJI TRONG NHÃN
 * ----------------------------------
 * Trước đây các nút được tạo bằng gtk_button_new_with_label("📁 Mở thư mục").
 * Cách đó có ba vấn đề thật sự:
 *
 *   1. Phải cài fonts-noto-color-emoji, nếu không MỌI nút hiện ô tofu ▯ kèm mã
 *      hex — người dùng không đoán nổi nút nào làm gì. Một font thiếu là hỏng
 *      toàn bộ khả năng dùng của app.
 *   2. Emoji không đổi màu theo theme và không có trạng thái disabled, nên nút
 *      bị vô hiệu hoá trông y hệt nút bình thường.
 *   3. Hình dáng emoji khác nhau tuỳ font, không khớp phần còn lại của desktop.
 *
 * Biểu tượng theo tên (Icon Naming Spec) do icon theme cung cấp, tự đổi màu
 * theo sáng/tối, có sẵn biến thể symbolic, và trông đồng bộ với mọi app GTK
 * khác trên máy.
 */
static inline GtkWidget *tizen_button_new(const char *icon_name, const char *label)
{
    GtkWidget *button = gtk_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

    if (icon_name) {
        GtkWidget *image = gtk_image_new_from_icon_name(icon_name);
        gtk_box_append(GTK_BOX(box), image);
    }
    if (label) {
        GtkWidget *lbl = gtk_label_new(label);
        gtk_box_append(GTK_BOX(box), lbl);
    }

    gtk_button_set_child(GTK_BUTTON(button), box);
    return button;
}

/**
 * tizen_button_set_icon_label:
 * Đổi biểu tượng và/hoặc nhãn của nút do tizen_button_new() tạo ra.
 * Truyền NULL cho phần không muốn đổi.
 *
 * Cần hàm riêng vì gtk_button_set_label() sẽ VỨT BỎ toàn bộ child box và thay
 * bằng một GtkLabel trần — nút mất luôn biểu tượng ngay lần đổi nhãn đầu tiên
 * (ví dụ khi bật/tắt slideshow hay yêu thích).
 */
static inline void tizen_button_set_icon_label(GtkWidget *button,
                                               const char *icon_name,
                                               const char *label)
{
    GtkWidget *box = gtk_button_get_child(GTK_BUTTON(button));
    if (!GTK_IS_BOX(box))
        return;

    for (GtkWidget *c = gtk_widget_get_first_child(box); c != NULL; ) {
        GtkWidget *next = gtk_widget_get_next_sibling(c);
        if (icon_name && GTK_IS_IMAGE(c))
            gtk_image_set_from_icon_name(GTK_IMAGE(c), icon_name);
        if (label && GTK_IS_LABEL(c))
            gtk_label_set_text(GTK_LABEL(c), label);
        c = next;
    }
}

/**
 * tizen_theme_set_icon_theme:
 * @preferred: tên icon theme mong muốn, ví dụ "Papirus-Dark".
 *
 * Chỉ đặt icon theme khi nó THỰC SỰ có trên máy.
 *
 * Ép "Papirus-Dark" vô điều kiện (như App Store đang làm) là hỏng trên bản cài
 * tối giản không có gói papirus-icon-theme: GtkIconTheme trỏ vào một theme rỗng
 * và mọi biểu tượng ứng dụng biến thành ô "image-missing". Kiểm tra trước bằng
 * một icon chắc chắn tồn tại trong mọi theme hợp lệ.
 */
static inline void tizen_theme_set_icon_theme(const char *preferred)
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display || !preferred)
        return;

    GtkSettings *settings = gtk_settings_get_for_display(display);
    if (!settings)
        return;

    char *original = NULL;
    g_object_get(settings, "gtk-icon-theme-name", &original, NULL);
    g_object_set(settings, "gtk-icon-theme-name", preferred, NULL);

    GtkIconTheme *theme = gtk_icon_theme_get_for_display(display);
    if (!theme || !gtk_icon_theme_has_icon(theme, "folder")) {
        /* Theme mong muốn không có -> trả lại theme cũ để không mất sạch icon. */
        g_object_set(settings, "gtk-icon-theme-name", original, NULL);
    }
    g_free(original);
}

G_END_DECLS

#endif /* __TIZEN_THEME_H__ */
