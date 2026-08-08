#ifndef TIZEN_CAMERA_H
#define TIZEN_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TizenCamera TizenCamera;

TizenCamera* tizen_camera_create(const char *device_path);
void tizen_camera_destroy(TizenCamera *camera);

void tizen_camera_start_preview(TizenCamera *camera);
void tizen_camera_stop_preview(TizenCamera *camera);

#ifdef __cplusplus
}
#endif

#endif // TIZEN_CAMERA_H
