cmake_minimum_required(VERSION 3.10)
project(libtizen-player C)

set(CMAKE_C_STANDARD 11)

add_library(tizen-player SHARED
    src/player.c
    src/player-display.c
    src/player-audio.c
)

target_include_directories(tizen-player PUBLIC include)
set_target_properties(tizen-player PROPERTIES VERSION 1.0 SOVERSION 1)

install(TARGETS tizen-player DESTINATION lib)
install(DIRECTORY include/tizen DESTINATION include)

configure_file(tizen-player.pc.in tizen-player.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tizen-player.pc DESTINATION lib/pkgconfig)
