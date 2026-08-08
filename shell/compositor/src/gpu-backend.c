/*
 * TizenOS GPU Backend - Implementation
 * =============================================================================
 * Parse GPU detection results, chọn renderer, và thiết lập env vars
 * cho wlroots compositor. Xử lý NVIDIA quirks, PRIME, virtual GPU.
 * =============================================================================
 */

#include "gpu-backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define GPU_ENV_FILE "/run/tizenos/gpu-info.env"
#define MAX_LINE_LEN 512

/* ---- Helpers ---------------------------------------------------------------- */

/* So sánh string không phân biệt hoa thường (portable) */
static int str_eq(const char *a, const char *b)
{
    return strcasecmp(a, b) == 0;
}

/* Parse giá trị boolean từ string "yes"/"no" */
static bool parse_bool(const char *val)
{
    return (str_eq(val, "yes") || str_eq(val, "1") || str_eq(val, "true"));
}

/* Parse vendor string thành enum */
static TizenGpuVendor parse_vendor(const char *val)
{
    if (str_eq(val, "nvidia"))   return TIZEN_GPU_VENDOR_NVIDIA;
    if (str_eq(val, "amd"))      return TIZEN_GPU_VENDOR_AMD;
    if (str_eq(val, "intel"))    return TIZEN_GPU_VENDOR_INTEL;
    if (str_eq(val, "virtual"))  return TIZEN_GPU_VENDOR_VIRTUAL;
    return TIZEN_GPU_VENDOR_UNKNOWN;
}

/* Copy string an toàn (null-terminate) */
static void safe_copy(char *dst, const char *src, size_t dst_size)
{
    if (!src || !dst || dst_size == 0) return;
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

/* ---- GPU Detection --------------------------------------------------------- */

TizenGpuResult tizen_gpu_detect(TizenGpuConfig *config)
{
    if (!config) return TIZEN_GPU_ERR_PARSE;

    /* Khởi tạo giá trị mặc định */
    memset(config, 0, sizeof(TizenGpuConfig));
    config->vendor = TIZEN_GPU_VENDOR_UNKNOWN;

    /* Mở file GPU info */
    FILE *f = fopen(GPU_ENV_FILE, "r");
    if (!f) {
        fprintf(stderr, "[gpu-backend] Không thể mở %s: %s\n",
                GPU_ENV_FILE, strerror(errno));
        return TIZEN_GPU_ERR_NO_FILE;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        /* Bỏ qua comment và dòng trống */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
            continue;

        /* Xóa newline */
        line[strcspn(line, "\r\n")] = '\0';

        /* Parse KEY=VALUE */
        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        const char *key = line;
        const char *val = eq + 1;

        /* Map key → struct field */
        if (strcmp(key, "GPU_VENDOR") == 0) {
            config->vendor = parse_vendor(val);
            safe_copy(config->vendor_name, val, sizeof(config->vendor_name));
        }
        else if (strcmp(key, "GPU_DRIVER") == 0) {
            safe_copy(config->driver, val, sizeof(config->driver));
        }
        else if (strcmp(key, "GPU_DRIVER_VERSION") == 0) {
            safe_copy(config->driver_version, val, sizeof(config->driver_version));
        }
        else if (strcmp(key, "GPU_HAS_GBM") == 0) {
            config->has_gbm = parse_bool(val);
        }
        else if (strcmp(key, "GPU_PRIME_AVAILABLE") == 0) {
            config->prime_available = parse_bool(val);
        }
        else if (strcmp(key, "GPU_WAYLAND_CAPABLE") == 0) {
            config->wayland_capable = parse_bool(val);
        }
        else if (strcmp(key, "GPU_XWAYLAND_HW_ACCEL") == 0) {
            config->xwayland_hw_accel = parse_bool(val);
        }
        else if (strcmp(key, "GPU_EXPLICIT_SYNC") == 0) {
            config->explicit_sync = parse_bool(val);
        }
        else if (strcmp(key, "GPU_RENDER_NODE") == 0) {
            safe_copy(config->render_node, val, sizeof(config->render_node));
        }
        else if (strcmp(key, "GPU_IS_VIRTUAL") == 0) {
            config->is_virtual = parse_bool(val);
        }
        else if (strcmp(key, "GPU_MODESET_ACTIVE") == 0) {
            config->modeset_active = parse_bool(val);
        }
        else if (strcmp(key, "GPU_NVIDIA_VERSION") == 0) {
            config->nvidia_major_version = (uint32_t)atoi(val);
        }
        else if (strcmp(key, "GPU_IGPU_DRIVER") == 0) {
            safe_copy(config->igpu_driver, val, sizeof(config->igpu_driver));
        }
        else if (strcmp(key, "GPU_DGPU_DRIVER") == 0) {
            safe_copy(config->dgpu_driver, val, sizeof(config->dgpu_driver));
        }
        else if (strcmp(key, "GPU_COUNT") == 0) {
            config->gpu_count = atoi(val);
        }
    }

    fclose(f);

    /* Validation */
    if (config->gpu_count == 0 && config->vendor == TIZEN_GPU_VENDOR_UNKNOWN) {
        fprintf(stderr, "[gpu-backend] Cảnh báo: Không phát hiện GPU nào\n");
        return TIZEN_GPU_ERR_NO_GPU;
    }

    return TIZEN_GPU_OK;
}

/* ---- Renderer Selection ----------------------------------------------------- */

TizenRendererType tizen_gpu_select_renderer(const TizenGpuConfig *config)
{
    if (!config) return TIZEN_RENDERER_PIXMAN;

    /*
     * Cây quyết định chọn renderer:
     *
     * 1. Virtual GPU → Pixman (software, ổn định nhất trong VM)
     * 2. NVIDIA proprietary + GBM → GLES2 (Vulkan trên NVIDIA wlroots chưa ổn)
     * 3. NVIDIA nouveau → Vulkan (Mesa Vulkan tốt hơn GL cho nouveau)
     * 4. AMD → Vulkan (RADV rất ổn định)
     * 5. Intel → Vulkan (ANV ổn định)
     * 6. Unknown → GLES2 → Pixman
     */

    /* Virtual GPU: luôn dùng Pixman */
    if (config->is_virtual) {
        fprintf(stderr, "[gpu-backend] Virtual GPU → Pixman renderer\n");
        return TIZEN_RENDERER_PIXMAN;
    }

    /* Không hỗ trợ Wayland: không nên gọi hàm này (X11 fallback) */
    if (!config->wayland_capable) {
        fprintf(stderr, "[gpu-backend] GPU không hỗ trợ Wayland → Pixman fallback\n");
        return TIZEN_RENDERER_PIXMAN;
    }

    switch (config->vendor) {
    case TIZEN_GPU_VENDOR_NVIDIA:
        if (str_eq(config->driver, "nvidia")) {
            /*
             * NVIDIA proprietary: ưu tiên GLES2
             * Vulkan renderer trên wlroots + NVIDIA có thể gây flicker
             * với driver < 545 (thiếu explicit sync)
             */
            if (config->explicit_sync) {
                fprintf(stderr, "[gpu-backend] NVIDIA + explicit sync → Vulkan\n");
                return TIZEN_RENDERER_VULKAN;
            }
            fprintf(stderr, "[gpu-backend] NVIDIA (no explicit sync) → GLES2\n");
            return TIZEN_RENDERER_GLES2;
        }
        /* nouveau: Mesa Vulkan (NVK cho mới, fallback GLES2) */
        fprintf(stderr, "[gpu-backend] nouveau → Vulkan (Mesa)\n");
        return TIZEN_RENDERER_VULKAN;

    case TIZEN_GPU_VENDOR_AMD:
        /* RADV Vulkan rất ổn định */
        fprintf(stderr, "[gpu-backend] AMD → Vulkan (RADV)\n");
        return TIZEN_RENDERER_VULKAN;

    case TIZEN_GPU_VENDOR_INTEL:
        /* ANV Vulkan ổn định */
        fprintf(stderr, "[gpu-backend] Intel → Vulkan (ANV)\n");
        return TIZEN_RENDERER_VULKAN;

    default:
        fprintf(stderr, "[gpu-backend] Unknown GPU → GLES2 fallback\n");
        return TIZEN_RENDERER_GLES2;
    }
}

/* ---- Environment Setup ------------------------------------------------------ */

int tizen_gpu_apply_env(const TizenGpuConfig *config)
{
    if (!config) return 0;

    int count = 0;

    /* Set renderer env var */
    TizenRendererType renderer = tizen_gpu_select_renderer(config);
    switch (renderer) {
    case TIZEN_RENDERER_VULKAN:
        setenv("WLR_RENDERER", "vulkan", 1);
        count++;
        break;
    case TIZEN_RENDERER_GLES2:
        /* wlroots mặc định là GLES2, không cần set */
        break;
    case TIZEN_RENDERER_PIXMAN:
        setenv("WLR_RENDERER", "pixman", 1);
        count++;
        break;
    case TIZEN_RENDERER_AUTO:
        break;
    }

    /* NVIDIA-specific env vars */
    if (config->vendor == TIZEN_GPU_VENDOR_NVIDIA &&
        str_eq(config->driver, "nvidia")) {

        if (config->has_gbm) {
            setenv("GBM_BACKEND", "nvidia-drm", 1);
            count++;
        }

        /* GLX vendor cho XWayland HW acceleration */
        if (config->xwayland_hw_accel) {
            setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
            count++;
        }

        /* Explicit sync flag */
        if (config->explicit_sync) {
            setenv("NVIDIA_EXPLICIT_SYNC", "1", 1);
            count++;
        }
    }

    /* PRIME: chỉ định DRM device cho compositor */
    if (config->prime_available && config->render_node[0] != '\0') {
        /*
         * Trong PRIME setup, compositor nên render trên iGPU (tiết kiệm điện).
         * Apps cần dGPU sẽ dùng tizenos-prime-run.
         *
         * WLR_DRM_DEVICES cho phép chỉ định card nào compositor dùng.
         * Format: /dev/dri/card0:/dev/dri/card1 (ưu tiên → fallback)
         */
        setenv("WLR_DRM_DEVICES", config->render_node, 1);
        count++;
        fprintf(stderr, "[gpu-backend] PRIME: compositor dùng %s\n",
                config->render_node);
    }

    fprintf(stderr, "[gpu-backend] Đã thiết lập %d biến môi trường\n", count);
    return count;
}

/* ---- Utility Functions ------------------------------------------------------ */

const char *tizen_renderer_name(TizenRendererType type)
{
    switch (type) {
    case TIZEN_RENDERER_AUTO:   return "auto";
    case TIZEN_RENDERER_VULKAN: return "vulkan";
    case TIZEN_RENDERER_GLES2:  return "gles2";
    case TIZEN_RENDERER_PIXMAN: return "pixman";
    }
    return "unknown";
}

const char *tizen_gpu_vendor_name(TizenGpuVendor vendor)
{
    switch (vendor) {
    case TIZEN_GPU_VENDOR_UNKNOWN: return "unknown";
    case TIZEN_GPU_VENDOR_NVIDIA:  return "nvidia";
    case TIZEN_GPU_VENDOR_AMD:     return "amd";
    case TIZEN_GPU_VENDOR_INTEL:   return "intel";
    case TIZEN_GPU_VENDOR_VIRTUAL: return "virtual";
    }
    return "unknown";
}

void tizen_gpu_dump_info(const TizenGpuConfig *config)
{
    if (!config) return;

    TizenRendererType selected = tizen_gpu_select_renderer(config);

    fprintf(stderr,
        "╔══════════════════════════════════════════════════╗\n"
        "║           TizenOS GPU Backend Info               ║\n"
        "╠══════════════════════════════════════════════════╣\n"
        "║ Vendor:        %-32s ║\n"
        "║ Driver:        %-32s ║\n"
        "║ Version:       %-32s ║\n"
        "║ Render Node:   %-32s ║\n"
        "║ GPU Count:     %-32d ║\n"
        "╠══════════════════════════════════════════════════╣\n"
        "║ GBM:           %-32s ║\n"
        "║ Wayland:       %-32s ║\n"
        "║ XWayland HW:   %-32s ║\n"
        "║ Explicit Sync: %-32s ║\n"
        "║ PRIME:         %-32s ║\n"
        "║ Virtual:       %-32s ║\n"
        "║ Modeset:       %-32s ║\n"
        "╠══════════════════════════════════════════════════╣\n"
        "║ Selected:      %-32s ║\n"
        "╚══════════════════════════════════════════════════╝\n",
        config->vendor_name,
        config->driver,
        config->driver_version,
        config->render_node[0] ? config->render_node : "(none)",
        config->gpu_count,
        config->has_gbm         ? "yes ✓" : "no ✗",
        config->wayland_capable ? "yes ✓" : "no ✗",
        config->xwayland_hw_accel ? "yes ✓" : "no ✗",
        config->explicit_sync   ? "yes ✓" : "no ✗",
        config->prime_available ? "yes ✓" : "no ✗",
        config->is_virtual      ? "yes"   : "no",
        config->modeset_active  ? "yes ✓" : "no ✗",
        tizen_renderer_name(selected)
    );
}
