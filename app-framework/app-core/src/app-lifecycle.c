#include <stdio.h>
#include "tizen/app.h"

// Xử lý chuyển đổi trạng thái của ứng dụng (lifecycle)
void app_lifecycle_pause() {
    printf("Ứng dụng chuyển sang trạng thái pause.\n");
}

void app_lifecycle_resume() {
    printf("Ứng dụng chuyển sang trạng thái resume.\n");
}
