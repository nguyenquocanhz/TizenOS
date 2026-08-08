cmake_minimum_required(VERSION 3.10)
project(tizenos-session-x11 C)

set(CMAKE_C_STANDARD 11)

find_package(X11 REQUIRED)

include_directories(${X11_INCLUDE_DIR} include)

add_executable(tizenos-session-x11 src/session-x11.c src/wm-x11.c)
target_link_libraries(tizenos-session-x11 ${X11_LIBRARIES})

install(TARGETS tizenos-session-x11 DESTINATION bin)
install(FILES config/xinitrc DESTINATION /etc/X11/xinit)
