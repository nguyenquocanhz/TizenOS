cmake_minimum_required(VERSION 3.10)
project(libtizen-camera C)

set(CMAKE_C_STANDARD 11)

add_library(tizen-camera SHARED
    src/camera.c
)

target_include_directories(tizen-camera PUBLIC include)
set_target_properties(tizen-camera PROPERTIES VERSION 1.0 SOVERSION 1)

install(TARGETS tizen-camera DESTINATION lib)
install(DIRECTORY include/tizen DESTINATION include)

configure_file(tizen-camera.pc.in tizen-camera.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tizen-camera.pc DESTINATION lib/pkgconfig)
