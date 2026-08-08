#include <gtk/gtk.h>
#include <stdio.h>

static void on_extract_here(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    g_print("Extract Here được chọn!\n");
    // Gọi API từ archive_engine.h ở đây
}

static void on_mount_virtual_drive(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    g_print("Mount Virtual Drive được chọn!\n");
    // Gọi API từ iso_mount.h ở đây
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "TizenOS File Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    // Một giao diện đơn giản (chưa hoàn thiện Grid/List views)
    GtkWidget *label = gtk_label_new("Trình quản lý tệp TizenOS");
    gtk_window_set_child(GTK_WINDOW(window), label);

    // Tạo các hành động (Actions) cho Menu Ngữ Cảnh (Context Menu)
    GSimpleAction *extract_action = g_simple_action_new("extract", NULL);
    g_signal_connect(extract_action, "activate", G_CALLBACK(on_extract_here), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(extract_action));

    GSimpleAction *mount_action = g_simple_action_new("mount", NULL);
    g_signal_connect(mount_action, "activate", G_CALLBACK(on_mount_virtual_drive), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mount_action));

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.tizenos.files", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
