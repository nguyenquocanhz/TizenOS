/*
 * TizenOS Digital Signature Verification - Implementation
 * =============================================================================
 * Lập trình xác minh và tạo chữ ký số bằng thuật toán Ed25519, RSA-PSS,
 * và ECDSA P-256 sử dụng OpenSSL 3.0 EVP API.
 * =============================================================================
 */

#include "tizen/signature.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#define CHUNK_SIZE 8192

static EVP_PKEY *load_public_key(const char *pubkey_path) {
    FILE *fp = fopen(pubkey_path, "r");
    if (!fp) return NULL;
    EVP_PKEY *pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);
    return pkey;
}

static EVP_PKEY *load_private_key(const char *privkey_path) {
    FILE *fp = fopen(privkey_path, "r");
    if (!fp) return NULL;
    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);
    return pkey;
}

bool tizen_signature_verify_file(const char *data_path,
                                 const char *sig_path,
                                 const char *pubkey_pem_path,
                                 TizenSignatureAlgorithm algo) {
    if (!data_path || !sig_path || !pubkey_pem_path) return false;

    // 1. Load Public Key
    EVP_PKEY *pkey = load_public_key(pubkey_pem_path);
    if (!pkey) {
        fprintf(stderr, "[SIG-ERROR] Không thể đọc Public Key từ %s\n", pubkey_pem_path);
        return false;
    }

    // 2. Load Signature Binary
    FILE *sig_fp = fopen(sig_path, "rb");
    if (!sig_fp) {
        EVP_PKEY_free(pkey);
        return false;
    }
    fseek(sig_fp, 0, SEEK_END);
    long sig_len = ftell(sig_fp);
    fseek(sig_fp, 0, SEEK_SET);

    unsigned char *sig_buf = malloc((size_t)sig_len);
    if (!sig_buf || fread(sig_buf, 1, (size_t)sig_len, sig_fp) != (size_t)sig_len) {
        fclose(sig_fp);
        free(sig_buf);
        EVP_PKEY_free(pkey);
        return false;
    }
    fclose(sig_fp);

    // 3. Khởi tạo OpenSSL EVP DigestVerify Context
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD *md = (algo == TIZEN_SIG_ED25519) ? NULL : EVP_sha256();

    if (EVP_DigestVerifyInit(ctx, NULL, md, NULL, pkey) != 1) {
        free(sig_buf);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    // Configure RSA-PSS padding nếu dùng RSA
    if (algo == TIZEN_SIG_RSA_PSS) {
        EVP_PKEY_CTX *pctx = NULL;
        EVP_MD_CTX_pkey_ctx(ctx, &pctx);
        if (pctx) {
            EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING);
            EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, RSA_PSS_SALTLEN_DIGEST);
        }
    }

    // 4. Update dữ liệu file
    FILE *data_fp = fopen(data_path, "rb");
    if (!data_fp) {
        free(sig_buf);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    unsigned char buffer[CHUNK_SIZE];
    size_t bytes_read = 0;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, data_fp)) > 0) {
        if (EVP_DigestVerifyUpdate(ctx, buffer, bytes_read) != 1) {
            fclose(data_fp);
            free(sig_buf);
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            return false;
        }
    }
    fclose(data_fp);

    // 5. Final Verify Signature
    int res = EVP_DigestVerifyFinal(ctx, sig_buf, (size_t)sig_len);

    free(sig_buf);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return (res == 1);
}

bool tizen_signature_sign_file(const char *data_path,
                               const char *privkey_pem_path,
                               const char *out_sig_path,
                               TizenSignatureAlgorithm algo) {
    if (!data_path || !privkey_pem_path || !out_sig_path) return false;

    EVP_PKEY *pkey = load_private_key(privkey_pem_path);
    if (!pkey) return false;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD *md = (algo == TIZEN_SIG_ED25519) ? NULL : EVP_sha256();

    if (EVP_DigestSignInit(ctx, NULL, md, NULL, pkey) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    FILE *data_fp = fopen(data_path, "rb");
    if (!data_fp) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    unsigned char buffer[CHUNK_SIZE];
    size_t bytes_read = 0;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, data_fp)) > 0) {
        if (EVP_DigestSignUpdate(ctx, buffer, bytes_read) != 1) {
            fclose(data_fp);
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            return false;
        }
    }
    fclose(data_fp);

    size_t sig_len = 0;
    if (EVP_DigestSignFinal(ctx, NULL, &sig_len) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    unsigned char *sig_buf = malloc(sig_len);
    if (EVP_DigestSignFinal(ctx, sig_buf, &sig_len) != 1) {
        free(sig_buf);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    FILE *sig_fp = fopen(out_sig_path, "wb");
    if (sig_fp) {
        fwrite(sig_buf, 1, sig_len, sig_fp);
        fclose(sig_fp);
    }

    free(sig_buf);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return true;
}
