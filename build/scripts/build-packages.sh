#!/bin/bash
set -euo pipefail

# Mảng các gói cần build theo thứ tự
PACKAGES=("base/gpu" "core/libtizen-core" "security/smack-utils")
OUTPUT_DIR="build/output/debs"
ARCH="amd64"
JOBS=1
NO_SIGN=0
UDEB_ONLY=0

# Hàm kiểm tra các dependency cần thiết để build
check_build_deps() {
    local deps=("dpkg-buildpackage" "debhelper" "cmake" "lintian")
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            echo "Lỗi: Không tìm thấy $dep. Vui lòng cài đặt trước."
            exit 1
        fi
    done
}

# Hàm build một package cụ thể
build_single() {
    local src_dir=$1
    echo "Đang build gói từ thư mục: $src_dir"
    
    if [ ! -d "$src_dir" ]; then
        echo "Lỗi: Không tìm thấy thư mục $src_dir"
        return 1
    fi

    pushd "$src_dir" > /dev/null
    
    local build_opts="-j$JOBS -a$ARCH"
    if [ "$NO_SIGN" -eq 1 ]; then
        build_opts="$build_opts -us -uc"
    fi
    
    if ! dpkg-buildpackage $build_opts; then
        echo "Lỗi khi build $src_dir"
        popd > /dev/null
        return 1
    fi
    
    popd > /dev/null
    return 0
}

# Hàm build tất cả các packages
build_all() {
    mkdir -p "$OUTPUT_DIR"
    local success=0
    local failed=0
    local warnings=0

    # Chuyển hướng log
    exec > >(tee -a "build/output/build.log") 2>&1

    for pkg in "${PACKAGES[@]}"; do
        if build_single "$pkg"; then
            ((success++))
            # Chạy lintian để kiểm tra
            local changes_file=$(ls -1rt ../*.changes 2>/dev/null | tail -n 1)
            if [ -n "$changes_file" ]; then
                lintian "$changes_file" || ((warnings++))
                cp ../*.deb "$OUTPUT_DIR/" 2>/dev/null || true
                cp ../*.udeb "$OUTPUT_DIR/" 2>/dev/null || true
            fi
        else
            ((failed++))
        fi
    done

    echo "=================================="
    echo "Tóm tắt quá trình build:"
    echo "Thành công: $success gói"
    echo "Thất bại: $failed gói"
    echo "Cảnh báo lintian: $warnings"
    echo "=================================="
}

# Phân tích tham số dòng lệnh
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --arch=*) ARCH="${1#*=}"; shift ;;
        --jobs=*) JOBS="${1#*=}"; shift ;;
        --no-sign) NO_SIGN=1; shift ;;
        --udeb-only) UDEB_ONLY=1; shift ;;
        *) echo "Tham số không hợp lệ: $1"; exit 1 ;;
    esac
done

check_build_deps
build_all
