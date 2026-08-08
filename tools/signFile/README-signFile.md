# Công cụ Ký số & Xác minh Chữ ký (`signFile`)

`signFile` là công cụ Command-Line (CLI) độc lập chuyên dụng của TizenOS dùng để:
1. **Tạo cặp khóa Ký số (Keygen)**: Ed25519 (Mặc định), RSA 4096-bit, hoặc ECDSA (secp256r1).
2. **Ký số bất kỳ Tệp đĩa (Sign File)**: Tạo file chữ ký số `.sig`.
3. **Xác minh Chữ ký (Verify Signature)**: Đảm bảo tính toàn vẹn 100%, chống giả mạo file đĩa.

---

## 🛠️ 1. Hướng dẫn Lệnh Sử dụng

### A. Tạo cặp khóa mới (Keygen)
```bash
# Tạo cặp khóa Ed25519 (Nhanh & Bảo mật cao nhất)
signFile keygen private_key.pem public_key.pem ed25519

# Tạo cặp khóa RSA 4096-bit
signFile keygen rsa_priv.pem rsa_pub.pem rsa
```

### B. Ký số Tệp đĩa (Sign File)
```bash
# Ký số tệp đĩa ISO hoặc ứng dụng .tpk
signFile sign tizenos-live.iso private_key.pem tizenos-live.iso.sig ed25519
```

### C. Xác minh Chữ ký (Verify Signature)
```bash
# Kiểm tra tệp đĩa có bị chỉnh sửa hay giả mạo hay không
signFile verify tizenos-live.iso tizenos-live.iso.sig public_key.pem ed25519
```

**Kết quả hiển thị:**
```text
====================================================
✓ XÁC MINH THÀNH CÔNG: Chữ ký số HỢP LỆ 100%!
  Tệp tizenos-live.iso chính gốc, không bị thay đổi.
====================================================
```
