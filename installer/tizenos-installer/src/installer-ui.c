#include <gtk/gtk.h>
#include <stdio.h>

static void on_app_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "TizenOS Installer");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_window_set_child(GTK_WINDOW(window), notebook);

    // Bước 1: Welcome/Lang
    GtkWidget *label_welcome = gtk_label_new("Bước 1: Chào mừng & Chọn Ngôn Ngữ");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label_welcome, gtk_label_new("Welcome"));

    // Bước 2: Keyboard
    GtkWidget *label_keyboard = gtk_label_new("Bước 2: Bố cục Bàn Phím");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label_keyboard, gtk_label_new("Keyboard"));

    // Bước 3: Timezone
    GtkWidget *label_timezone = gtk_label_new("Bước 3: Múi Giờ");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label_timezone, gtk_label_new("Timezone"));

    // Bước 4: Disk Partitioning
    GtkWidget *label_disk = gtk_label_new("Bước 4: Phân vùng Ổ Đĩa (Xóa toàn bộ, LUKS, ESP)");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label_disk, gtk_label_new("Disk"));

    // Bước 5: User Setup
    GtkWidget *label_user = gtk_label_new("Bước 5: Thiết lập Tài Khoản (User, Hostname)");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label_user, gtk_label_new("User Setup"));

    // Bước 6: Third-Party Software
    GtkWidget *label_thirdparty = gtk_label_new("Bước 6: Phần mềm Bên Thứ Ba (NVIDIA, Codecs, Flatpak)");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label_thirdparty, gtk_label_new("Third-Party"));

    // Bước 7: Installation Progress
    GtkWidget *label_progress = gtk_label_new("Bước 7: Tiến trình Cài Đặt...");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label_progress, gtk_label_new("Install"));

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.tizenos.installer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
