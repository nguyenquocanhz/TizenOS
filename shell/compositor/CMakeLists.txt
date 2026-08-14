cmake_minimum_required(VERSION 3.16)
project(tizen-compositor C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WLROOTS REQUIRED wlroots wayland-server xkbcommon)

include_directories(src ${WLROOTS_INCLUDE_DIRS})

add_executable(tizen-compositor 
    src/compositor.c 
    src/output.c 
    src/input.c 
    src/desktop.c 
    src/xwayland.c
)
target_link_libraries(tizen-compositor ${WLROOTS_LIBRARIES})

# Thiếu install() ở bản cũ: tizen-compositor biên dịch xong rồi nằm lại trong
# thư mục build, không bao giờ vào rootfs. Cùng lỗi với shell/settings.
install(TARGETS tizen-compositor DESTINATION bin)
