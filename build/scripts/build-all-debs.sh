#!/bin/bash
# ==============================================================================
# TizenOS — Đóng gói toàn hệ thống thành .deb (build-all-debs.sh)
# ==============================================================================
# Biên dịch toàn bộ TizenOS và chia thành các gói .deb KHÔNG CHỒNG LẤN nhau,
# tuân thủ Debian Policy.
#
# LỖI CỦA BẢN CŨ — vì sao phải viết lại
# --------------------------------------
# Bản cũ gọi `make install DESTDIR=$PKG_STAGE` cho MỖI gói. CMake Makefile không
# có bộ lọc theo target, nên lệnh đó cài TOÀN BỘ dự án vào mọi thư mục staging:
# cả 6 gói xuất xưởng chứa y hệt 162 tệp giống nhau (kích thước chênh nhau vài
# chục byte, chỉ do metadata). Cài hai gói bất kỳ cạnh nhau là dpkg từ chối:
#
#     dpkg: error processing archive tizen-shell.deb
#      trying to overwrite '/usr/bin/tizenos-mount-iso',
#      which is also in package tizen-core
#
# Nghĩa là bộ .deb đó chưa bao giờ cài được quá một gói — lỗi chặn hoàn toàn.
#
# CÁCH SỬA
# --------
# CMake sinh sẵn một cmake_install.cmake trong TỪNG thư mục con của cây build,
# và mỗi tệp đó chỉ cài đúng target của thư mục ấy. Chạy riêng từng cái với
# DESTDIR cho ra đúng phần thuộc về component đó:
#
#     DESTDIR=<stage> cmake -P <builddir>/core/libtizen-core/cmake_install.cmake
#
# Ưu điểm so với việc chép tay danh sách tệp: manifest KHÔNG THỂ lệch. Thêm một
# target vào CMakeLists nào thì nó tự vào đúng gói của component đó, không cần
# ai nhớ cập nhật script này.
#
# Script tự KIỂM TRA chồng lấn ở cuối và thoát với lỗi nếu phát hiện — để lỗi
# cũ không thể lặng lẽ quay lại.
#
# Ba app GTK4 (Album, App Store, App Manager) được đóng gói riêng bởi
# build/scripts/package-apps-deb.sh với man page và phụ thuộc đầy đủ, nên chúng
# bị loại khỏi các gói hệ thống ở đây.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${TIZEN_BUILD_DIR:-$ROOT_DIR/build_deb_staging}"
STAGE_ROOT="${TIZEN_STAGE_DIR:-$ROOT_DIR/build/output/stage-system}"
OUT_DIR="$ROOT_DIR/build/output/debs"

VERSION="${TIZEN_VERSION:-1.0.0-1}"
ARCH="$(dpkg --print-architecture)"
MAINTAINER="TizenOS Engineering Team <dev@tizenos.org>"
HOMEPAGE="https://github.com/nguyenquocanhz/TizenOS"

echo "======================================================================"
echo " TizenOS — Đóng gói .deb toàn hệ thống"
echo "======================================================================"
echo " Nguồn      : $ROOT_DIR"
echo " Kết quả    : $OUT_DIR"
echo " Phiên bản  : $VERSION    Kiến trúc: $ARCH"
echo "======================================================================"

mkdir -p "$OUT_DIR"
rm -rf "$STAGE_ROOT"
mkdir -p "$STAGE_ROOT"

# ------------------------------------------------------------------------------
# 1. Biên dịch một lần, prefix /usr
# ------------------------------------------------------------------------------
# CMAKE_INSTALL_PREFIX PHẢI là /usr. Mặc định của CMake là /usr/local, và Debian
# Policy §9.1.1 cấm gói .deb đặt tệp ở đó (chỗ đó dành riêng cho quản trị viên
# máy) — lintian bắn dir-or-file-in-usr-local.
echo
echo "[1/5] Biên dịch Release (prefix=/usr)..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr >/dev/null
make -C "$BUILD_DIR" -j"$(nproc)" >/dev/null

# ------------------------------------------------------------------------------
# 2. Hàm đóng gói một gói từ danh sách COMPONENT
# ------------------------------------------------------------------------------
# $1 tên gói  $2 section  $3 mô tả  $4 Depends  $5.. thư mục component trong cây build
declare -a BUILT_PACKAGES=()

build_deb_package() {
    local pkg="$1"; shift
    local section="$1"; shift
    local summary="$1"; shift
    local deps="$1"; shift
    local components=("$@")

    local stage="$STAGE_ROOT/$pkg"
    rm -rf "$stage"
    mkdir -p "$stage/DEBIAN"

    echo
    echo "----------------------------------------------------------------------"
    echo " Đóng gói: ${pkg}_${VERSION}_${ARCH}.deb"

    # --- Cài ĐÚNG các component của gói này ---------------------------------
    local found=0
    for comp in "${components[@]}"; do
        local script="$BUILD_DIR/$comp/cmake_install.cmake"
        if [ -f "$script" ]; then
            DESTDIR="$stage" cmake -P "$script" >/dev/null
            found=$((found + 1))
        else
            echo "   ! không có component: $comp"
        fi
    done
    if [ "$found" -eq 0 ]; then
        echo "   ✗ Không cài được component nào — bỏ qua $pkg"
        return 0
    fi

    # --- Loại 3 app có gói riêng (package-apps-deb.sh) -----------------------
    rm -f "$stage/usr/bin/tizen-album" \
          "$stage/usr/bin/tizen-store" \
          "$stage/usr/bin/tizen-app-manager"
    rm -f "$stage/usr/share/applications/tizen-album.desktop" \
          "$stage/usr/share/applications/tizen-store.desktop" \
          "$stage/usr/share/applications/tizen-app-manager.desktop"
    rm -f "$stage/usr/share/icons/hicolor/512x512/apps/tizen-store.png" \
          "$stage/usr/share/icons/hicolor/512x512/apps/tizen-app-manager.png"
    rm -rf "$stage/usr/share/tizen-store"
    # -path ./DEBIAN -prune: DEBIAN vẫn rỗng ở thời điểm này (control/postinst
    # được ghi ở dưới), nên một lệnh xoá thư mục rỗng không loại trừ nó sẽ xoá
    # mất chính thư mục metadata và mọi lệnh ghi sau đó thất bại.
    find "$stage" -path "$stage/DEBIAN" -prune -o -type d -empty -print0 2>/dev/null \
        | xargs -0 -r rmdir 2>/dev/null || true

    if [ -z "$(find "$stage" -type f ! -path '*/DEBIAN/*' 2>/dev/null)" ]; then
        echo "   ✗ Rỗng sau khi loại app — bỏ qua $pkg"
        rm -rf "$stage"
        return 0
    fi

    # --- §10.1 strip nhị phân và thư viện ------------------------------------
    find "$stage" -type f \( -perm -u+x -o -name '*.so*' \) \
        -exec strip --strip-unneeded {} \; 2>/dev/null || true

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

    # --- §12.7 changelog.Debian.gz (bắt buộc, gzip -9) -----------------------
    cat > "$stage/usr/share/doc/$pkg/changelog.Debian" <<CHANGELOG
$pkg ($VERSION) stable; urgency=medium

  * $summary
  * Chia gói theo component: mỗi gói chỉ chứa tệp của riêng nó, không còn
    chồng lấn khiến dpkg từ chối cài song song.

 -- TizenOS Engineering Team <dev@tizenos.org>  $(date -R)
CHANGELOG
    gzip -9n "$stage/usr/share/doc/$pkg/changelog.Debian"

    # --- §5.6.7 Depends sinh bằng dpkg-shlibdeps ------------------------------
    # -l dạng DÍNH LIỀN (-l<dir>): viết rời `-l <dir>` thì shlibdeps coi thư mục
    # là nhị phân cần phân tích rồi chết với "cannot read <dir>: Is a directory",
    # và Depends rỗng sạch — kể cả libc6.
    local -a lib_paths=()
    [ -d "$STAGE_ROOT" ] && lib_paths+=("-l$STAGE_ROOT/tizen-core/usr/lib")
    [ -d "$stage/usr/lib" ] && lib_paths+=("-l$stage/usr/lib")

    local shlib_deps=""
    (
        cd "$stage"
        mkdir -p debian
        printf 'Source: %s\n\nPackage: %s\nArchitecture: %s\nDescription: x\n' \
            "$pkg" "$pkg" "$ARCH" > debian/control
        : > debian/substvars
        mapfile -t bins < <(find usr/bin usr/sbin usr/lib -type f \
            \( -perm -u+x -o -name '*.so*' \) 2>/dev/null)
        if [ "${#bins[@]}" -gt 0 ]; then
            dpkg-shlibdeps -O "${lib_paths[@]}" --ignore-missing-info "${bins[@]}" \
                2>/dev/null | sed 's/^shlibs:Depends=//' > .deps || true
        fi
    )
    shlib_deps="$(cat "$stage/.deps" 2>/dev/null || true)"
    rm -rf "$stage/debian" "$stage/.deps"

    local all_deps="$shlib_deps"
    [ -n "$deps" ] && all_deps="${all_deps:+$all_deps, }$deps"
    echo "   Depends: ${all_deps:-(không có)}"

    # --- control --------------------------------------------------------------
    local installed_kb
    installed_kb="$(du -sk "$stage" | cut -f1)"
    cat > "$stage/DEBIAN/control" <<CONTROL
Package: $pkg
Version: $VERSION
Section: $section
Priority: optional
Architecture: $ARCH
Depends: $all_deps
Maintainer: $MAINTAINER
Homepage: $HOMEPAGE
Installed-Size: $installed_kb
Description: $summary
 Thành phần hệ thống TizenOS 1.0 (Debian Edition) — bản phân phối Linux
 desktop với kiến trúc bảo mật 7 lớp và giao diện GTK4.
CONTROL

    # --- §10.7 conffiles — mọi tệp trong /etc phải được khai báo --------------
    # Không khai báo thì dpkg coi chúng là tệp thường và GHI ĐÈ KHÔNG HỎI khi
    # nâng cấp, xoá sạch cấu hình quản trị viên đã sửa. Debian Policy bắt buộc,
    # và lintian coi thiếu là lỗi (file-in-etc-not-marked-as-conffile).
    if [ -d "$stage/etc" ]; then
        ( cd "$stage" && find etc -type f -printf '/%p\n' | sort ) > "$stage/DEBIAN/conffiles"
        echo "   conffiles: $(wc -l < "$stage/DEBIAN/conffiles") tệp"
    fi

    # --- Thư viện dùng chung: trigger ldconfig + shlibs -----------------------
    # Gọi thẳng `ldconfig` trong postinst là cách làm cũ: nó chạy lại một lần
    # cho MỖI gói được cài, thay vì gộp một lần ở cuối giao dịch dpkg.
    # Cách đúng là khai báo trigger — dpkg tự gom và chạy ldconfig đúng một lần.
    # Thiếu nó, lintian bắn lacks-ldconfig-trigger (lỗi).
    local -a solibs=()
    mapfile -t solibs < <(find "$stage/usr/lib" "$stage/lib" -maxdepth 2 -name '*.so*' \
                          -type f 2>/dev/null || true)

    local needs_ldconfig=0
    if [ "${#solibs[@]}" -gt 0 ]; then
        needs_ldconfig=1
        printf 'activate-noawait ldconfig\n' > "$stage/DEBIAN/triggers"

        # shlibs: cho các gói KHÁC dùng dpkg-shlibdeps phân giải được thư viện
        # của ta thành tên gói. Thiếu tệp này thì thư viện public không có gói
        # nào "khai chủ quyền", lintian bắn no-shlibs (lỗi).
        # Định dạng mỗi dòng: <tên-lib> <soversion> <gói> (>= <phiên bản>)
        : > "$stage/DEBIAN/shlibs"
        for lib in "${solibs[@]}"; do
            local soname
            soname="$(objdump -p "$lib" 2>/dev/null \
                      | awk '/SONAME/ {print $2; exit}')"
            # Chỉ thư viện CÓ SONAME mới vào shlibs. Bản .so không phiên bản là
            # symlink dev hoặc plugin nội bộ, không phải ABI công khai.
            [ -z "$soname" ] && continue
            local libname="${soname%%.so.*}"
            local soversion="${soname#*.so.}"
            [ "$soname" = "$soversion" ] && continue   # không có .so.N -> bỏ
            printf '%s %s %s (>= %s)\n' \
                "$libname" "$soversion" "$pkg" "${VERSION%-*}" >> "$stage/DEBIAN/shlibs"
        done
        sort -u -o "$stage/DEBIAN/shlibs" "$stage/DEBIAN/shlibs"
        [ -s "$stage/DEBIAN/shlibs" ] || rm -f "$stage/DEBIAN/shlibs"
    fi

    # --- postinst -------------------------------------------------------------
    # KHÔNG gọi ldconfig ở đây — đã có trigger ở trên lo việc đó.
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
    if command -v systemctl >/dev/null 2>&1; then
        systemctl daemon-reload || true
    fi
fi
exit 0
POSTINST
    chmod 755 "$stage/DEBIAN/postinst"

    # --- §3.9.1 md5sums -------------------------------------------------------
    ( cd "$stage" && find . -type f ! -path './DEBIAN/*' -printf '%P\0' \
        | xargs -0 md5sum > DEBIAN/md5sums )

    # --- Build ----------------------------------------------------------------
    local out="$OUT_DIR/${pkg}_${VERSION}_${ARCH}.deb"
    dpkg-deb --build --root-owner-group "$stage" "$out" >/dev/null
    echo "   ✓ $out ($(du -h "$out" | cut -f1))"
    BUILT_PACKAGES+=("$pkg")
}

# ------------------------------------------------------------------------------
# 3. Định nghĩa các gói
# ------------------------------------------------------------------------------
echo
echo "[2/5] Đóng gói từng thành phần..."

# tizen-core phải đóng gói TRƯỚC: các gói sau dùng thư viện của nó để
# dpkg-shlibdeps phân giải được libtizen-core.so.1.
build_deb_package "tizen-core" "libs" \
    "TizenOS core C runtime and D-Bus libraries" \
    "" \
    core/libtizen-core core/libtizen-dbus

build_deb_package "tizen-base-gpu" "admin" \
    "TizenOS GPU auto-detection and display fallback" \
    "" \
    base/gpu

build_deb_package "tizen-security" "admin" \
    "TizenOS 7-layer security subsystem (Smack, Cynara, Security Manager, Auth)" \
    "tizen-core" \
    security/smack-utils security/cynara security/security-manager security/tizen-auth

build_deb_package "tizen-app-framework" "admin" \
    "TizenOS application lifecycle framework and Launchpad pre-fork pool" \
    "tizen-core" \
    app-framework/app-core app-framework/app-installer app-framework/launchpad

build_deb_package "tizen-package-manager" "admin" \
    "TizenOS dual package manager (TPK native and Debian APT bridge)" \
    "tizen-core, dpkg-dev, apt" \
    package-manager/pkgmgr-client package-manager/pkgmgr-server package-manager/tpk-tools

build_deb_package "tizen-multimedia" "video" \
    "TizenOS multimedia framework (player, camera, media indexing daemon)" \
    "tizen-core" \
    multimedia/player multimedia/camera multimedia/media-server

build_deb_package "tizen-shell" "x11" \
    "TizenOS GTK4 desktop shell (compositor, panel, launcher, files, settings)" \
    "tizen-core" \
    shell/compositor shell/session-x11 shell/panel shell/launcher \
    shell/file-manager shell/settings shell/notepad

build_deb_package "tizen-resource-manager" "admin" \
    "TizenOS LVE resource manager (per-user cgroup v2 limits)" \
    "tizen-core" \
    resource-manager/tizen-lve resource-manager/tizen-lve-manager

build_deb_package "tizen-installer" "admin" \
    "TizenOS graphical OS installer and first-boot welcome wizard" \
    "tizen-core, parted, rsync" \
    installer/tizenos-installer

build_deb_package "tizen-tools" "utils" \
    "TizenOS system utilities (signFile, archive mount, screenshot, benchmark)" \
    "tizen-core" \
    tools/signFile tools/archive-mount tools/benchmark-launchpad \
    tools/tizenos-script-runner tools/tizenos-app-updater

# ------------------------------------------------------------------------------
# 4. Kiểm tra chồng lấn — cổng chặn, không phải cảnh báo
# ------------------------------------------------------------------------------
# Đây chính là lỗi mà bản cũ mắc phải. Kiểm tra bằng máy để nó không thể
# lặng lẽ quay lại: bất kỳ tệp nào xuất hiện ở hai gói -> thoát khác 0.
echo
echo "[3/5] Kiểm tra chồng lấn tệp giữa các gói..."
OVERLAP_LIST="$STAGE_ROOT/.all-files"
: > "$OVERLAP_LIST"
for pkg in "${BUILT_PACKAGES[@]}"; do
    dpkg-deb -c "$OUT_DIR/${pkg}_${VERSION}_${ARCH}.deb" \
        | awk '{print $6}' | grep -v '/$' | sed 's|^\./|/|' \
        | grep -v '^/usr/share/doc/' \
        | sed "s|\$| $pkg|" >> "$OVERLAP_LIST"
done

DUPES="$(awk '{print $1}' "$OVERLAP_LIST" | sort | uniq -d || true)"
if [ -n "$DUPES" ]; then
    echo "   ✗ PHÁT HIỆN CHỒNG LẤN — dpkg sẽ từ chối cài song song:"
    while IFS= read -r f; do
        printf '      %-52s <- %s\n' "$f" \
            "$(grep -F "$f " "$OVERLAP_LIST" | awk '{print $2}' | tr '\n' ' ')"
    done <<< "$DUPES"
    exit 1
fi
echo "   ✓ Không có tệp nào thuộc hai gói ($(wc -l < "$OVERLAP_LIST") tệp / ${#BUILT_PACKAGES[@]} gói)"

# ------------------------------------------------------------------------------
# 5. Lintian + băm kiểm tra
# ------------------------------------------------------------------------------
echo
echo "[4/5] Lintian..."
if command -v lintian >/dev/null 2>&1; then
    for pkg in "${BUILT_PACKAGES[@]}"; do
        f="$OUT_DIR/${pkg}_${VERSION}_${ARCH}.deb"
        out="$(lintian --tag-display-limit 0 "$f" 2>&1 | grep -E '^(E|W):' || true)"
        errs="$(echo "$out" | grep -c '^E:' || true)"
        echo "--- $pkg (lỗi: $errs) ---"
        [ -n "$out" ] && echo "$out" | head -12
    done
else
    echo "   (lintian chưa cài — bỏ qua)"
fi

echo
echo "[5/5] Sinh và xác minh băm..."
cd "$OUT_DIR"
rm -f SHA256SUMS SHA512SUMS MD5SUMS
for f in *.deb; do
    sha256sum "$f" >> SHA256SUMS
    sha512sum "$f" >> SHA512SUMS
    md5sum    "$f" >> MD5SUMS
done
sha256sum -c SHA256SUMS >/dev/null && echo "   ✓ SHA256 khớp"
sha512sum -c SHA512SUMS >/dev/null && echo "   ✓ SHA512 khớp"
md5sum    -c MD5SUMS    >/dev/null && echo "   ✓ MD5 khớp"

echo
echo "======================================================================"
echo " HOÀN TẤT — ${#BUILT_PACKAGES[@]} gói hệ thống"
echo "======================================================================"
ls -lh "$OUT_DIR"/*.deb
