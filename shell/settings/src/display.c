/*
 * TizenOS Display & Multi-Monitor Settings Module
 * =============================================================================
 * Quản lý độ phân giải, tần số quét (Hz), tỷ lệ hiển thị (Scaling),
 * và chế độ đa màn hình (Extend / Mirror Display) trên Wayland & X11.
 * =============================================================================
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

/* Áp dụng cấu hình đa màn hình qua wlr-randr hoặc xrandr */
void display_apply_config(const char *output_name, const char *mode, float scale, bool mirror) {
    char cmd[512];
    if (mirror) {
        printf("[DISPLAY-SETTINGS] Setting Mirror Mode on output %s...\n", output_name);
        snprintf(cmd, sizeof(cmd), "wlr-randr --output %s --mode %s --scale %.2f --same-as eDP-1 2>/dev/null || xrandr --output %s --same-as eDP-1", output_name, mode, scale, output_name);
    } else {
        printf("[DISPLAY-SETTINGS] Setting Extended Display Mode on output %s (%s, scale: %.2f)...\n", output_name, mode, scale);
        snprintf(cmd, sizeof(cmd), "wlr-randr --output %s --mode %s --scale %.2f --right-of eDP-1 2>/dev/null || xrandr --output %s --auto --right-of eDP-1", output_name, mode, scale, output_name);
    }
    system(cmd);
}

/* Tạo GtkWidget giao diện cài đặt hiển thị GTK4 */
GtkWidget* create_display_settings_widget(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(vbox, 24);
    gtk_widget_set_margin_end(vbox, 24);
    gtk_widget_set_margin_top(vbox, 24);

    GtkWidget *title = gtk_label_new("🖥️ Cài đặt Màn hình & Đa Màn hình (Multi-Monitor)");
    gtk_widget_add_css_class(title, "title-large");
    gtk_box_append(GTK_BOX(vbox), title);

    // 1. Chế độ Màn hình
    GtkWidget *mode_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *mode_lbl = gtk_label_new("Chế độ hiển thị màn hình rời:");
    GtkWidget *btn_extend = gtk_button_new_with_label(" Mở rộng (Extend) ");
    GtkWidget *btn_mirror = gtk_button_new_with_label(" Phản chiếu (Mirror) ");
    gtk_box_append(GTK_BOX(mode_group), mode_lbl);
    gtk_box_append(GTK_BOX(mode_group), btn_extend);
    gtk_box_append(GTK_BOX(mode_group), btn_mirror);
    gtk_box_append(GTK_BOX(vbox), mode_group);

    // 2. Độ phân giải & Tần số quét
    GtkWidget *res_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *res_lbl = gtk_label_new("Độ phân giải & Tần số quét:");
    GtkWidget *res_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(res_combo), "3840x2160 @ 60Hz (4K Ultra HD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(res_combo), "2560x1440 @ 144Hz (2K QHD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(res_combo), "1920x1080 @ 144Hz (Full HD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(res_combo), "1920x1080 @ 60Hz (Full HD Standard)");
    gtk_combo_box_active_id(GTK_COMBO_BOX(res_combo));
    gtk_box_append(GTK_BOX(res_group), res_lbl);
    gtk_box_append(GTK_BOX(res_group), res_combo);
    gtk_box_append(GTK_BOX(vbox), res_group);

    // 3. Wireless Display (Miracast / Cast)
    GtkWidget *cast_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *cast_lbl = gtk_label_new("📡 Chia sẻ Màn hình Không dây (Miracast / Wireless Display):");
    GtkWidget *cast_btn = gtk_button_new_with_label(" Quét tìm Smart TV & Máy chiếu Không dây ");
    gtk_box_append(GTK_BOX(cast_card), cast_lbl);
    gtk_box_append(GTK_BOX(cast_card), cast_btn);
    gtk_box_append(GTK_BOX(vbox), cast_card);

    return vbox;
}
