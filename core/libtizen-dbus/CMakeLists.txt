cmake_minimum_required(VERSION 3.10)
project(tizen-dbus VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0 gio-unix-2.0)
pkg_check_modules(TIZEN_CORE REQUIRED tizen-core)

include_directories(include ${GLIB_INCLUDE_DIRS} ${TIZEN_CORE_INCLUDE_DIRS})

set(SOURCES
    src/dbus-conn.c
    src/dbus-signal.c
    src/dbus-method.c
)

add_library(tizen-dbus SHARED ${SOURCES})
target_link_libraries(tizen-dbus ${GLIB_LIBRARIES} ${TIZEN_CORE_LIBRARIES})
set_target_properties(tizen-dbus PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)

install(TARGETS tizen-dbus LIBRARY DESTINATION lib)
install(DIRECTORY include/tizen DESTINATION include)

configure_file(tizen-dbus.pc.in tizen-dbus.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tizen-dbus.pc DESTINATION lib/pkgconfig)
