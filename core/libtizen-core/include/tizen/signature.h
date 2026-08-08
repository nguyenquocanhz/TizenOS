#ifndef TIZEN_SIGNATURE_H
#define TIZEN_SIGNATURE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TIZEN_SIG_ED25519,      /* Ed25519 (EdDSA 256-bit - Nhanh & Bảo mật cao nhất) */
    TIZEN_SIG_RSA_PSS,      /* RSA-PSS 2048/4096-bit + SHA256 */
    TIZEN_SIG_ECDSA_P256    /* ECDSA Elliptic Curve (secp256r1) + SHA256 */
} TizenSignatureAlgorithm;

/**
 * Xúc tiến xác minh Chữ ký số (Digital Signature Verification) của dữ liệu đĩa.
 * 
 * @param data_path Đường dẫn file dữ liệu cần xác minh (VD: app.tpk hoặc SHA256SUMS).
 * @param sig_path Đường dẫn file chữ ký số (.sig / .asc).
 * @param pubkey_pem_path Đường dẫn file Khóa Công khai (Public Key PEM).
 * @param algo Thuật toán ký số (TIZEN_SIG_ED25519, TIZEN_SIG_RSA_PSS, TIZEN_SIG_ECDSA_P256).
 * @return true nếu chữ ký hợp lệ 100%, false nếu chữ ký giả mạo hoặc file bị can thiệp.
 */
bool tizen_signature_verify_file(const char *data_path,
                                 const char *sig_path,
                                 const char *pubkey_pem_path,
                                 TizenSignatureAlgorithm algo);

/**
 * Thực thi ký số dữ liệu file bằng Khóa Bí mật (Private Key PEM).
 */
bool tizen_signature_sign_file(const char *data_path,
                               const char *privkey_pem_path,
                               const char *out_sig_path,
                               TizenSignatureAlgorithm algo);

#ifdef __cplusplus
}
#endif

#endif /* TIZEN_SIGNATURE_H */
