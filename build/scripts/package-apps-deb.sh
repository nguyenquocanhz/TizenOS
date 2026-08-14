#!/bin/bash
# ==============================================================================
# TizenOS — Đóng gói .deb cho các ứng dụng GTK4 (chuẩn Debian Policy)
# ==============================================================================
# Tạo gói .deb RIÊNG BIỆT cho từng ứng dụng, tuân thủ Debian Policy Manual:
#
#   §9.1.1  FHS      — cài vào /usr, KHÔNG BAO GIỜ /usr/local (chỗ đó dành riêng
#                      cho quản trị viên máy; gói .deb đặt tệp vào đó là vi phạm)
#   §12.5   copyright— bắt buộc có /usr/share/doc/<gói>/copyright
#   §12.7   changelog— bắt buộc có changelog.Debian.gz, nén gzip -9
#   §5.6.7  Depends  — sinh bằng dpkg-shlibdeps chứ không viết tay
#   §10.1   binary   — strip ký hiệu gỡ lỗi
#   §3.9.1  md5sums  — kèm bảng băm cho từng tệp trong gói
#
# Kèm kiểm định: desktop-file-validate (freedesktop Desktop Entry Spec) và
# lintian cho từng gói tạo ra.
#
# Vì sao cần script riêng thay vì build-all-debs.sh:
# build-all-debs.sh chạy `make install DESTDIR=...` cho MỌI gói, nên cả 6 gói
# chứa y hệt 162 tệp như nhau. Cài hai gói bất kỳ cạnh nhau là dpkg báo
# "trying to overwrite '/usr/bin/...', which is also in package ...". Ở đây mỗi
# gói chỉ nhận đúng tệp của riêng nó.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${TIZEN_APP_BUILD_DIR:-$ROOT_DIR/build_apps}"
STAGE_ROOT="${TIZEN_APP_STAGE_DIR:-$ROOT_DIR/build/output/stage}"
OUT_DIR="$ROOT_DIR/build/output/debs"

VERSION="${TIZEN_APP_VERSION:-1.0.0-1}"
ARCH="$(dpkg --print-architecture)"
MAINTAINER="TizenOS Engineering Team <dev@tizenos.org>"
HOMEPAGE="https://github.com/nguyenquocanhz/TizenOS"

echo "=============================================================="
echo " TizenOS — Đóng gói ứng dụng GTK4 thành .deb"
echo "=============================================================="
echo " Nguồn      : $ROOT_DIR"
echo " Kết quả    : $OUT_DIR"
echo " Phiên bản  : $VERSION   Kiến trúc: $ARCH"
echo "=============================================================="

mkdir -p "$OUT_DIR"

# ------------------------------------------------------------------------------
# 1. Biên dịch Release, cài vào cây đầy đủ với prefix /usr
# ------------------------------------------------------------------------------
# CMAKE_INSTALL_PREFIX PHẢI là /usr. Mặc định của CMake là /usr/local, và mọi
# tệp rơi vào đó sẽ khiến lintian bắn lỗi dir-or-file-in-usr-local (nghiêm trọng)
# — đúng lỗi mà cây build hiện tại đang mắc.
echo
echo "[1/4] Biên dịch Release (prefix=/usr)..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr >/dev/null
make -C "$BUILD_DIR" -j"$(nproc)" tizen-album tizen-store tizen-app-manager >/dev/null

FULLTREE="$STAGE_ROOT/_fulltree"
rm -rf "$FULLTREE"
make -C "$BUILD_DIR" install DESTDIR="$FULLTREE" >/dev/null 2>&1 || true

# ------------------------------------------------------------------------------
# 2. Hàm đóng gói một ứng dụng
# ------------------------------------------------------------------------------
# $1 tên gói   $2 section   $3 mô tả ngắn   $4 Depends thêm   $5.. danh sách tệp
#              (đường dẫn tuyệt đối trong cây đã cài, phân tách bởi khoảng trắng)
build_app_deb() {
    local pkg="$1"; shift
    local section="$1"; shift
    local summary="$1"; shift
    local extra_deps="$1"; shift
    local files=("$@")

    local stage="$STAGE_ROOT/$pkg"
    rm -rf "$stage"
    mkdir -p "$stage/DEBIAN"

    echo
    echo "--------------------------------------------------------------"
    echo " Đóng gói: ${pkg}_${VERSION}_${ARCH}.deb"

    # --- Chỉ chép ĐÚNG tệp của gói này -------------------------------------
    local copied=0
    for f in "${files[@]}"; do
        if [ -e "$FULLTREE$f" ]; then
            install -Dm "$( [ -x "$FULLTREE$f" ] && echo 755 || echo 644 )" \
                    "$FULLTREE$f" "$stage$f"
            copied=$((copied + 1))
        else
            echo "   ! thiếu tệp: $f"
        fi
    done
    if [ "$copied" -eq 0 ]; then
        echo "   ✗ Không có tệp nào — bỏ qua $pkg"
        return 1
    fi

    # --- §10.1 strip nhị phân ------------------------------------------------
    find "$stage/usr/bin" -type f -exec strip --strip-unneeded {} \; 2>/dev/null || true

    # --- §12.5 copyright (bắt buộc) ------------------------------------------
    mkdir -p "$stage/usr/share/doc/$pkg"
    cat > "$stage/usr/share/doc/$pkg/copyright" <<COPYRIGHT
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: $pkg
Source: $HOMEPAGE

Files: *
Copyright: 2026 TizenOS Team
License: GPL-3.0+
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 On Debian systems, the complete text of the GNU General Public
 License version 3 can be found in "/usr/share/common-licenses/GPL-3".
COPYRIGHT

    # --- §12.7 changelog.Debian.gz (bắt buộc, nén -9) ------------------------
    cat > "$stage/usr/share/doc/$pkg/changelog.Debian" <<CHANGELOG
$pkg ($VERSION) stable; urgency=medium

  * $summary
  * Sửa lỗi giao diện tối loang lổ: bật gtk-application-prefer-dark-theme
    và dùng chung bảng token màu (tizen/theme.h).
  * Escape Pango markup cho mọi chuỗi lấy từ hệ thống.

 -- TizenOS Engineering Team <dev@tizenos.org>  $(date -R)
CHANGELOG
    gzip -9n "$stage/usr/share/doc/$pkg/changelog.Debian"

    # --- §12.1 man page --------------------------------------------------------
    local man_src="$ROOT_DIR/build/packaging/man/$pkg.1"
    if [ -f "$man_src" ]; then
        install -Dm644 "$man_src" "$stage/usr/share/man/man1/$pkg.1"
        gzip -9n "$stage/usr/share/man/man1/$pkg.1"
    fi

    # --- §5.6.7 Depends sinh tự động bằng dpkg-shlibdeps ---------------------
    # Viết tay danh sách thư viện là cách chắc chắn sẽ lệch khi hệ đích đổi
    # phiên bản. dpkg-shlibdeps đọc thẳng ELF NEEDED của nhị phân rồi tra ngược
    # ra tên gói kèm ràng buộc phiên bản tối thiểu.
    #
    # -l "$FULLTREE/usr/lib" là BẮT BUỘC: tizen-store và tizen-app-manager link
    # libtizen-core.so.1, một thư viện riêng của dự án chưa được cài vào hệ
    # thống. Không có -l thì shlibdeps DỪNG HẲN với "cannot find library" và
    # không sinh ra dòng Depends nào cả — kể cả libc6. Hệ quả là gói xuất xưởng
    # thiếu sạch phụ thuộc và lintian bắn missing-dependency-on-libc.
    # Dựng danh sách -l theo MẢNG. Nối chuỗi ở đây rất dễ hỏng: nếu
    # dpkg-architecture không trả về gì thì `-l ""` khiến shlibdeps nuốt luôn
    # đối số kế tiếp làm đường dẫn thư viện, và nhị phân cần phân tích biến mất
    # khỏi dòng lệnh — kết quả là KHÔNG gói nào có Depends, kể cả gói vốn chạy
    # tốt trước đó.
    # LƯU Ý CÚ PHÁP: dpkg-shlibdeps nhận -l ở dạng DÍNH LIỀN (-l<dir>). Viết rời
    # thành `-l <dir>` thì dpkg-shlibdeps không hiểu đó là tuỳ chọn và coi thư
    # mục như một nhị phân cần phân tích, rồi chết với
    #     error: cannot read <dir>: Is a directory
    # khiến toàn bộ Depends rỗng.
    local -a lib_paths=()
    [ -d "$FULLTREE/usr/lib" ] && lib_paths+=("-l$FULLTREE/usr/lib")
    local multiarch
    multiarch="$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true)"
    if [ -n "$multiarch" ] && [ -d "$FULLTREE/usr/lib/$multiarch" ]; then
        lib_paths+=("-l$FULLTREE/usr/lib/$multiarch")
    fi

    local deps=""
    (
        cd "$stage"
        mkdir -p debian
        printf 'Source: %s\n' "$pkg" > debian/control
        printf '\nPackage: %s\nArchitecture: %s\nDescription: x\n' "$pkg" "$ARCH" >> debian/control
        : > debian/substvars

        mapfile -t bins < <(find usr/bin -type f 2>/dev/null)
        if [ "${#bins[@]}" -gt 0 ]; then
            dpkg-shlibdeps -O "${lib_paths[@]}" --ignore-missing-info "${bins[@]}" \
                2>/dev/null | sed 's/^shlibs:Depends=//' > .deps || true
        fi
    )
    deps="$(cat "$stage/.deps" 2>/dev/null || true)"
    rm -rf "$stage/debian" "$stage/.deps"

    if [ -z "$deps" ]; then
        echo "   ! shlibdeps không sinh được Depends — kiểm tra lại đường -l"
    fi

    if [ -n "$extra_deps" ]; then
        deps="${deps:+$deps, }$extra_deps"
    fi
    echo "   Depends: $deps"

    # --- control -------------------------------------------------------------
    local installed_kb
    installed_kb="$(du -sk "$stage" | cut -f1)"

    cat > "$stage/DEBIAN/control" <<CONTROL
Package: $pkg
Version: $VERSION
Section: $section
Priority: optional
Architecture: $ARCH
Depends: $deps
Maintainer: $MAINTAINER
Homepage: $HOMEPAGE
Installed-Size: $installed_kb
Description: $summary
 Ứng dụng gốc của TizenOS (Debian Edition), viết bằng C11 với GTK4.
 Dùng chung bảng màu và theme tối của hệ thống qua libtizen-core.
CONTROL

    # --- postinst: cập nhật cache desktop/icon --------------------------------
    cat > "$stage/DEBIAN/postinst" <<'POSTINST'
#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database -q /usr/share/applications || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
    fi
fi
exit 0
POSTINST
    chmod 755 "$stage/DEBIAN/postinst"

    # --- §3.9.1 md5sums -------------------------------------------------------
    ( cd "$stage" && find . -type f ! -path './DEBIAN/*' -printf '%P\0' \
        | xargs -0 md5sum > DEBIAN/md5sums )

    # --- Kiểm định .desktop theo Desktop Entry Spec ---------------------------
    if command -v desktop-file-validate >/dev/null 2>&1; then
        for d in "$stage"/usr/share/applications/*.desktop; do
            [ -e "$d" ] || continue
            if desktop-file-validate "$d"; then
                echo "   ✓ desktop-file-validate: $(basename "$d")"
            else
                echo "   ✗ desktop-file-validate BÁO LỖI: $(basename "$d")"
            fi
        done
    fi

    # --- Build ----------------------------------------------------------------
    local out="$OUT_DIR/${pkg}_${VERSION}_${ARCH}.deb"
    dpkg-deb --build --root-owner-group "$stage" "$out" >/dev/null
    echo "   ✓ $out ($(du -h "$out" | cut -f1))"
}

# ------------------------------------------------------------------------------
# 3. Định nghĩa từng gói — danh sách tệp KHÔNG chồng lấn nhau
# ------------------------------------------------------------------------------
# fonts-noto-color-emoji nằm trong Depends vì giao diện dùng emoji làm biểu tượng
# nút ("📁 Mở thư mục", "▶️ Slideshow", "⭐ Yêu thích"...). Thiếu font, mọi nút
# hiện ô tofu ▯ và người dùng không đoán được nút nào làm gì.

echo
echo "[2/4] Đóng gói từng ứng dụng..."

# Phụ thuộc video của Album — KHÔNG phải tuỳ chọn:
#   libgtk-4-media-gstreamer  backend media cho GtkVideo; thiếu nó thì không
#                             phát được gì cả
#   gstreamer1.0-plugins-base cung cấp playbin3. Thiếu playbin3, GstPlay gọi
#                             g_error() -> abort() -> app CHẾT khi mở video
#                             (đã dựng lại được: exit 134 / SIGABRT)
#   gstreamer1.0-plugins-good bộ giải mã và muxer thông dụng
#   gstreamer1.0-libav        H.264/H.265 cho .mp4, .mkv, .mov
#   gstreamer1.0-tools        gst-inspect-1.0, dùng để dò playbin3 trước khi
#                             chạm vào GtkVideo
#   gstreamer1.0-plugins-bad  bộ giải mã phần cứng V4L2 M2M (v4l2codecs:
#                             v4l2h264dec, v4l2vp8dec...). Quan trọng với đích
#                             ARM64: SoC ARM phơi bộ giải mã video qua V4L2
#                             memory-to-memory, thiếu plugin này thì playbin3
#                             rơi về giải mã bằng CPU và 1080p giật hình.
#                             Trên x86_64 vô hại, chỉ đơn giản là không có
#                             thiết bị V4L2 M2M nào để dùng.
#   ffmpegthumbnailer         trích khung hình làm ảnh xem trước cho video;
#                             thiếu nó thì mọi video trong lưới chỉ là một biểu
#                             tượng giống hệt nhau
build_app_deb "tizen-album" "graphics" \
    "TizenOS Photo & Video Album Viewer" \
    "libgtk-4-media-gstreamer, gstreamer1.0-plugins-base, gstreamer1.0-plugins-good, gstreamer1.0-plugins-bad, gstreamer1.0-libav, gstreamer1.0-tools, ffmpegthumbnailer" \
    /usr/bin/tizen-album \
    /usr/share/applications/tizen-album.desktop

# tizen-core: shlibdeps thấy libtizen-core.so.1 qua -l nhưng không biết gói nào
# cung cấp nó (thư viện của chính dự án, chưa có tệp shlibs đăng ký), nên phải
# khai báo tay.
build_app_deb "tizen-store" "admin" \
    "TizenOS App Store — 1-Click software centre" \
    "tizen-core, fonts-noto-color-emoji, pkexec, apt" \
    /usr/bin/tizen-store \
    /usr/share/applications/tizen-store.desktop \
    /usr/share/icons/hicolor/512x512/apps/tizen-store.png \
    /usr/share/tizen-store/screenshots/tizen-album.jpg \
    /usr/share/tizen-store/screenshots/tizen-notepad.jpg

# KHÔNG khai báo Depends: dpkg. dpkg là gói Essential — Debian Policy §3.5 cấm
# phụ thuộc vào gói Essential mà không kèm ràng buộc phiên bản, và lintian coi
# đây là lỗi (depends-on-essential-package-without-using-version). Gói Essential
# luôn hiện diện nên khai báo cũng thừa.
build_app_deb "tizen-app-manager" "admin" \
    "TizenOS Application Manager for .deb and .tpk packages" \
    "tizen-core, fonts-noto-color-emoji, pkexec" \
    /usr/bin/tizen-app-manager \
    /usr/share/applications/tizen-app-manager.desktop \
    /usr/share/icons/hicolor/512x512/apps/tizen-app-manager.png

# ------------------------------------------------------------------------------
# 4. Lintian + băm kiểm tra
# ------------------------------------------------------------------------------
echo
echo "[3/4] Lintian..."
if command -v lintian >/dev/null 2>&1; then
    for p in tizen-album tizen-store tizen-app-manager; do
        f="$OUT_DIR/${p}_${VERSION}_${ARCH}.deb"
        [ -e "$f" ] || continue
        echo "--- $p ---"
        lintian --no-tag-display-limit "$f" 2>&1 | head -25 || true
    done
else
    echo "   (lintian chưa cài — bỏ qua)"
fi

echo
echo "[4/4] Sinh và kiểm tra băm..."
cd "$OUT_DIR"
# Băm cho MỌI .deb trong thư mục, không chỉ ba gói app.
# Bản cũ chỉ ghi ba dòng rồi ghi đè tệp SHA256SUMS — nếu build-all-debs.sh đã
# chạy trước, danh sách băm của 10 gói hệ thống bị xoá sạch và người tải về
# tưởng bản phát hành chỉ có ba gói. Cả hai script giờ cùng sinh danh sách đầy
# đủ, nên chạy thứ tự nào cũng ra kết quả đúng.
rm -f SHA256SUMS SHA512SUMS MD5SUMS
for f in *.deb; do
    [ -e "$f" ] || continue
    sha256sum "$f" >> SHA256SUMS
    sha512sum "$f" >> SHA512SUMS
    md5sum    "$f" >> MD5SUMS
done

echo
echo "=== Xác minh lại băm vừa ghi ==="
sha256sum -c SHA256SUMS
sha512sum -c SHA512SUMS
md5sum    -c MD5SUMS

echo
echo "=============================================================="
echo " HOÀN TẤT"
echo "=============================================================="
ls -lh "$OUT_DIR"/*.deb
