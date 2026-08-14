cmake_minimum_required(VERSION 3.16)
project(tizen-launcher C)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4 gtk4-layer-shell-0)
add_executable(tizen-launcher src/launcher.c src/search.c)
target_link_libraries(tizen-launcher ${GTK4_LIBRARIES})

# Thiếu install() ở bản cũ — xem ghi chú ở shell/settings/CMakeLists.txt.
install(TARGETS tizen-launcher DESTINATION bin)
