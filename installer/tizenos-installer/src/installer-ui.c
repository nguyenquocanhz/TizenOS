#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "partman.h"
#include "user-setup.h"
#include "third-party.h"
#include "target-install.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *list_steps;
    GtkWidget *btn_back;
    GtkWidget *btn_next;
    GtkWidget *progress_bar;
    GtkWidget *lbl_status;
    GtkWidget *lbl_summary;
    GtkWidget *combo_disk;
    GtkStringList *disk_model;
    GtkWidget *entry_username;
    GtkWidget *entry_fullname;
    GtkWidget *entry_password;
    GtkWidget *entry_hostname;
    int current_step;
} InstallerApp;

static void force_ui_update(void) {
    while (g_main_context_iteration(NULL, FALSE));
}

static void on_btn_reboot_now(GtkButton *btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    system("reboot || systemctl reboot");
}

static void on_btn_reboot_later(GtkButton *btn, gpointer user_data) {
    (void)btn;
    GtkWindow *win = GTK_WINDOW(user_data);
    if (win) {
        gtk_window_destroy(win);
    }
}

static void show_reboot_modal(GtkWindow *parent) {
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_title(GTK_WINDOW(dialog), "🎉 Hoàn Tất Cài Đặt TizenOS");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 240);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget *lbl = gtk_label_new("🎉 HOÀN TẤT CÀI ĐẶT TIZENOS!\n\nHệ thống TizenOS đã được cài đặt thành công lên đĩa cứng.\nVui lòng tháo ISO và chọn Khởi động lại ngay.");
    gtk_label_set_justify(GTK_LABEL(lbl), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(box), lbl);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), btn_box);

    GtkWidget *btn_reboot = gtk_button_new_with_label("🔄 Khởi Động Lại Ngay");
    gtk_widget_add_css_class(btn_reboot, "suggested-action");
    g_signal_connect(btn_reboot, "clicked", G_CALLBACK(on_btn_reboot_now), NULL);
    gtk_box_append(GTK_BOX(btn_box), btn_reboot);

    GtkWidget *btn_later = gtk_button_new_with_label("✕ Để Sau");
    g_signal_connect(btn_later, "clicked", G_CALLBACK(on_btn_reboot_later), dialog);
    gtk_box_append(GTK_BOX(btn_box), btn_later);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void populate_disks(GtkStringList *string_list) {
    FILE *fp = popen("lsblk -d -n -o NAME,SIZE,MODEL,TYPE | grep -E 'disk'", "r");
    if (!fp) {
        gtk_string_list_append(string_list, "/dev/sda");
        return;
    }

    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        char name[64];
        if (sscanf(line, "%63s", name) == 1) {
            char dev_path[128];
            snprintf(dev_path, sizeof(dev_path), "/dev/%s", name);
            gtk_string_list_append(string_list, dev_path);
            count++;
        }
    }
    pclose(fp);

    if (count == 0) {
        gtk_string_list_append(string_list, "/dev/sda");
    }
}

static void update_summary_text(InstallerApp *app) {
    guint selected_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(app->combo_disk));
    const char *disk_str = gtk_string_list_get_string(app->disk_model, selected_idx);
    if (!disk_str || strlen(disk_str) == 0) disk_str = "/dev/sda";

    const char *username = gtk_editable_get_text(GTK_EDITABLE(app->entry_username));
    const char *fullname = gtk_editable_get_text(GTK_EDITABLE(app->entry_fullname));
    const char *hostname = gtk_editable_get_text(GTK_EDITABLE(app->entry_hostname));

    if (strlen(username) == 0) username = "tizen";
    if (strlen(fullname) == 0) fullname = "TizenOS User";
    if (strlen(hostname) == 0) hostname = "TizenOS-PC";

    char summary_buf[1024];
    snprintf(summary_buf, sizeof(summary_buf),
        "📌 XÁC NHẬN CẤU HÌNH CÀI ĐẶT TIZENOS:\n\n"
        "• 💽 Ổ đĩa mục tiêu : %s (Tự động chia GPT + BIOS/UEFI Dual Boot)\n"
        "• 👤 Người dùng chính: %s (%s)\n"
        "• 💻 Tên máy tính   : %s\n"
        "• 🕒 Múi giờ        : Asia/Ho_Chi_Minh (UTC+7)\n"
        "• 📦 Phần mềm & App : Mousepad, Evince PDF, Firefox ESR, VLC, GDebi 1-Click\n\n"
        "⚠️ CẢNH BÁO: Toàn bộ dữ liệu trên ổ đĩa %s sẽ được khởi tạo lại!",
        disk_str, username, fullname, hostname, disk_str
    );
    gtk_label_set_text(GTK_LABEL(app->lbl_summary), summary_buf);
}

static void update_navigation(InstallerApp *app) {
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->list_steps), app->current_step);
    if (row) {
        gtk_list_box_select_row(GTK_LIST_BOX(app->list_steps), row);
    }

    if (app->current_step == 0) {
        gtk_widget_set_sensitive(app->btn_back, FALSE);
    } else {
        gtk_widget_set_sensitive(app->btn_back, TRUE);
    }

    if (app->current_step == 5) {
        update_summary_text(app);
        gtk_button_set_label(GTK_BUTTON(app->btn_next), "⚡ Bắt Đầu Cài Đặt");
    } else if (app->current_step == 6) {
        gtk_widget_set_sensitive(app->btn_back, FALSE);
        gtk_button_set_label(GTK_BUTTON(app->btn_next), "🔄 Khởi Động Lại Ngay");
    } else {
        gtk_button_set_label(GTK_BUTTON(app->btn_next), "Tiếp theo →");
    }
}

static void on_btn_back_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    InstallerApp *app = (InstallerApp *)user_data;
    if (app->current_step > 0 && app->current_step < 6) {
        app->current_step--;
        char page_name[32];
        snprintf(page_name, sizeof(page_name), "step%d", app->current_step + 1);
        gtk_stack_set_visible_child_name(GTK_STACK(app->stack), page_name);
        update_navigation(app);
    }
}

static gboolean run_installation(gpointer user_data) {
    InstallerApp *app = (InstallerApp *)user_data;

    guint selected_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(app->combo_disk));
    const char *disk_str = gtk_string_list_get_string(app->disk_model, selected_idx);

    char selected_disk[256];
    if (!disk_str || strlen(disk_str) == 0) {
        snprintf(selected_disk, sizeof(selected_disk), "/dev/sda");
    } else {
        snprintf(selected_disk, sizeof(selected_disk), "%.240s", disk_str);
    }

    const char *username = gtk_editable_get_text(GTK_EDITABLE(app->entry_username));
    const char *fullname = gtk_editable_get_text(GTK_EDITABLE(app->entry_fullname));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(app->entry_password));
    const char *hostname = gtk_editable_get_text(GTK_EDITABLE(app->entry_hostname));

    if (strlen(username) == 0) username = "tizen";
    if (strlen(fullname) == 0) fullname = "TizenOS User";
    if (strlen(password) == 0) password = "live";
    if (strlen(hostname) == 0) hostname = "TizenOS-PC";

    char esp_part[512], swap_part[512], root_part[512];
    if (strstr(selected_disk, "nvme") || strstr(selected_disk, "mmcblk")) {
        snprintf(esp_part, sizeof(esp_part), "%.240sp2", selected_disk);
        snprintf(swap_part, sizeof(swap_part), "%.240sp3", selected_disk);
        snprintf(root_part, sizeof(root_part), "%.240sp4", selected_disk);
    } else {
        snprintf(esp_part, sizeof(esp_part), "%.240s2", selected_disk);
        snprintf(swap_part, sizeof(swap_part), "%.240s3", selected_disk);
        snprintf(root_part, sizeof(root_part), "%.240s4", selected_disk);
    }

    // Step 1: Wipe & partition disk
    gtk_label_set_text(GTK_LABEL(app->lbl_status), "--> [1/5] Xóa đĩa & khởi tạo phân vùng GPT kép (BIOS + UEFI)...");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0.15);
    force_ui_update();
    partman_erase_disk(selected_disk);

    // Step 2: Format EFI, SWAP & rootfs
    gtk_label_set_text(GTK_LABEL(app->lbl_status), "--> [2/5] Định dạng EFI (FAT32 512MB), SWAP & Root ext4 (20GB+)...");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0.30);
    force_ui_update();
    partman_format_partitions(esp_part, swap_part, root_part);

    // Mount target partitions before copying
    system("mkdir -p /mnt/target");
    char mount_cmd[2048];
    snprintf(mount_cmd, sizeof(mount_cmd), "mount %s /mnt/target && mkdir -p /mnt/target/boot/efi && mount %s /mnt/target/boot/efi", root_part, esp_part);
    system(mount_cmd);

    // Step 3: Copy rootfs
    gtk_label_set_text(GTK_LABEL(app->lbl_status), "--> [3/5] Xả hệ thống RootFS (rsync -aHAX bảo toàn Smack xattr)...");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0.60);
    force_ui_update();
    target_install_copy_rootfs(NULL, "/mnt/target");

    // Step 4: Generate fstab & user setup inside target
    gtk_label_set_text(GTK_LABEL(app->lbl_status), "--> [4/5] Sinh fstab UUID, hostname & khởi tạo tài khoản cá nhân...");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0.80);
    force_ui_update();
    target_install_generate_fstab(root_part, esp_part, swap_part);
    user_setup_create_account(username, password, fullname);
    user_setup_set_hostname(hostname);
    user_setup_enable_autologin(username);

    // Step 5: Install dual bootloader GRUB2 & flush VMware I/O buffers
    gtk_label_set_text(GTK_LABEL(app->lbl_status), "--> [5/5] Cài đặt GRUB2 Dual Bootloader & Đồng bộ đĩa ảo VMware...");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 1.0);
    force_ui_update();
    target_install_grub(selected_disk, true);

    gtk_label_set_text(GTK_LABEL(app->lbl_status), "✓ HOÀN TẤT CÀI ĐẶT TIZENOS! Bạn có thể tháo ISO và bấm Khởi động lại.");
    app->current_step = 6;
    update_navigation(app);

    // Hiển thị hộp thoại Modal thông báo và hỏi Khởi động lại ngay
    show_reboot_modal(GTK_WINDOW(app->window));

    return G_SOURCE_REMOVE;
}

static void on_btn_next_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    InstallerApp *app = (InstallerApp *)user_data;

    if (app->current_step < 5) {
        app->current_step++;
        char page_name[32];
        snprintf(page_name, sizeof(page_name), "step%d", app->current_step + 1);
        gtk_stack_set_visible_child_name(GTK_STACK(app->stack), page_name);
        update_navigation(app);
    } else if (app->current_step == 5) {
        app->current_step = 6;
        gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "step7");
        gtk_widget_set_sensitive(app->btn_next, FALSE);
        g_timeout_add(100, run_installation, app);
    } else if (app->current_step == 6) {
        system("reboot || systemctl reboot");
    }
}

static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css =
        "window { background-color: #1e1e2e; color: #cdd6f4; font-family: 'Inter', sans-serif; }\n"
        ".sidebar { background-color: #181825; border-right: 1px solid #313244; padding: 16px; min-width: 230px; }\n"
        ".sidebar label { font-size: 14px; font-weight: bold; color: #a6adc8; }\n"
        "listview row:selected, listbox row:selected { background-color: #89b4fa; color: #11111b; border-radius: 6px; font-weight: bold; }\n"
        "label { font-size: 14px; }\n"
        ".title-label { font-size: 20px; font-weight: bold; color: #89b4fa; margin-bottom: 8px; }\n"
        "button { background: #313244; color: #cdd6f4; border-radius: 8px; padding: 8px 16px; font-weight: bold; border: 1px solid #45475a; }\n"
        "button:hover { background: #45475a; color: #89b4fa; }\n"
        "button.suggested-action { background: #89b4fa; color: #11111b; border: none; }\n"
        "button.suggested-action:hover { background: #b4befe; }\n"
        "entry, dropdown { background: #313244; color: #cdd6f4; border-radius: 6px; padding: 6px; border: 1px solid #45475a; }\n"
        "progressbar > trough { background-color: #313244; border-radius: 6px; }\n"
        "progressbar > trough > progress { background-color: #89b4fa; border-radius: 6px; }\n";

    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    InstallerApp *app = (InstallerApp *)user_data;
    if (app) {
        g_free(app);
    }
}

static void on_app_activate(GtkApplication *app_gtk, gpointer user_data) {
    (void)user_data;
    apply_css();

    InstallerApp *app = g_malloc0(sizeof(InstallerApp));
    app->current_step = 0;

    app->window = gtk_application_window_new(app_gtk);
    gtk_window_set_title(GTK_WINDOW(app->window), "TizenOS Debian Installer GTK4");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 920, 640);
    g_signal_connect(app->window, "destroy", G_CALLBACK(on_window_destroy), app);

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(app->window), root_box);

    // 1. Left Sidebar Navigation (Calamares / Debian Installer Style)
    GtkWidget *sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(sidebar_box, "sidebar");
    gtk_box_append(GTK_BOX(root_box), sidebar_box);

    GtkWidget *brand_lbl = gtk_label_new("🌀 TizenOS 1.0\nDebian Installer");
    gtk_widget_add_css_class(brand_lbl, "title-label");
    gtk_box_append(GTK_BOX(sidebar_box), brand_lbl);

    app->list_steps = gtk_list_box_new();
    gtk_widget_set_sensitive(app->list_steps, FALSE);
    gtk_box_append(GTK_BOX(sidebar_box), app->list_steps);

    const char *step_names[] = {
        "1. 🌐 Chào Mừng",
        "2. ⌨️ Bàn Phím",
        "3. 🕒 Múi Giờ",
        "4. 💽 Phân Vùng Đĩa",
        "5. 👤 Người Dùng",
        "6. 📋 Xác Nhận & Cài Đặt",
        "7. ⚙️ Tiến Trình Cài Đặt"
    };

    for (int i = 0; i < 7; i++) {
        GtkWidget *lbl = gtk_label_new(step_names[i]);
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_widget_set_margin_start(lbl, 8);
        gtk_widget_set_margin_top(lbl, 8);
        gtk_widget_set_margin_bottom(lbl, 8);
        gtk_list_box_append(GTK_LIST_BOX(app->list_steps), lbl);
    }

    // 2. Right Content & Action Area
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_hexpand(right_box, TRUE);
    gtk_widget_set_margin_start(right_box, 20);
    gtk_widget_set_margin_end(right_box, 20);
    gtk_widget_set_margin_top(right_box, 20);
    gtk_widget_set_margin_bottom(right_box, 20);
    gtk_box_append(GTK_BOX(root_box), right_box);

    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_widget_set_vexpand(app->stack, TRUE);
    gtk_box_append(GTK_BOX(right_box), app->stack);

    // Step 1: Welcome
    GtkWidget *box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(box1), gtk_label_new("Bước 1/6: Chào mừng bạn đến với TizenOS 1.0 (Debian Edition)"));
    gtk_box_append(GTK_BOX(box1), gtk_label_new("Trình cài đặt đồ họa GTK4 thiết kế theo giao diện Calamares / Debian Installer chuẩn hóa."));
    gtk_stack_add_named(GTK_STACK(app->stack), box1, "step1");

    // Step 2: Keyboard
    GtkWidget *box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(box2), gtk_label_new("Bước 2/6: Bố cục bàn phím (Keyboard Layout)"));
    gtk_box_append(GTK_BOX(box2), gtk_label_new("Mặc định: English (US) Keyboard Layout"));
    gtk_stack_add_named(GTK_STACK(app->stack), box2, "step2");

    // Step 3: Timezone
    GtkWidget *box3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(box3), gtk_label_new("Bước 3/6: Múi giờ & Vị trí (Timezone)"));
    gtk_box_append(GTK_BOX(box3), gtk_label_new("Mặc định: Asia/Ho_Chi_Minh (UTC+7)"));
    gtk_stack_add_named(GTK_STACK(app->stack), box3, "step3");

    // Step 4: Disk Partitioning & Drive Dropdown
    GtkWidget *box4 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(box4), gtk_label_new("Bước 4/6: Phân vùng Ổ đĩa Kép (BIOS + UEFI Clean Wipe)"));
    gtk_box_append(GTK_BOX(box4), gtk_label_new("Chọn ổ đĩa mục tiêu để cài đặt TizenOS:"));

    app->disk_model = gtk_string_list_new(NULL);
    populate_disks(app->disk_model);

    app->combo_disk = gtk_drop_down_new(G_LIST_MODEL(app->disk_model), NULL);
    gtk_box_append(GTK_BOX(box4), app->combo_disk);

    gtk_box_append(GTK_BOX(box4), gtk_label_new("Sơ đồ phân vùng tự động (GPT Clean Wipe):\n"
                                                "• Partition 1: bios_grub (2MB) - Legacy BIOS MBR\n"
                                                "• Partition 2: ESP FAT32 (512MB) - UEFI /boot/efi\n"
                                                "• Partition 3: Linux Swap (4GB)\n"
                                                "• Partition 4: Root OS ext4 (20GB+ đến 100% đĩa)"));
    gtk_stack_add_named(GTK_STACK(app->stack), box4, "step4");

    // Step 5: User Setup
    GtkWidget *box5 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_append(GTK_BOX(box5), gtk_label_new("Bước 5/6: Thiết lập Tài khoản & Hostname"));
    
    app->entry_username = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(app->entry_username), "tizen");
    gtk_box_append(GTK_BOX(box5), gtk_label_new("Tên người dùng (Username):"));
    gtk_box_append(GTK_BOX(box5), app->entry_username);

    app->entry_fullname = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(app->entry_fullname), "TizenOS User");
    gtk_box_append(GTK_BOX(box5), gtk_label_new("Họ và tên (Full Name):"));
    gtk_box_append(GTK_BOX(box5), app->entry_fullname);

    app->entry_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app->entry_password), FALSE);
    gtk_editable_set_text(GTK_EDITABLE(app->entry_password), "live");
    gtk_box_append(GTK_BOX(box5), gtk_label_new("Mật khẩu (Password):"));
    gtk_box_append(GTK_BOX(box5), app->entry_password);

    app->entry_hostname = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(app->entry_hostname), "TizenOS-PC");
    gtk_box_append(GTK_BOX(box5), gtk_label_new("Tên máy tính (Hostname):"));
    gtk_box_append(GTK_BOX(box5), app->entry_hostname);
    gtk_stack_add_named(GTK_STACK(app->stack), box5, "step5");

    // Step 6: Confirmation Summary
    GtkWidget *box6 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    app->lbl_summary = gtk_label_new("Xác nhận thông tin...");
    gtk_widget_set_halign(app->lbl_summary, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box6), app->lbl_summary);
    gtk_stack_add_named(GTK_STACK(app->stack), box6, "step6");

    // Step 7: Install Progress
    GtkWidget *box7 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_append(GTK_BOX(box7), gtk_label_new("Bước Tiến Trình: Cài đặt Hệ thống lên Ổ đĩa"));
    
    app->progress_bar = gtk_progress_bar_new();
    gtk_box_append(GTK_BOX(box7), app->progress_bar);

    app->lbl_status = gtk_label_new("Sẵn sàng cài đặt TizenOS...");
    gtk_box_append(GTK_BOX(box7), app->lbl_status);
    gtk_stack_add_named(GTK_STACK(app->stack), box7, "step7");

    // Navigation Action Buttons
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(right_box), btn_box);

    app->btn_back = gtk_button_new_with_label("← Quay lại");
    g_signal_connect(app->btn_back, "clicked", G_CALLBACK(on_btn_back_clicked), app);
    gtk_box_append(GTK_BOX(btn_box), app->btn_back);

    app->btn_next = gtk_button_new_with_label("Tiếp theo →");
    gtk_widget_add_css_class(app->btn_next, "suggested-action");
    g_signal_connect(app->btn_next, "clicked", G_CALLBACK(on_btn_next_clicked), app);
    gtk_box_append(GTK_BOX(btn_box), app->btn_next);

    update_navigation(app);
    gtk_window_present(GTK_WINDOW(app->window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.tizenos.installer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
