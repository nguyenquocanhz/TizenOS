cmake_minimum_required(VERSION 3.16)
project(tizen-album C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0 gmodule-2.0)
pkg_check_modules(GDK_PIXBUF REQUIRED gdk-pixbuf-2.0)

# tizen/glib-compat.h là header thuần macro (không cần link libtizen-core).
# Cần cả hai chế độ vì component này được build hai cách:
#   - cmake từ gốc dự án            -> CMAKE_SOURCE_DIR trỏ tới gốc
#   - dpkg-buildpackage tại chỗ     -> phải lấy từ libtizen-core-dev đã cài
if(EXISTS "${CMAKE_SOURCE_DIR}/core/libtizen-core/include/tizen/glib-compat.h")
    set(TIZEN_CORE_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/core/libtizen-core/include")
else()
    pkg_check_modules(TIZEN_CORE REQUIRED tizen-core)
endif()

include_directories(
    include
    ${GTK4_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GDK_PIXBUF_INCLUDE_DIRS}
    ${TIZEN_CORE_INCLUDE_DIRS}
)

add_executable(tizen-album
    src/main.c
    src/album-scanner.c
    src/album-viewer.c
)

target_link_libraries(tizen-album
    ${GTK4_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GDK_PIXBUF_LIBRARIES}
    m
)

install(TARGETS tizen-album DESTINATION bin)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tizen-album.desktop")
    install(FILES tizen-album.desktop DESTINATION share/applications)
endif()
