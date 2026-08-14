cmake_minimum_required(VERSION 3.10)
project(app-installer C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO2 REQUIRED gio-unix-2.0)

include_directories(${GIO2_INCLUDE_DIRS})
link_directories(${GIO2_LIBRARY_DIRS})

add_executable(installer-daemon 
    src/installer-daemon.c 
    src/tpk-installer.c 
    src/wgt-installer.c 
    src/deb-bridge.c)

target_link_libraries(installer-daemon ${GIO2_LIBRARIES})

install(TARGETS installer-daemon DESTINATION bin)
# Duong dan TUYET DOI /etc, khong phai "etc" tuong doi.
# Duong dan tuong doi duoc ghep sau CMAKE_INSTALL_PREFIX, nen voi prefix=/usr
# tep roi vao /usr/etc/dbus-1/system.d - cho D-Bus khong bao gio doc toi, va
# lintian coi la loi FHS. Cac component khac (cynara, tizen-lve) von da dung
# /etc tuyet doi; day la cho duy nhat con lech.
install(FILES config/org.tizen.installer.conf DESTINATION /etc/dbus-1/system.d)
install(FILES config/app-installer.service DESTINATION lib/systemd/system)
