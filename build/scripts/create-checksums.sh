#!/bin/bash
# ==============================================================================
# TizenOS Checksum & Integrity Verification Generator
# ==============================================================================
# Tự động tính toán mã băm SHA256, SHA512, MD5 và tạo chữ ký số GPG cho:
# - Các file đĩa ISO Hybrid (tizenos-live.iso)
# - Các file ảnh đĩa (.img)
# - Toàn bộ các kho gói .deb và .udeb trong build/output/
# ==============================================================================

set -euo pipefail

OUTPUT_DIR="${1:-build/output}"
KEY_ID="${GPG_KEY_ID:-}"

if [ ! -d "$OUTPUT_DIR" ]; then
    echo "[CHECKSUM-ERROR] Thư mục $OUTPUT_DIR không tồn tại!"
    exit 1
fi

echo "======================================================================"
echo "Khởi tạo Mã Băm Kiểm Tra Kiểm Định (Checksum Generator) TizenOS..."
echo "======================================================================"

cd "$OUTPUT_DIR"

# 1. Tạo SHA256SUMS
echo "[1/4] Tính toán SHA-256 Checksums..."
find . -maxdepth 2 -type f \( -name "*.iso" -o -name "*.img" -o -name "*.deb" -o -name "*.udeb" \) \
    -exec sha256sum {} + | sort -k2 > SHA256SUMS
echo "✓ Đã tạo SHA256SUMS ("$(wc -l < SHA256SUMS)" tệp)"

# 2. Tạo SHA512SUMS
echo "[2/4] Tính toán SHA-512 Checksums..."
find . -maxdepth 2 -type f \( -name "*.iso" -o -name "*.img" -o -name "*.deb" -o -name "*.udeb" \) \
    -exec sha512sum {} + | sort -k2 > SHA512SUMS
echo "✓ Đã tạo SHA512SUMS ("$(wc -l < SHA512SUMS)" tệp)"

# 3. Tạo MD5SUMS
echo "[3/4] Tính toán MD5 Checksums..."
find . -maxdepth 2 -type f \( -name "*.iso" -o -name "*.img" -o -name "*.deb" -o -name "*.udeb" \) \
    -exec md5sum {} + | sort -k2 > MD5SUMS
echo "✓ Đã tạo MD5SUMS ("$(wc -l < MD5SUMS)" tệp)"

# 4. Ký GPG Detached Signature nếu có GPG Key
echo "[4/4] Kiểm tra GPG Signature..."
if command -v gpg >/dev/null 2>&1; then
    if [ -n "$KEY_ID" ]; then
        echo "Tạo chữ ký GPG rời (SHA256SUMS.gpg) với Key ID: $KEY_ID..."
        gpg --batch --yes --default-key "$KEY_ID" --detach-sign --armor --output SHA256SUMS.gpg SHA256SUMS || true
        gpg --batch --yes --default-key "$KEY_ID" --clear-sign --output SHA256SUMS.asc SHA256SUMS || true
    else
        echo "Tạo chữ ký GPG rời (SHA256SUMS.gpg) với key mặc định..."
        gpg --batch --yes --detach-sign --armor --output SHA256SUMS.gpg SHA256SUMS 2>/dev/null || echo "(Không tìm thấy GPG key mặc định, bỏ qua bước ký số)"
    fi
else
    echo "Bỏ qua GPG signature (Chưa cài gpg)."
fi

echo "======================================================================"
echo "✓ Hoàn tất tạo Checksums cho TizenOS Distribution Artifacts!"
echo "Các tập tin đã được ghi vào: $OUTPUT_DIR/"
echo "  - SHA256SUMS"
echo "  - SHA512SUMS"
echo "  - MD5SUMS"
echo "======================================================================"
