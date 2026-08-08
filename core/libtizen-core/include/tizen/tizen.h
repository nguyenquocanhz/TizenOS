#ifndef __TIZEN_H__
#define __TIZEN_H__

// Định nghĩa chung cho thư viện tizen-core
#include <tizen/types.h>
#include <tizen/error.h>
#include <tizen/log.h>

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo thư viện tizen core
int tizen_core_init(void);

// Dọn dẹp tài nguyên thư viện
void tizen_core_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_H__
