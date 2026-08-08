cmake_minimum_required(VERSION 3.16)
project(tizen-auth VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(OPENSSL REQUIRED openssl)

find_library(PAM_LIB pam REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

# Tizen Keyring Library (AES-256-GCM via OpenSSL)
add_library(tizen-keyring SHARED src/keyring.c)
target_link_libraries(tizen-keyring
    ${OPENSSL_LIBRARIES}
    ${GLIB_LIBRARIES}
)
set_target_properties(tizen-keyring PROPERTIES VERSION 1.0.0 SOVERSION 1)

# Auth Daemon Executable (PAM + GDBus org.tizen.Auth)
add_executable(tizen-auth-daemon src/auth-daemon.c)
target_link_libraries(tizen-auth-daemon
    ${PAM_LIB}
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
)

# Installation Rules
install(TARGETS tizen-keyring tizen-auth-daemon
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES include/tizen/tizen-keyring.h DESTINATION include/tizen)
install(FILES org.tizen.Auth.conf DESTINATION /etc/dbus-1/system.d)
install(FILES tizen-auth.service DESTINATION /usr/lib/systemd/system)
