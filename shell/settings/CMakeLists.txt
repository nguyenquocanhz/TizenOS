cmake_minimum_required(VERSION 3.16)
project(tizen-settings C)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
add_executable(tizen-settings src/settings.c src/display.c src/network.c src/sound.c src/about.c)
target_link_libraries(tizen-settings ${GTK4_LIBRARIES})

# Thiếu hoàn toàn install() ở bản cũ — tizen-settings biên dịch xong rồi nằm lại
# trong thư mục build và KHÔNG BAO GIỜ vào rootfs. Phát hiện khi dựng ISO: mọi
# app khác đều có mặt, riêng Settings báo "không có /usr/bin/tizen-settings"
# dù build thành công. Cùng lý do tizen-settings.desktop chưa từng được cài,
# nên nó cũng không hiện trong menu ứng dụng.
install(TARGETS tizen-settings DESTINATION bin)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tizen-settings.desktop")
    install(FILES tizen-settings.desktop DESTINATION share/applications)
endif()
