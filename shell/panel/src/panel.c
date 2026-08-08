/*
 * TizenOS Navbar / Top Panel Bar - Implementation (GTK4 + Layer Shell)
 * =============================================================================
 * Thanh Navbar nằm ngang ở mép trên màn hình với hiệu ứng Glassmorphism.
 * Cấu trúc 3 Khu vực:
 * 1. LEFT: App Menu Categories Dropdown (Internet, Graphics, Games, Office, System)
 * 2. CENTER: Workspace Switcher (1, 2, 3, 4) & Active Window Indicator
 * 3. RIGHT: Quick Settings System Tray (Wi-Fi, PipeWire Volume, Battery, Clock, Power)
 * =============================================================================
 */

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <time.h>

static GtkWidget *clock_label = NULL;

/* Cập nhật đồng hồ thời gian thực */
static gboolean update_clock(gpointer user_data) {
    (void)user_data;
    if (!clock_label) return FALSE;

    time_t rawtime;
    struct tm *info;
    char buffer[64];

    time(&rawtime);
    info = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), " %H:%M  •  %d/%m/%Y ", info);

    gtk_label_set_text(GTK_LABEL(clock_label), buffer);
    return TRUE; // Tiếp tục gọi lại sau 1s
}

/* Tạo App Menu Popover bên Trái */
static GtkWidget* create_app_menu_button(void) {
    GtkWidget *btn = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(btn), " 🌐 TizenOS Menu ");

    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);

    const char *categories[] = {
        "🌐  Trình duyệt & Internet",
        "🎨  Đồ họa & Đa phương tiện",
        "💼  Văn phòng & Công việc",
        "🎮  Trò chơi & Giải trí",
        "⚙️  Hệ thống & Cài đặt (tizen-settings)",
        "📁  Trình quản lý File (tizenos-files)",
        "💻  Terminal Command Prompt"
    };

    for (size_t i = 0; i < sizeof(categories)/sizeof(categories[0]); i++) {
        GtkWidget *item_btn = gtk_button_new_with_label(categories[i]);
        gtk_box_append(GTK_BOX(box), item_btn);
    }

    gtk_popover_set_child(GTK_POPOVER(popover), box);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(btn), popover);
    return btn;
}

/* Tạo Workspace Switcher ở Giữa */
static GtkWidget* create_workspace_switcher(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    for (int i = 1; i <= 4; i++) {
        char label[16];
        snprintf(label, sizeof(label), " %d ", i);
        GtkWidget *ws_btn = gtk_button_new_with_label(label);
        if (i == 1) {
            gtk_widget_add_css_class(ws_btn, "active-workspace");
        }
        gtk_box_append(GTK_BOX(box), ws_btn);
    }
    return box;
}

/* Tạo Quick Settings Tray bên Phải */
static GtkWidget* create_quick_settings_tray(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

    // Network Icon
    GtkWidget *net_btn = gtk_button_new_with_label(" 📶 Wi-Fi ");
    gtk_box_append(GTK_BOX(box), net_btn);

    // Volume Icon
    GtkWidget *vol_btn = gtk_button_new_with_label(" 🔊 80% ");
    gtk_box_append(GTK_BOX(box), vol_btn);

    // Battery Icon
    GtkWidget *bat_btn = gtk_button_new_with_label(" 🔋 98% ");
    gtk_box_append(GTK_BOX(box), bat_btn);

    // Clock Label
    clock_label = gtk_label_new("");
    gtk_widget_add_css_class(clock_label, "navbar-clock");
    gtk_box_append(GTK_BOX(box), clock_label);
    update_clock(NULL);
    g_timeout_add_seconds(1, update_clock, NULL);

    // Power Options Button
    GtkWidget *power_btn = gtk_button_new_with_label(" ⏏️ Nguồn ");
    gtk_box_append(GTK_BOX(box), power_btn);

    return box;
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    GtkWidget *window = gtk_application_window_new(app);

    // Cấu hình GTK4 Layer Shell ở vị trí TOP BAR
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);

    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 40); // Cao 40px

    // Master Horizontal Header Box
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(header_box, 8);
    gtk_widget_set_margin_end(header_box, 8);

    // 1. LEFT ZONE
    GtkWidget *left_box = create_app_menu_button();
    gtk_box_append(GTK_BOX(header_box), left_box);

    // Separator Spacer Left -> Center
    GtkWidget *spacer1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer1, TRUE);
    gtk_box_append(GTK_BOX(header_box), spacer1);

    // 2. CENTER ZONE
    GtkWidget *center_box = create_workspace_switcher();
    gtk_box_append(GTK_BOX(header_box), center_box);

    // Separator Spacer Center -> Right
    GtkWidget *spacer2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer2, TRUE);
    gtk_box_append(GTK_BOX(header_box), spacer2);

    // 3. RIGHT ZONE
    GtkWidget *right_box = create_quick_settings_tray();
    gtk_box_append(GTK_BOX(header_box), right_box);

    gtk_window_set_child(GTK_WINDOW(window), header_box);
    gtk_widget_show(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.tizenos.panel", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
