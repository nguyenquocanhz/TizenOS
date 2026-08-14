cmake_minimum_required(VERSION 3.16)
project(tizen-shell)

# ==============================================================================
# Cổng phụ thuộc — vì sao if(EXISTS) là chưa đủ
# ==============================================================================
# Bản cũ chỉ kiểm tra TỆP CMakeLists có tồn tại hay không, mà tệp thì luôn tồn
# tại trong cây nguồn. Thư viện thiếu vẫn lọt vào add_subdirectory(), rồi
# pkg_check_modules(... REQUIRED) bên trong làm HỎNG TOÀN BỘ bước configure —
# kéo sập cả những component chẳng liên quan gì.
#
# Rất cụ thể: dựng ISO trên Debian 12 (bookworm) không có wlroots đủ mới lẫn
# gtk4-layer-shell-0. Với bản cũ, thiếu wlroots là cmake chết ngay và KHÔNG
# component nào của shell được build — mất luôn cả Files, Settings, Notepad
# vốn chỉ cần gtk4 mà bookworm có sẵn.
#
# Dò bằng QUIET trước rồi mới quyết định, để build tự giảm quy mô một cách có
# kiểm soát: cái nào đủ thư viện thì build, cái nào thiếu thì báo rõ và bỏ qua.
# ==============================================================================
find_package(PkgConfig REQUIRED)

pkg_check_modules(SHELL_GTK4     QUIET gtk4)
pkg_check_modules(SHELL_WLROOTS  QUIET wlroots wayland-server xkbcommon)
pkg_check_modules(SHELL_LAYERSH  QUIET gtk4-layer-shell-0)
pkg_check_modules(SHELL_GTKSRC   QUIET gtksourceview-5)

# component_name, thư mục, biến _FOUND cần có, mô tả thư viện còn thiếu
function(tizen_shell_add dir found_var requires)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        return()
    endif()
    if(${found_var})
        add_subdirectory(${dir})
    else()
        message(STATUS "shell: bỏ qua '${dir}' — thiếu ${requires}")
    endif()
endfunction()

# Compositor Wayland cần wlroots; bookworm thường không có bản đủ mới.
tizen_shell_add(compositor   SHELL_WLROOTS_FOUND  "wlroots + wayland-server + xkbcommon")

# session-x11 chỉ cần Xlib, không qua pkg-config -> luôn build được.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/session-x11/CMakeLists.txt")
    add_subdirectory(session-x11)
endif()

# Panel và Launcher neo vào mép màn hình bằng gtk4-layer-shell.
tizen_shell_add(panel        SHELL_LAYERSH_FOUND  "gtk4-layer-shell-0")
tizen_shell_add(launcher     SHELL_LAYERSH_FOUND  "gtk4-layer-shell-0")

# Ba cái dưới chỉ cần GTK4 (và gtksourceview cho Notepad) — chạy được trên
# bookworm, nên đừng để chúng chết theo compositor.
tizen_shell_add(file-manager SHELL_GTK4_FOUND     "gtk4")
tizen_shell_add(settings     SHELL_GTK4_FOUND     "gtk4")
tizen_shell_add(notepad      SHELL_GTKSRC_FOUND   "gtksourceview-5")
