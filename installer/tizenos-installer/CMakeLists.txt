cmake_minimum_required(VERSION 3.16)
project(tizenos-installer VERSION 1.0)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

include_directories(include)

set(SOURCES
    src/installer-ui.c
    src/partman.c
    src/user-setup.c
    src/third-party.c
    src/target-install.c
)

add_executable(tizenos-installer ${SOURCES})
target_include_directories(tizenos-installer PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_libraries(tizenos-installer PRIVATE ${GTK4_LIBRARIES} crypt)

install(TARGETS tizenos-installer DESTINATION bin)
install(FILES tizenos-installer.desktop DESTINATION share/applications)
