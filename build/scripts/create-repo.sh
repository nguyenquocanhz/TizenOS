#!/bin/bash
# ==============================================================================
# TizenOS APT Repository Generator (reprepro)
# ==============================================================================
# Tự động tạo cấu hình conf và thêm các gói .deb / .udeb vào APT repo
# ==============================================================================

set -euo pipefail

REPO_DIR="repo"
DEBS_DIR="build/output/debs"

# Đảm bảo cấu hình reprepro tồn tại
if [ ! -d "${REPO_DIR}/conf" ]; then
    echo "Không tìm thấy thư mục conf. Đang tạo cấu hình..."
    chmod +x ./build/scripts/*.sh 2>/dev/null || true
    bash ./build/scripts/create-repo-conf.sh || true
fi

if [ ! -d "${REPO_DIR}/conf" ]; then
    mkdir -p "${REPO_DIR}/conf"
    cat << 'EOF' > "${REPO_DIR}/conf/distributions"
Origin: TizenOS
Label: TizenOS
Codename: bookworm
Architectures: amd64 arm64
Components: main
UDebComponents: main
Description: TizenOS Official Debian 12 Repository
EOF
fi

if command -v reprepro >/dev/null 2>&1; then
    pushd "$REPO_DIR" > /dev/null

    # Thêm tất cả các file deb
    if ls "../${DEBS_DIR}"/*.deb >/dev/null 2>&1; then
        echo "Đang thêm các file .deb vào repository..."
        reprepro includedeb bookworm "../${DEBS_DIR}"/*.deb || true
    fi

    # Thêm tất cả các file udeb
    if ls "../${DEBS_DIR}"/*.udeb >/dev/null 2>&1; then
        echo "Đang thêm các file .udeb vào repository..."
        reprepro includeudeb bookworm "../${DEBS_DIR}"/*.udeb || true
    fi

    popd > /dev/null
    echo "✓ Repository đã được tạo/cập nhật thành công."
else
    echo "[INFO] reprepro chưa được cài đặt, bỏ qua bước tạo APT Repo."
fi
