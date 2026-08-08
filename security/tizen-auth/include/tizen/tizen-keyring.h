#ifndef TIZEN_KEYRING_H
#define TIZEN_KEYRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi tạo Tizen Keyring (tương thích Secret Service).
 * Sử dụng thuật toán AES-256-GCM để mã hóa các thông tin xác thực.
 * @return 0 nếu thành công, < 0 nếu lỗi.
 */
int tizen_keyring_init(void);

/**
 * @brief Lưu trữ thông tin đăng nhập vào keyring an toàn.
 * @param label Nhãn nhận diện của mục.
 * @param secret Dữ liệu nhạy cảm (mật khẩu/token).
 * @param secret_len Chiều dài dữ liệu nhạy cảm.
 * @return 0 nếu thành công, < 0 nếu lỗi.
 */
int tizen_keyring_store(const char *label, const unsigned char *secret, size_t secret_len);

/**
 * @brief Truy xuất thông tin đăng nhập từ keyring.
 * @param label Nhãn nhận diện của mục cần lấy.
 * @param out_secret Con trỏ lưu trữ dữ liệu trả về (cần được giải phóng bởi người gọi).
 * @param out_len Chiều dài dữ liệu trả về.
 * @return 0 nếu thành công, < 0 nếu lỗi.
 */
int tizen_keyring_retrieve(const char *label, unsigned char **out_secret, size_t *out_len);

/**
 * @brief Xóa thông tin đăng nhập từ keyring.
 * @param label Nhãn nhận diện của mục cần xóa.
 * @return 0 nếu thành công, < 0 nếu lỗi.
 */
int tizen_keyring_delete(const char *label);

#ifdef __cplusplus
}
#endif

#endif // TIZEN_KEYRING_H
