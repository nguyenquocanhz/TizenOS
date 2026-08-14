cmake_minimum_required(VERSION 3.16)
project(tizen-app-manager C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
# gio-unix-2.0 cần cho GDesktopAppInfo dùng trong libtizen-core/pkg.c
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0 gio-unix-2.0)

# libtizen-core: dùng target trong cây nguồn nếu có, ngược lại dùng pkg-config.
# Cần cả hai chế độ vì component này được build hai cách:
#   - cmake từ gốc dự án  -> target tizen-core tồn tại
#   - dpkg-buildpackage trong chính thư mục này -> phải dùng libtizen-core-dev
if(TARGET tizen-core)
    set(TIZEN_CORE_LIBRARIES tizen-core)
    set(TIZEN_CORE_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/core/libtizen-core/include)
else()
    pkg_check_modules(TIZEN_CORE REQUIRED tizen-core)
endif()

include_directories(include ${GTK4_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS}
                    ${TIZEN_CORE_INCLUDE_DIRS})

add_executable(tizen-app-manager
    src/main.c
    src/installed-apps.c
    src/deb-files.c)

target_link_libraries(tizen-app-manager ${GTK4_LIBRARIES} ${GLIB_LIBRARIES} ${TIZEN_CORE_LIBRARIES})

install(TARGETS tizen-app-manager DESTINATION bin)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tizen-app-manager.desktop")
    install(FILES tizen-app-manager.desktop DESTINATION share/applications)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tizen-app-manager.png")
    install(FILES tizen-app-manager.png DESTINATION share/icons/hicolor/512x512/apps)
endif()
