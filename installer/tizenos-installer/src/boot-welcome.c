#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

static void on_btn_installer_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    system("/usr/local/bin/tizenos-installer-gui &");
}

static void on_btn_browser_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    system("firefox-esr &");
}

static void on_btn_files_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    system("thunar &");
}

static void on_btn_terminal_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    system("xfce4-terminal &");
}

static void on_btn_close_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    GtkWindow *win = GTK_WINDOW(user_data);
    gtk_window_destroy(win);
}

static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css =
        "window { background-color: #1e1e2e; color: #cdd6f4; font-family: 'Inter', sans-serif; }\n"
        ".welcome-header { font-size: 22px; font-weight: bold; color: #89b4fa; margin-bottom: 8px; }\n"
        ".sub-header { font-size: 14px; color: #a6adc8; margin-bottom: 16px; }\n"
        ".card { background-color: #181825; border: 1px solid #313244; border-radius: 10px; padding: 16px; margin-bottom: 12px; }\n"
        ".card-title { font-size: 16px; font-weight: bold; color: #f9e2af; margin-bottom: 8px; }\n"
        "button { background: #313244; color: #cdd6f4; border-radius: 8px; padding: 10px 16px; font-weight: bold; border: 1px solid #45475a; margin: 4px; }\n"
        "button:hover { background: #45475a; color: #89b4fa; }\n"
        "button.suggested-action { background: #89b4fa; color: #11111b; border: none; }\n"
        "button.suggested-action:hover { background: #b4befe; }\n";

    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

static void on_app_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    apply_css();

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Chào Mừng Đến Với TizenOS 1.0");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 560);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(main_box, 24);
    gtk_widget_set_margin_end(main_box, 24);
    gtk_widget_set_margin_top(main_box, 24);
    gtk_widget_set_margin_bottom(main_box, 24);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    // Header
    GtkWidget *lbl_header = gtk_label_new("🌀 Chào Mừng Đến Với TizenOS 1.0 (Debian Edition)");
    gtk_widget_add_css_class(lbl_header, "welcome-header");
    gtk_box_append(GTK_BOX(main_box), lbl_header);

    GtkWidget *lbl_sub = gtk_label_new("Hệ điều hành Linux hiện đại, nhẹ, bảo mật cao và tích hợp đầy đủ ứng dụng văn phòng.");
    gtk_widget_add_css_class(lbl_sub, "sub-header");
    gtk_box_append(GTK_BOX(main_box), lbl_sub);

    // Card 1: Features & Info
    GtkWidget *card1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(card1, "card");
    gtk_box_append(GTK_BOX(main_box), card1);

    GtkWidget *c1_title = gtk_label_new("🚀 Điểm Nổi Bật Hệ Thống:");
    gtk_widget_set_halign(c1_title, GTK_ALIGN_START);
    gtk_widget_add_css_class(c1_title, "card-title");
    gtk_box_append(GTK_BOX(card1), c1_title);

    GtkWidget *c1_text = gtk_label_new("• Nền tảng Debian 12/14 Kernel 6.1 LTS tích hợp sẵn Smack Security MAC.\n"
                                       "• Bộ ứng dụng tinh gọn: Mousepad Text Editor, Evince PDF, VLC Player, GIMP.\n"
                                       "• Hỗ trợ đầy đủ Intel Audio Firmware, Wi-Fi Realtek/Atheros & Codecs.");
    gtk_widget_set_halign(c1_text, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(card1), c1_text);

    // Card 2: Phím tắt hữu ích
    GtkWidget *card2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(card2, "card");
    gtk_box_append(GTK_BOX(main_box), card2);

    GtkWidget *c2_title = gtk_label_new("⌨️ Phím Tắt Nhanh Nổi Bật:");
    gtk_widget_set_halign(c2_title, GTK_ALIGN_START);
    gtk_widget_add_css_class(c2_title, "card-title");
    gtk_box_append(GTK_BOX(card2), c2_title);

    GtkWidget *c2_text = gtk_label_new("• Alt + Tab : Chuyển đổi ứng dụng đang mở (Có ảnh thu nhỏ Preview)\n"
                                       "• Ctrl + Alt + Left/Right : Chuyển giữa 4 màn hình làm việc ảo (Workspaces)\n"
                                       "• Super (Windows) + 1..4 : Nhảy trực tiếp tới Workspace 1, 2, 3, 4\n"
                                       "• Ctrl + Alt + T : Mở nhanh XFCE Terminal");
    gtk_widget_set_halign(c2_text, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(card2), c2_text);

    // Quick Action Buttons
    GtkWidget *btn_box1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_box1, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), btn_box1);

    GtkWidget *btn_inst = gtk_button_new_with_label("🚀 Cài Đặt TizenOS Lên Ổ Đĩa");
    gtk_widget_add_css_class(btn_inst, "suggested-action");
    g_signal_connect(btn_inst, "clicked", G_CALLBACK(on_btn_installer_clicked), window);
    gtk_box_append(GTK_BOX(btn_box1), btn_inst);

    GtkWidget *btn_web = gtk_button_new_with_label("🌐 Trình Duyệt Web");
    g_signal_connect(btn_web, "clicked", G_CALLBACK(on_btn_browser_clicked), window);
    gtk_box_append(GTK_BOX(btn_box1), btn_web);

    GtkWidget *btn_files = gtk_button_new_with_label("📂 Quản Lý Tệp");
    g_signal_connect(btn_files, "clicked", G_CALLBACK(on_btn_files_clicked), window);
    gtk_box_append(GTK_BOX(btn_box1), btn_files);

    GtkWidget *btn_term = gtk_button_new_with_label("💻 Terminal");
    g_signal_connect(btn_term, "clicked", G_CALLBACK(on_btn_terminal_clicked), window);
    gtk_box_append(GTK_BOX(btn_box1), btn_term);

    // Footer Close Button
    GtkWidget *btn_close = gtk_button_new_with_label("✕ Bắt Đầu Trải Nghiệm TizenOS");
    g_signal_connect(btn_close, "clicked", G_CALLBACK(on_btn_close_clicked), window);
    gtk_box_append(GTK_BOX(main_box), btn_close);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.tizenos.welcome", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
