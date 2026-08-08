#!/bin/bash
set -euo pipefail

# Script tạo cấu hình reprepro cho APT repository của TizenOS
# Hỗ trợ deb và udeb components

REPO_DIR="repo"
CONF_DIR="${REPO_DIR}/conf"

mkdir -p "$CONF_DIR"

# Tạo file conf/distributions
cat > "${CONF_DIR}/distributions" <<EOF
Codename: bookworm
Components: main
Architectures: amd64 arm64
UDebComponents: main
Description: TizenOS APT Repository
SignWith: default
EOF

# Tạo file conf/options
cat > "${CONF_DIR}/options" <<EOF
basedir .
ask-passphrase
EOF

# Tạo file conf/incoming
cat > "${CONF_DIR}/incoming" <<EOF
Name: default
IncomingDir: incoming
TempDir: tmp
Allow: bookworm
EOF

echo "Đã tạo xong cấu hình reprepro tại $CONF_DIR"
