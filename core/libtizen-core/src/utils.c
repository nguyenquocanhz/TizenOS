#include <tizen/error.h>
#include <glib.h>
#include <string.h>

// Hàm chuyển mã lỗi thành thông báo (Tiếng Anh/Việt tuỳ chọn)
const char* tizen_error_to_string(tizen_error_t error) {
    switch (error) {
        case TIZEN_ERROR_NONE: return "Không có lỗi (Success)";
        case TIZEN_ERROR_INVALID_PARAMETER: return "Tham số không hợp lệ (Invalid Parameter)";
        case TIZEN_ERROR_OUT_OF_MEMORY: return "Hết bộ nhớ (Out of Memory)";
        case TIZEN_ERROR_IO_ERROR: return "Lỗi vào/ra (IO Error)";
        case TIZEN_ERROR_PERMISSION_DENIED: return "Từ chối quyền truy cập (Permission Denied)";
        case TIZEN_ERROR_NOT_SUPPORTED: return "Không hỗ trợ (Not Supported)";
        default: return "Lỗi không xác định (Unknown Error)";
    }
}
