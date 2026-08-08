cmake_minimum_required(VERSION 3.10)
project(pkgmgr-client)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)

include_directories(include ${GLIB_INCLUDE_DIRS})

add_library(pkgmgr-client SHARED src/pkgmgr-client.c)
target_link_libraries(pkgmgr-client ${GLIB_LIBRARIES})

add_executable(tpkg src/pkgmgr-cli.c)
target_link_libraries(tpkg pkgmgr-client)

install(TARGETS pkgmgr-client DESTINATION lib)
install(TARGETS tpkg DESTINATION bin)
