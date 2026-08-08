#ifndef __TIZEN_ERROR_H__
#define __TIZEN_ERROR_H__

#include <tizen/types.h>

// Các mã lỗi chuẩn (Unified error handling macros)
#define TIZEN_ERROR_NONE                 0
#define TIZEN_ERROR_INVALID_PARAMETER    -1
#define TIZEN_ERROR_OUT_OF_MEMORY        -2
#define TIZEN_ERROR_IO_ERROR             -3
#define TIZEN_ERROR_PERMISSION_DENIED    -4
#define TIZEN_ERROR_NOT_SUPPORTED        -5

#ifdef __cplusplus
extern "C" {
#endif

// Hàm chuyển đổi mã lỗi thành chuỗi thông báo
const char* tizen_error_to_string(tizen_error_t error);

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_ERROR_H__
