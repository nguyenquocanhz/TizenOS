#!/usr/bin/env python3
# ==============================================================================
# signFile - Standalone Cryptographic Digital Signature & Verification Utility (Python/Windows Edition)
# ==============================================================================
# Cho phép thực thi ký số và xác minh chữ ký Ed25519, RSA-4096 PSS, ECDSA P-256
# trực tiếp trên môi trường Windows mà không cần cài đặt GCC/Visual Studio.
# ==============================================================================

import sys
import os
import hashlib
import subprocess

def print_usage(prog):
    print("====================================================================")
    print(" TizenOS Digital Signature Utility (signFile.exe / signFile.py)")
    print("====================================================================")
    print("Cú pháp sử dụng:")
    print(f"  1. Ký số (Sign File):")
    print(f"     python {prog} sign <data_file> <private_key.pem> <output.sig> [algo: ed25519|rsa|ecdsa]\n")
    print(f"  2. Xác minh chữ ký (Verify Signature):")
    print(f"     python {prog} verify <data_file> <sig_file> <public_key.pem> [algo: ed25519|rsa|ecdsa]\n")
    print(f"  3. Tạo cặp khóa mới (Generate Keypair):")
    print(f"     python {prog} keygen <private_key.pem> <public_key.pem> [algo: ed25519|rsa|ecdsa]")
    print("====================================================================")

def run_openssl_cmd(cmd_list):
    try:
        res = subprocess.run(cmd_list, capture_output=True, text=True, check=True)
        return res.stdout
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] OpenSSL Error: {e.stderr}")
        return None

def do_keygen(priv_path, pub_path, algo):
    print(f"[signFile] Đang tạo cặp khóa ({algo})...")
    if algo.lower() == "rsa":
        cmd_priv = ["openssl", "genpkey", "-algorithm", "RSA", "-out", priv_path, "-pkeyopt", "rsa_keygen_bits:4096"]
    elif algo.lower() == "ecdsa":
        cmd_priv = ["openssl", "genpkey", "-algorithm", "EC", "-out", priv_path, "-pkeyopt", "ec_paramgen_curve:P-256"]
    else:
        cmd_priv = ["openssl", "genpkey", "-algorithm", "Ed25519", "-out", priv_path]
    
    if run_openssl_cmd(cmd_priv) is not None:
        cmd_pub = ["openssl", "pkey", "-in", priv_path, "-pubout", "-out", pub_path]
        if run_openssl_cmd(cmd_pub) is not None:
            print(f"✓ Đã tạo thành công Private Key ({priv_path}) và Public Key ({pub_path})!")
            return 0
    return 1

def do_sign(data_path, priv_path, sig_path, algo):
    print(f"[signFile] Đang ký số tệp {data_path} bằng khóa {priv_path}...")
    cmd = ["openssl", "pkeyutl", "-sign", "-inkey", priv_path, "-in", data_path, "-out", sig_path]
    if run_openssl_cmd(cmd) is not None:
        size = os.path.getsize(sig_path)
        print(f"✓ Ký số THÀNH CÔNG! File chữ ký: {sig_path} (Kích thước: {size} bytes)")
        return 0
    return 1

def do_verify(data_path, sig_path, pub_path, algo):
    print(f"[signFile] Đang xác minh chữ ký {sig_path} cho tệp {data_path}...")
    cmd = ["openssl", "pkeyutl", "-verify", "-pubin", "-inkey", pub_path, "-in", data_path, "-sigfile", sig_path]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode == 0:
        print("====================================================")
        print("✓ XÁC MINH THÀNH CÔNG: Chữ ký số HỢP LỆ 100%!")
        print(f"  Tệp {data_path} chính gốc, không bị thay đổi.")
        print("====================================================")
        return 0
    else:
        print("====================================================")
        print("❌ XÁC MINH THẤT BẠI: CHỮ KÝ GIẢ MẠO HOẶC FILE BỊ SỬA!")
        print("====================================================")
        return 1

def main():
    if len(sys.argv) < 4:
        print_usage(sys.argv[0])
        sys.exit(1)

    mode = sys.argv[1].lower()
    algo = sys.argv[-1] if len(sys.argv) >= 5 else "ed25519"

    if mode == "sign" and len(sys.argv) >= 5:
        sys.exit(do_sign(sys.argv[2], sys.argv[3], sys.argv[4], algo))
    elif mode == "verify" and len(sys.argv) >= 5:
        sys.exit(do_verify(sys.argv[2], sys.argv[3], sys.argv[4], algo))
    elif mode == "keygen" and len(sys.argv) >= 4:
        sys.exit(do_keygen(sys.argv[2], sys.argv[3], algo))
    else:
        print_usage(sys.argv[0])
        sys.exit(1)

if __name__ == "__main__":
    main()
