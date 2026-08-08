/*
 * TizenOS Checksum & Digest Module - Implementation
 * =============================================================================
 * Thực thi tính toán mã băm SHA-256, SHA-512, MD5 bằng OpenSSL EVP API.
 * Cho phép kiểm tra tính toàn vẹn của gói TPK, đĩa ISO, và các tệp hệ thống.
 * =============================================================================
 */

#include "tizen/checksum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

#define CHUNK_SIZE 8192

static const EVP_MD* get_evp_md(TizenHashType type) {
    switch (type) {
        case TIZEN_HASH_MD5:    return EVP_md5();
        case TIZEN_HASH_SHA256: return EVP_sha256();
        case TIZEN_HASH_SHA512: return EVP_sha512();
        default:                return EVP_sha256();
    }
}

bool tizen_checksum_buffer(const void *data, size_t len, TizenHashType type, char *out_hash_hex) {
    if (!data || !out_hash_hex) return false;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return false;

    const EVP_MD *md = get_evp_md(type);
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    if (EVP_DigestInit_ex(ctx, md, NULL) != 1 ||
        EVP_DigestUpdate(ctx, data, len) != 1 ||
        EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return false;
    }
    EVP_MD_CTX_free(ctx);

    for (unsigned int i = 0; i < hash_len; i++) {
        sprintf(out_hash_hex + (i * 2), "%02x", hash[i]);
    }
    out_hash_hex[hash_len * 2] = '\0';
    return true;
}

bool tizen_checksum_file(const char *filepath, TizenHashType type, char *out_hash_hex) {
    if (!filepath || !out_hash_hex) return false;

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return false;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        fclose(fp);
        return false;
    }

    const EVP_MD *md = get_evp_md(type);
    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        fclose(fp);
        return false;
    }

    unsigned char buffer[CHUNK_SIZE];
    size_t bytes_read = 0;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        if (EVP_DigestUpdate(ctx, buffer, bytes_read) != 1) {
            EVP_MD_CTX_free(ctx);
            fclose(fp);
            return false;
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        fclose(fp);
        return false;
    }

    EVP_MD_CTX_free(ctx);
    fclose(fp);

    for (unsigned int i = 0; i < hash_len; i++) {
        sprintf(out_hash_hex + (i * 2), "%02x", hash[i]);
    }
    out_hash_hex[hash_len * 2] = '\0';
    return true;
}

bool tizen_checksum_verify_file(const char *filepath, TizenHashType type, const char *expected_hash_hex) {
    if (!filepath || !expected_hash_hex) return false;

    char calculated_hash[130];
    if (!tizen_checksum_file(filepath, type, calculated_hash)) {
        return false;
    }

    return (strcasecmp(calculated_hash, expected_hash_hex) == 0);
}
