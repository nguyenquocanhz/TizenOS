/*
 * signFile - Standalone Cryptographic Digital Signature & Verification Utility
 * =============================================================================
 * Lệnh CLI ký số và xác minh chữ ký tệp bằng Ed25519, RSA-PSS, và ECDSA P-256.
 * Sử dụng OpenSSL 3.0 EVP Cryptographic API.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#define CHUNK_SIZE 8192

static void print_usage(const char *prog) {
    printf("====================================================================\n");
    printf(" TizenOS Digital Signature Utility (signFile)\n");
    printf("====================================================================\n");
    printf("Cú pháp sử dụng:\n");
    printf("  1. Ký số (Sign File):\n");
    printf("     %s sign <data_file> <private_key.pem> <output.sig> [algo: ed25519|rsa|ecdsa]\n\n", prog);
    printf("  2. Xác minh chữ ký (Verify Signature):\n");
    printf("     %s verify <data_file> <sig_file> <public_key.pem> [algo: ed25519|rsa|ecdsa]\n\n", prog);
    printf("  3. Tạo cặp khóa mới (Generate Keypair):\n");
    printf("     %s keygen <private_key.pem> <public_key.pem> [algo: ed25519|rsa|ecdsa]\n", prog);
    printf("====================================================================\n");
}

static const EVP_MD* get_md_by_algo(const char *algo) {
    if (algo && strcasecmp(algo, "ed25519") == 0) return NULL; // Ed25519 không dùng pre-hash MD
    return EVP_sha256();
}

static EVP_PKEY* load_private_key(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] Không thể mở Private Key file: %s\n", path);
        return NULL;
    }
    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);
    return pkey;
}

static EVP_PKEY* load_public_key(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] Không thể mở Public Key file: %s\n", path);
        return NULL;
    }
    EVP_PKEY *pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);
    return pkey;
}

/* ---- 1. Lệnh Ký số (Sign File) ---------------------------------------------- */

static int do_sign(const char *data_path, const char *priv_path, const char *sig_path, const char *algo) {
    printf("[signFile] Đang ký số tệp %s bằng khóa %s...\n", data_path, priv_path);

    EVP_PKEY *pkey = load_private_key(priv_path);
    if (!pkey) return 1;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD *md = get_md_by_algo(algo);

    if (EVP_DigestSignInit(ctx, NULL, md, NULL, pkey) != 1) {
        fprintf(stderr, "[ERROR] EVP_DigestSignInit thất bại.\n");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 1;
    }

    FILE *dfp = fopen(data_path, "rb");
    if (!dfp) {
        fprintf(stderr, "[ERROR] Không thể mở tệp dữ liệu %s\n", data_path);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 1;
    }

    unsigned char buf[CHUNK_SIZE];
    size_t n = 0;
    while ((n = fread(buf, 1, CHUNK_SIZE, dfp)) > 0) {
        if (EVP_DigestSignUpdate(ctx, buf, n) != 1) {
            fclose(dfp);
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            return 1;
        }
    }
    fclose(dfp);

    size_t sig_len = 0;
    if (EVP_DigestSignFinal(ctx, NULL, &sig_len) != 1) {
        fprintf(stderr, "[ERROR] EVP_DigestSignFinal get len thất bại.\n");
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 1;
    }

    unsigned char *sig_buf = malloc(sig_len);
    if (EVP_DigestSignFinal(ctx, sig_buf, &sig_len) != 1) {
        fprintf(stderr, "[ERROR] EVP_DigestSignFinal sign thất bại.\n");
        free(sig_buf);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 1;
    }

    FILE *sfp = fopen(sig_path, "wb");
    if (!sfp) {
        fprintf(stderr, "[ERROR] Không thể ghi file chữ ký %s\n", sig_path);
        free(sig_buf);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 1;
    }
    fwrite(sig_buf, 1, sig_len, sfp);
    fclose(sfp);

    free(sig_buf);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    printf("✓ Ký số THÀNH CÔNG! File chữ ký: %s (Kích thước: %zu bytes)\n", sig_path, sig_len);
    return 0;
}

/* ---- 2. Lệnh Xác minh Chữ ký (Verify File) ---------------------------------- */

static int do_verify(const char *data_path, const char *sig_path, const char *pub_path, const char *algo) {
    printf("[signFile] Đang xác minh chữ ký %s cho tệp %s...\n", sig_path, data_path);

    EVP_PKEY *pkey = load_public_key(pub_path);
    if (!pkey) return 1;

    FILE *sfp = fopen(sig_path, "rb");
    if (!sfp) {
        fprintf(stderr, "[ERROR] Không thể mở file chữ ký %s\n", sig_path);
        EVP_PKEY_free(pkey);
        return 1;
    }
    fseek(sfp, 0, SEEK_END);
    long sig_len = ftell(sfp);
    fseek(sfp, 0, SEEK_SET);

    unsigned char *sig_buf = malloc((size_t)sig_len);
    if (!sig_buf || fread(sig_buf, 1, (size_t)sig_len, sfp) != (size_t)sig_len) {
        fclose(sfp);
        free(sig_buf);
        EVP_PKEY_free(pkey);
        return 1;
    }
    fclose(sfp);

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD *md = get_md_by_algo(algo);

    if (EVP_DigestVerifyInit(ctx, NULL, md, NULL, pkey) != 1) {
        free(sig_buf);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 1;
    }

    FILE *dfp = fopen(data_path, "rb");
    if (!dfp) {
        free(sig_buf);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 1;
    }

    unsigned char buf[CHUNK_SIZE];
    size_t n = 0;
    while ((n = fread(buf, 1, CHUNK_SIZE, dfp)) > 0) {
        if (EVP_DigestVerifyUpdate(ctx, buf, n) != 1) {
            fclose(dfp);
            free(sig_buf);
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            return 1;
        }
    }
    fclose(dfp);

    int res = EVP_DigestVerifyFinal(ctx, sig_buf, (size_t)sig_len);

    free(sig_buf);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    if (res == 1) {
        printf("====================================================\n");
        printf("✓ XÁC MINH THÀNH CÔNG: Chữ ký số HỢP LỆ 100%%!\n");
        printf("  Tệp %s chính gốc, không bị thay đổi.\n", data_path);
        printf("====================================================\n");
        return 0;
    } else {
        printf("====================================================\n");
        printf("❌ XÁC MINH THẤT BẠI: CHỮ KÝ GIẢ MẠO HOẶC FILE BỊ SỬA!\n");
        printf("====================================================\n");
        return 1;
    }
}

/* ---- 3. Lệnh Tạo Cặp Khóa (Keygen) ------------------------------------------ */

static int do_keygen(const char *priv_path, const char *pub_path, const char *algo_name) {
    printf("[signFile] Đang tạo cặp khóa (%s)...\n", algo_name);
    EVP_PKEY_CTX *pctx = NULL;
    EVP_PKEY *pkey = NULL;

    if (strcasecmp(algo_name, "rsa") == 0) {
        pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
        EVP_PKEY_keygen_init(pctx);
        EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 4096);
        EVP_PKEY_keygen(pctx, &pkey);
    } else if (strcasecmp(algo_name, "ecdsa") == 0) {
        pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
        EVP_PKEY_keygen_init(pctx);
        EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1);
        EVP_PKEY_keygen(pctx, &pkey);
    } else {
        // Mặc định Ed25519 (EdDSA 256-bit)
        pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
        EVP_PKEY_keygen_init(pctx);
        EVP_PKEY_keygen(pctx, &pkey);
    }
    EVP_PKEY_CTX_free(pctx);

    if (!pkey) {
        fprintf(stderr, "[ERROR] Tạo cặp khóa thất bại.\n");
        return 1;
    }

    FILE *priv_fp = fopen(priv_path, "w");
    PEM_write_PrivateKey(priv_fp, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(priv_fp);

    FILE *pub_fp = fopen(pub_path, "w");
    PEM_write_PUBKEY(pub_fp, pkey);
    fclose(pub_fp);

    EVP_PKEY_free(pkey);
    printf("✓ Đã tạo thành công Private Key (%s) và Public Key (%s)!\n", priv_path, pub_path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    const char *algo = (argc >= 5) ? argv[argc - 1] : "ed25519";

    if (strcmp(mode, "sign") == 0) {
        if (argc < 5) { print_usage(argv[0]); return 1; }
        return do_sign(argv[2], argv[3], argv[4], algo);
    }
    else if (strcmp(mode, "verify") == 0) {
        if (argc < 5) { print_usage(argv[0]); return 1; }
        return do_verify(argv[2], argv[3], argv[4], algo);
    }
    else if (strcmp(mode, "keygen") == 0) {
        return do_keygen(argv[2], argv[3], algo);
    }
    else {
        print_usage(argv[0]);
        return 1;
    }
}
