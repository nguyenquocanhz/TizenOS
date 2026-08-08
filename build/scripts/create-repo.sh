#!/bin/bash
set -euo pipefail

# Script tạo APT repository từ các file .deb/.udeb đã build
# Yêu cầu cài đặt reprepro

REPO_DIR="repo"
DEBS_DIR="build/output/debs"

# Đảm bảo cấu hình reprepro tồn tại
if [ ! -d "${REPO_DIR}/conf" ]; then
    echo "Không tìm thấy thư mục conf. Đang tạo cấu hình..."
    ./build/scripts/create-repo-conf.sh
fi

pushd "$REPO_DIR" > /dev/null

# Thêm tất cả các file deb
if ls "../${DEBS_DIR}"/*.deb >/dev/null 2>&1; then
    echo "Đang thêm các file .deb vào repository..."
    reprepro includedeb bookworm "../${DEBS_DIR}"/*.deb
fi

# Thêm tất cả các file udeb
if ls "../${DEBS_DIR}"/*.udeb >/dev/null 2>&1; then
    echo "Đang thêm các file .udeb vào repository..."
    reprepro includeudeb bookworm "../${DEBS_DIR}"/*.udeb
fi

popd > /dev/null

echo "Repository đã được tạo/cập nhật thành công."
echo "Người dùng có thể thêm repo bằng cách cấu hình:"
echo "deb [signed-by=/usr/share/keyrings/tizenos-archive-keyring.gpg] https://repo.tizenos.org/debian bookworm main"
