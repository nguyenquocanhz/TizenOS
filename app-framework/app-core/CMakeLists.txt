cmake_minimum_required(VERSION 3.10)
project(app-core C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fPIC")

include_directories(include)

# Tìm thư viện Glib
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GIO2 REQUIRED gio-2.0)
pkg_check_modules(GOBJECT2 REQUIRED gobject-2.0)

include_directories(${GLIB2_INCLUDE_DIRS} ${GIO2_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS} ${GIO2_LIBRARY_DIRS})

add_library(capi-appfw-application SHARED 
    src/app-main.c 
    src/app-lifecycle.c 
    src/app-control.c 
    src/app-event.c)

target_link_libraries(capi-appfw-application ${GLIB2_LIBRARIES} ${GIO2_LIBRARIES} ${GOBJECT2_LIBRARIES})

install(TARGETS capi-appfw-application DESTINATION lib)
install(DIRECTORY include/tizen DESTINATION include)

# Cấu hình pkg-config
configure_file(capi-appfw-application.pc.in capi-appfw-application.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/capi-appfw-application.pc DESTINATION lib/pkgconfig)
