#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>

// Hàm chính khởi tạo Wayland compositor sử dụng wlroots
// Đây là trái tim của TizenOS Desktop Shell
int main(int argc, char **argv) {
    struct wl_display *display = wl_display_create();
    
    // Khởi tạo backend, quản lý output (màn hình) và input (chuột, phím)
    struct wlr_backend *backend = wlr_backend_autocreate(display, NULL);
    
    // Khởi tạo renderer để vẽ các surface của Wayland client
    // Tích hợp wlroots surface rendering
    struct wlr_renderer *renderer = wlr_renderer_autocreate(backend);
    wlr_renderer_init_wl_display(renderer, display);
    
    struct wlr_allocator *allocator = wlr_allocator_autocreate(backend, renderer);
    wlr_compositor_create(display, 5, renderer);
    wlr_data_device_manager_create(display);

    // Chạy vòng lặp sự kiện Wayland
    wl_display_run(display);
    
    wl_display_destroy(display);
    return 0;
}
