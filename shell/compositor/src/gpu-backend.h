/*
 * TizenOS GPU Backend - Header
 * =============================================================================
 * Quản lý chọn renderer và cấu hình GPU cho Wayland compositor.
 * Đọc /run/tizenos/gpu-info.env và quyết định renderer phù hợp nhất.
 * =============================================================================
 */

#ifndef TIZENOS_GPU_BACKEND_H
#define TIZENOS_GPU_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

/* Loại renderer hỗ trợ bởi wlroots */
typedef enum {
    TIZEN_RENDERER_AUTO = 0,    /* Để wlroots tự chọn */
    TIZEN_RENDERER_VULKAN,      /* Vulkan renderer (ưu tiên) */
    TIZEN_RENDERER_GLES2,       /* OpenGL ES 2.0 renderer */
    TIZEN_RENDERER_PIXMAN,      /* Software renderer (fallback cuối) */
} TizenRendererType;

/* Vendor GPU */
typedef enum {
    TIZEN_GPU_VENDOR_UNKNOWN = 0,
    TIZEN_GPU_VENDOR_NVIDIA,
    TIZEN_GPU_VENDOR_AMD,
    TIZEN_GPU_VENDOR_INTEL,
    TIZEN_GPU_VENDOR_VIRTUAL,
} TizenGpuVendor;

/* Thông tin GPU đầy đủ */
typedef struct {
    TizenGpuVendor  vendor;
    char            vendor_name[32];
    char            driver[32];
    char            driver_version[32];
    char            render_node[64];

    /* Khả năng */
    bool            has_gbm;
    bool            wayland_capable;
    bool            xwayland_hw_accel;
    bool            explicit_sync;
    bool            prime_available;
    bool            is_virtual;
    bool            modeset_active;

    /* NVIDIA-specific */
    uint32_t        nvidia_major_version;

    /* Hybrid GPU */
    char            igpu_driver[32];
    char            dgpu_driver[32];

    /* GPU count */
    int             gpu_count;
} TizenGpuConfig;

/* Kết quả trả về */
typedef enum {
    TIZEN_GPU_OK = 0,
    TIZEN_GPU_ERR_NO_FILE,      /* Không tìm thấy gpu-info.env */
    TIZEN_GPU_ERR_PARSE,        /* Lỗi parse file */
    TIZEN_GPU_ERR_NO_GPU,       /* Không phát hiện GPU */
    TIZEN_GPU_ERR_NO_WAYLAND,   /* GPU không hỗ trợ Wayland */
} TizenGpuResult;

/*
 * Phát hiện GPU: đọc /run/tizenos/gpu-info.env và parse vào config.
 * Trả về TIZEN_GPU_OK nếu thành công.
 */
TizenGpuResult tizen_gpu_detect(TizenGpuConfig *config);

/*
 * Chọn renderer tối ưu dựa trên GPU capabilities.
 * Fallback chain: Vulkan → GLES2 → Pixman
 */
TizenRendererType tizen_gpu_select_renderer(const TizenGpuConfig *config);

/*
 * Thiết lập biến môi trường cần thiết trước khi tạo wlr_backend.
 * Gọi hàm này TRƯỚC wlr_backend_autocreate().
 * Trả về số lượng env vars đã set.
 */
int tizen_gpu_apply_env(const TizenGpuConfig *config);

/*
 * Lấy tên renderer dưới dạng string (cho logging).
 */
const char *tizen_renderer_name(TizenRendererType type);

/*
 * Lấy tên vendor dưới dạng string.
 */
const char *tizen_gpu_vendor_name(TizenGpuVendor vendor);

/*
 * In diagnostic info ra stderr (cho debugging).
 */
void tizen_gpu_dump_info(const TizenGpuConfig *config);

#endif /* TIZENOS_GPU_BACKEND_H */
