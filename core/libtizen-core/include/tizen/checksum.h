#ifndef TIZEN_CHECKSUM_H
#define TIZEN_CHECKSUM_H

#include <stdbool.h>
#include <stddef.h>
#include "tizen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TIZEN_HASH_MD5,
    TIZEN_HASH_SHA256,
    TIZEN_HASH_SHA512
} TizenHashType;

/**
 * Tính toán mã băm (Hash Checksum) cho một tập tin đĩa.
 * @param filepath Đường dẫn đến file cần tính checksum.
 * @param type Loại thuật toán (TIZEN_HASH_SHA256, TIZEN_HASH_SHA512, TIZEN_HASH_MD5).
 * @param out_hash_hex Buffer nhận chuỗi hex (yêu cầu tối thiểu 129 bytes).
 * @return true nếu tính toán thành công, false nếu lỗi.
 */
bool tizen_checksum_file(const char *filepath, TizenHashType type, char *out_hash_hex);

/**
 * Tính toán mã băm cho vùng nhớ RAM buffer.
 */
bool tizen_checksum_buffer(const void *data, size_t len, TizenHashType type, char *out_hash_hex);

/**
 * Đối chiếu kiểm tra checksum của tệp có khớp với hash kỳ vọng hay không.
 */
bool tizen_checksum_verify_file(const char *filepath, TizenHashType type, const char *expected_hash_hex);

#ifdef __cplusplus
}
#endif

#endif /* TIZEN_CHECKSUM_H */
