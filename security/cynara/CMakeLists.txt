cmake_minimum_required(VERSION 3.16)
project(tizen-cynara VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(DBUS REQUIRED dbus-1)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
)

# Cynara Policy Daemon Binary
add_executable(cynara-daemon cynara-daemon.c)
target_link_libraries(cynara-daemon
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${DBUS_LIBRARIES}
)

# Cynara Client Shared Library
add_library(cynara-client SHARED cynara-client.c)
target_link_libraries(cynara-client
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${DBUS_LIBRARIES}
)
set_target_properties(cynara-client PROPERTIES VERSION 1.0.0 SOVERSION 1)

# Cynara Admin CLI Tool
add_executable(cynara-admin cynara-admin.c)
target_link_libraries(cynara-admin
    cynara-client
    ${GLIB_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

# Installation Rules
install(TARGETS cynara-daemon cynara-admin DESTINATION bin)
install(TARGETS cynara-client DESTINATION lib)
install(FILES include/cynara/cynara-client.h include/cynara/cynara-admin.h DESTINATION include/cynara)
install(FILES dbus/org.tizen.cynara.conf DESTINATION /etc/dbus-1/system.d)
install(FILES tizen-cynara.service DESTINATION /usr/lib/systemd/system)
