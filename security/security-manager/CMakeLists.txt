cmake_minimum_required(VERSION 3.16)
project(tizen-security-manager VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(DBUS REQUIRED dbus-1)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
)

add_executable(security-manager security-manager.c)
target_link_libraries(security-manager
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${DBUS_LIBRARIES}
)

install(TARGETS security-manager DESTINATION bin)
install(FILES tizen-security-manager.service DESTINATION /usr/lib/systemd/system)
