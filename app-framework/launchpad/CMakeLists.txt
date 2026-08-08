cmake_minimum_required(VERSION 3.10)
project(launchpad C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")

add_executable(launchpad-daemon src/launchpad-daemon.c src/launchpad-loader.c)

install(TARGETS launchpad-daemon DESTINATION bin)
install(FILES config/launchpad.service DESTINATION lib/systemd/system)
