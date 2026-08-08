cmake_minimum_required(VERSION 3.10)
project(tizen-media-server C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-pbutils-1.0)
pkg_check_modules(SQLITE REQUIRED sqlite3)

add_executable(tizen-media-server
    src/media-server.c
    src/media-db.c
    src/media-scanner.c
)

target_include_directories(tizen-media-server PRIVATE
    ${GLIB_INCLUDE_DIRS}
    ${GST_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIRS}
)

target_link_libraries(tizen-media-server
    ${GLIB_LIBRARIES}
    ${GST_LIBRARIES}
    ${SQLITE_LIBRARIES}
)

install(TARGETS tizen-media-server DESTINATION bin)
install(FILES tizen-media-server.service DESTINATION lib/systemd/system)
