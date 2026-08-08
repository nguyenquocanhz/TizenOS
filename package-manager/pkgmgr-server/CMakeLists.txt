cmake_minimum_required(VERSION 3.10)
project(pkgmgr-server)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)
pkg_check_modules(SQLITE REQUIRED sqlite3)

include_directories(src ${GLIB_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIRS})
add_executable(tizen-pkgmgrd src/pkgmgr-daemon.c src/pkgmgr-db.c src/pkgmgr-dbus.c)
target_link_libraries(tizen-pkgmgrd ${GLIB_LIBRARIES} ${SQLITE_LIBRARIES})

install(TARGETS tizen-pkgmgrd DESTINATION sbin)
install(FILES conf/tizen-pkgmgr.conf DESTINATION /etc/dbus-1/system.d/)
install(FILES conf/tizen-pkgmgr.service DESTINATION /lib/systemd/system/)
