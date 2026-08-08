#!/bin/bash
# ==============================================================================
# Cross-Compile signFile.exe for Windows 64-bit using MinGW-w64
# ==============================================================================

set -euo pipefail

echo "=== Compiling native signFile.exe for Windows 64-bit ==="

CROSS_CC="${CROSS_CC:-x86_64-w64-mingw32-gcc}"

if command -v "$CROSS_CC" >/dev/null 2>&1; then
    "$CROSS_CC" -O2 -Wall src/main.c -lssl -lcrypto -o signFile.exe
    echo "✓ Biên dịch thành công tệp thực thi Windows: signFile.exe"
else
    echo "[INFO] Chưa cài x86_64-w64-mingw32-gcc cross-compiler."
    echo "Vui lòng cài đặt: sudo apt install gcc-mingw-w64-x86-64"
    echo "Sử dụng tạm thời signFile.bat / signFile.py trên Windows!"
fi
