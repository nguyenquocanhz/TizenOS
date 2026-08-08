cmake_minimum_required(VERSION 3.10)
project(TizenOSFiles)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

add_executable(tizenos-files src/file-manager.c)
target_include_directories(tizenos-files PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_libraries(tizenos-files PRIVATE ${GTK4_LIBRARIES})

install(TARGETS tizenos-files DESTINATION /usr/bin)
