#include <stdio.h>
#include <glib.h>
#include "tizen/app.h"
#include "tizen/app-event.h"

// Vòng lặp chính của ứng dụng
static GMainLoop *main_loop = NULL;

int app_main(int argc, char **argv, app_callbacks_t *callbacks, void *user_data) {
    printf("Bắt đầu vòng lặp ứng dụng TizenOS...\n");
    
    // Gởi sự kiện create để khởi tạo ứng dụng
    if (callbacks && callbacks->create) {
        callbacks->create(user_data);
    }
    
    app_event_register_system_events();
    
    // Tích hợp với Glib mainloop để xử lý IPC và event
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);
    
    // Gởi sự kiện terminate khi thoát
    if (callbacks && callbacks->terminate) {
        callbacks->terminate(user_data);
    }
    return 0;
}

void app_exit(void) {
    if (main_loop) {
        g_main_loop_quit(main_loop);
    }
}
