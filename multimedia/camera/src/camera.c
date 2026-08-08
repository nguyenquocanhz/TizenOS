#include "tizen/camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _TizenCamera {
    char *device_path;
    int is_previewing;
};

TizenCamera* tizen_camera_create(const char *device_path) {
    TizenCamera *camera = malloc(sizeof(TizenCamera));
    camera->device_path = strdup(device_path ? device_path : "/dev/video0");
    camera->is_previewing = 0;
    
    // Giao tiếp với V4L2 (Video4Linux2) để mở thiết bị capture
    printf("Đã khởi tạo Camera bằng V4L2 cho thiết bị: %s\n", camera->device_path);
    return camera;
}

void tizen_camera_destroy(TizenCamera *camera) {
    if (camera) {
        free(camera->device_path);
        free(camera);
    }
}

void tizen_camera_start_preview(TizenCamera *camera) {
    if (camera && !camera->is_previewing) {
        camera->is_previewing = 1;
        // Tạo Wayland preview surface để hiển thị video trực tiếp (zero-copy)
        printf("Bắt đầu xem trước Camera qua Wayland preview surface.\n");
    }
}

void tizen_camera_stop_preview(TizenCamera *camera) {
    if (camera && camera->is_previewing) {
        camera->is_previewing = 0;
        printf("Đã dừng xem trước Camera.\n");
    }
}
