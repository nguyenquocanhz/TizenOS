#include <gtk/gtk.h>

// Giao diện điều khiển hệ thống TizenOS bằng GTK4
// Ứng dụng này sẽ được hiển thị như một cửa sổ bình thường (XDG shell)
static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "TizenOS Settings");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    // Giao diện chính chứa sidebar và stack
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), box);
    
    gtk_widget_show(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.tizenos.settings", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
