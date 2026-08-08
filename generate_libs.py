import os

base_dir = "d:/TizenOS"

files = {
    # libtizen-core
    "core/libtizen-core/CMakeLists.txt": """\
cmake_minimum_required(VERSION 3.10)
project(tizen-core VERSION 1.0.0 LANGUAGES C)

# C99/C11 standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(LIBSYSTEMD REQUIRED libsystemd)

include_directories(include ${GLIB_INCLUDE_DIRS} ${LIBSYSTEMD_INCLUDE_DIRS})

set(SOURCES
    src/core.c
    src/log.c
    src/utils.c
)

add_library(tizen-core SHARED ${SOURCES})
target_link_libraries(tizen-core ${GLIB_LIBRARIES} ${LIBSYSTEMD_LIBRARIES})
set_target_properties(tizen-core PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)

# Install
install(TARGETS tizen-core LIBRARY DESTINATION lib)
install(DIRECTORY include/tizen DESTINATION include)

# Pkg-config
configure_file(tizen-core.pc.in tizen-core.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tizen-core.pc DESTINATION lib/pkgconfig)
""",
    
    "core/libtizen-core/tizen-core.pc.in": """\
prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: tizen-core
Description: Tizen Core Library
Version: @PROJECT_VERSION@
Requires: glib-2.0 libsystemd
Libs: -L${libdir} -ltizen-core
Cflags: -I${includedir}
""",
    "core/libtizen-core/include/tizen/tizen.h": """\
#ifndef __TIZEN_H__
#define __TIZEN_H__

// Định nghĩa chung cho thư viện tizen-core
#include <tizen/types.h>
#include <tizen/error.h>
#include <tizen/log.h>

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo thư viện tizen core
int tizen_core_init(void);

// Dọn dẹp tài nguyên thư viện
void tizen_core_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_H__
""",
    "core/libtizen-core/include/tizen/types.h": """\
#ifndef __TIZEN_TYPES_H__
#define __TIZEN_TYPES_H__

#include <stdint.h>
#include <stdbool.h>

// Định nghĩa các kiểu dữ liệu cơ bản cho TizenOS
typedef int32_t tizen_error_t;

#endif // __TIZEN_TYPES_H__
""",
    "core/libtizen-core/include/tizen/error.h": """\
#ifndef __TIZEN_ERROR_H__
#define __TIZEN_ERROR_H__

#include <tizen/types.h>

// Các mã lỗi chuẩn (Unified error handling macros)
#define TIZEN_ERROR_NONE                 0
#define TIZEN_ERROR_INVALID_PARAMETER    -1
#define TIZEN_ERROR_OUT_OF_MEMORY        -2
#define TIZEN_ERROR_IO_ERROR             -3
#define TIZEN_ERROR_PERMISSION_DENIED    -4
#define TIZEN_ERROR_NOT_SUPPORTED        -5

#ifdef __cplusplus
extern "C" {
#endif

// Hàm chuyển đổi mã lỗi thành chuỗi thông báo
const char* tizen_error_to_string(tizen_error_t error);

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_ERROR_H__
""",
    "core/libtizen-core/include/tizen/log.h": """\
#ifndef __TIZEN_LOG_H__
#define __TIZEN_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

// Mức độ log (Severity levels)
typedef enum {
    TIZEN_LOG_DEBUG = 0,
    TIZEN_LOG_INFO,
    TIZEN_LOG_WARNING,
    TIZEN_LOG_ERROR,
    TIZEN_LOG_FATAL
} tizen_log_level_t;

// Hàm ghi log chính, hỗ trợ syslog/sd_journal và stdout
void tizen_log_print(tizen_log_level_t level, const char* tag, const char* format, ...);

// Các macro tiện ích cho việc ghi log
#define TIZEN_LOGD(tag, format, ...) tizen_log_print(TIZEN_LOG_DEBUG, tag, format, ##__VA_ARGS__)
#define TIZEN_LOGI(tag, format, ...) tizen_log_print(TIZEN_LOG_INFO, tag, format, ##__VA_ARGS__)
#define TIZEN_LOGW(tag, format, ...) tizen_log_print(TIZEN_LOG_WARNING, tag, format, ##__VA_ARGS__)
#define TIZEN_LOGE(tag, format, ...) tizen_log_print(TIZEN_LOG_ERROR, tag, format, ##__VA_ARGS__)
#define TIZEN_LOGF(tag, format, ...) tizen_log_print(TIZEN_LOG_FATAL, tag, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_LOG_H__
""",
    "core/libtizen-core/src/core.c": """\
#include <tizen/tizen.h>
#include <stdio.h>

#define TAG "TIZEN_CORE"

// Hàm khởi tạo core subsystem
int tizen_core_init(void) {
    TIZEN_LOGI(TAG, "Đang khởi tạo Tizen Core Library...");
    // Thực hiện các thao tác khởi tạo tại đây
    return TIZEN_ERROR_NONE;
}

// Hàm dọn dẹp core subsystem
void tizen_core_deinit(void) {
    TIZEN_LOGI(TAG, "Đang dọn dẹp Tizen Core Library...");
    // Thực hiện các thao tác dọn dẹp tại đây
}
""",
    "core/libtizen-core/src/log.c": """\
#include <tizen/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <systemd/sd-journal.h>

// Hàm ghi log tích hợp sd_journal
void tizen_log_print(tizen_log_level_t level, const char* tag, const char* format, ...) {
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    int priority = LOG_INFO;
    const char* level_str = "INFO";
    switch (level) {
        case TIZEN_LOG_DEBUG:   priority = LOG_DEBUG;   level_str = "DEBUG"; break;
        case TIZEN_LOG_INFO:    priority = LOG_INFO;    level_str = "INFO"; break;
        case TIZEN_LOG_WARNING: priority = LOG_WARNING; level_str = "WARN"; break;
        case TIZEN_LOG_ERROR:   priority = LOG_ERR;     level_str = "ERROR"; break;
        case TIZEN_LOG_FATAL:   priority = LOG_CRIT;    level_str = "FATAL"; break;
    }

    // Ghi ra sd_journal (systemd)
    sd_journal_print(priority, "[%s] %s", tag, message);

    // Ghi ra stdout với severity tags
    printf("[%s] [%s] %s\\n", level_str, tag, message);
}
""",
    "core/libtizen-core/src/utils.c": """\
#include <tizen/error.h>
#include <glib.h>
#include <string.h>

// Hàm chuyển mã lỗi thành thông báo (Tiếng Anh/Việt tuỳ chọn)
const char* tizen_error_to_string(tizen_error_t error) {
    switch (error) {
        case TIZEN_ERROR_NONE: return "Không có lỗi (Success)";
        case TIZEN_ERROR_INVALID_PARAMETER: return "Tham số không hợp lệ (Invalid Parameter)";
        case TIZEN_ERROR_OUT_OF_MEMORY: return "Hết bộ nhớ (Out of Memory)";
        case TIZEN_ERROR_IO_ERROR: return "Lỗi vào/ra (IO Error)";
        case TIZEN_ERROR_PERMISSION_DENIED: return "Từ chối quyền truy cập (Permission Denied)";
        case TIZEN_ERROR_NOT_SUPPORTED: return "Không hỗ trợ (Not Supported)";
        default: return "Lỗi không xác định (Unknown Error)";
    }
}
""",
    "core/libtizen-core/debian/control": """\
Source: libtizen-core
Priority: optional
Maintainer: TizenOS Team <team@tizenos.org>
Build-Depends: debhelper (>= 12), cmake, pkg-config, libglib2.0-dev, libsystemd-dev
Standards-Version: 4.5.0

Package: libtizen-core1
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}
Description: TizenOS Core Library
 Thư viện cốt lõi cung cấp các chức năng cơ bản, xử lý lỗi, logging
 cho hệ điều hành TizenOS.

Package: libtizen-core-dev
Architecture: any
Depends: libtizen-core1 (= ${binary:Version}), ${misc:Depends}, libglib2.0-dev, libsystemd-dev
Description: Development files for libtizen-core
 Cung cấp header files và file cấu hình pkg-config cho libtizen-core.
""",
    "core/libtizen-core/debian/rules": """\
#!/usr/bin/make -f
%:
	dh $@ --buildsystem=cmake
""",
    "core/libtizen-core/debian/changelog": """\
libtizen-core (1.0.0-1) unstable; urgency=medium

  * Bản phát hành đầu tiên cho hệ thống TizenOS.
  * Cung cấp logging qua systemd-journal và tiện ích cơ bản.

 -- TizenOS Team <team@tizenos.org>  Sun, 09 Aug 2026 01:49:00 +0700
""",
    "core/libtizen-core/debian/copyright": """\
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: libtizen-core
Source: local

Files: *
Copyright: 2026 TizenOS Team
License: GPL-3.0+
""",
    "core/libtizen-core/debian/compat": """12\n""",
    "core/libtizen-core/debian/source/format": """3.0 (quilt)\n""",
    "core/libtizen-core/debian/install": """\
usr/lib/*/libtizen-core.so.*
""",
    "core/libtizen-core/debian/libtizen-core-dev.install": """\
usr/include/tizen/*
usr/lib/*/libtizen-core.so
usr/lib/*/pkgconfig/tizen-core.pc
""",
    "core/libtizen-core/debian/postinst": """\
#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    ldconfig
fi
exit 0
""",
    
    # libtizen-dbus
    "core/libtizen-dbus/CMakeLists.txt": """\
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
""",
    "core/libtizen-dbus/tizen-dbus.pc.in": """\
prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: tizen-dbus
Description: Tizen D-Bus Integration Library
Version: @PROJECT_VERSION@
Requires: glib-2.0 gio-2.0 tizen-core
Libs: -L${libdir} -ltizen-dbus
Cflags: -I${includedir}
""",
    "core/libtizen-dbus/include/tizen/tizen-dbus.h": """\
#ifndef __TIZEN_DBUS_H__
#define __TIZEN_DBUS_H__

#include <gio/gio.h>
#include <tizen/tizen.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hàm lấy kết nối D-Bus System
GDBusConnection* tizen_dbus_get_system_connection(tizen_error_t* error);

// Hàm lấy kết nối D-Bus Session
GDBusConnection* tizen_dbus_get_session_connection(tizen_error_t* error);

// Gửi tín hiệu (emit signal) qua D-Bus
tizen_error_t tizen_dbus_emit_signal(GDBusConnection* conn, const char* object_path, const char* interface_name, const char* signal_name, GVariant* parameters);

// Đăng ký phương thức D-Bus đơn giản
tizen_error_t tizen_dbus_register_method(GDBusConnection* conn, const char* object_path, GDBusInterfaceInfo* interface_info, const GDBusInterfaceVTable* vtable);

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_DBUS_H__
""",
    "core/libtizen-dbus/src/dbus-conn.c": """\
#include <tizen/tizen-dbus.h>

#define TAG "TIZEN_DBUS_CONN"

// Lấy System Bus kết nối (Sử dụng GIO GDBus)
GDBusConnection* tizen_dbus_get_system_connection(tizen_error_t* error) {
    GError* gerror = NULL;
    GDBusConnection* conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &gerror);
    
    if (gerror != NULL) {
        TIZEN_LOGE(TAG, "Lỗi kết nối System Bus: %s", gerror->message);
        if (error) *error = TIZEN_ERROR_IO_ERROR;
        g_error_free(gerror);
        return NULL;
    }
    
    TIZEN_LOGI(TAG, "Đã kết nối thành công System Bus");
    if (error) *error = TIZEN_ERROR_NONE;
    return conn;
}

// Lấy Session Bus kết nối
GDBusConnection* tizen_dbus_get_session_connection(tizen_error_t* error) {
    GError* gerror = NULL;
    GDBusConnection* conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &gerror);
    
    if (gerror != NULL) {
        TIZEN_LOGE(TAG, "Lỗi kết nối Session Bus: %s", gerror->message);
        if (error) *error = TIZEN_ERROR_IO_ERROR;
        g_error_free(gerror);
        return NULL;
    }
    
    TIZEN_LOGI(TAG, "Đã kết nối thành công Session Bus");
    if (error) *error = TIZEN_ERROR_NONE;
    return conn;
}
""",
    "core/libtizen-dbus/src/dbus-signal.c": """\
#include <tizen/tizen-dbus.h>

#define TAG "TIZEN_DBUS_SIGNAL"

// Hàm phát (emit) D-Bus signal
tizen_error_t tizen_dbus_emit_signal(GDBusConnection* conn, const char* object_path, const char* interface_name, const char* signal_name, GVariant* parameters) {
    if (!conn || !object_path || !interface_name || !signal_name) {
        return TIZEN_ERROR_INVALID_PARAMETER;
    }
    
    GError* gerror = NULL;
    gboolean success = g_dbus_connection_emit_signal(
        conn,
        NULL, // destination_bus_name
        object_path,
        interface_name,
        signal_name,
        parameters,
        &gerror
    );
    
    if (!success) {
        TIZEN_LOGE(TAG, "Lỗi phát tín hiệu: %s", gerror->message);
        g_error_free(gerror);
        return TIZEN_ERROR_IO_ERROR;
    }
    
    TIZEN_LOGI(TAG, "Tín hiệu '%s' đã được phát đi trên interface '%s'", signal_name, interface_name);
    return TIZEN_ERROR_NONE;
}
""",
    "core/libtizen-dbus/src/dbus-method.c": """\
#include <tizen/tizen-dbus.h>

#define TAG "TIZEN_DBUS_METHOD"

// Hàm đăng ký interface và vtable cho một phương thức D-Bus
tizen_error_t tizen_dbus_register_method(GDBusConnection* conn, const char* object_path, GDBusInterfaceInfo* interface_info, const GDBusInterfaceVTable* vtable) {
    if (!conn || !object_path || !interface_info || !vtable) {
        return TIZEN_ERROR_INVALID_PARAMETER;
    }
    
    GError* gerror = NULL;
    guint registration_id = g_dbus_connection_register_object(
        conn,
        object_path,
        interface_info,
        vtable,
        NULL, // user_data
        NULL, // user_data_free_func
        &gerror
    );
    
    if (registration_id == 0) {
        TIZEN_LOGE(TAG, "Lỗi đăng ký đối tượng D-Bus tại '%s': %s", object_path, gerror->message);
        g_error_free(gerror);
        return TIZEN_ERROR_IO_ERROR;
    }
    
    TIZEN_LOGI(TAG, "Đã đăng ký phương thức tại object path '%s', ID đăng ký: %u", object_path, registration_id);
    return TIZEN_ERROR_NONE;
}
""",
    "core/libtizen-dbus/debian/control": """\
Source: libtizen-dbus
Priority: optional
Maintainer: TizenOS Team <team@tizenos.org>
Build-Depends: debhelper (>= 12), cmake, pkg-config, libglib2.0-dev, libtizen-core-dev
Standards-Version: 4.5.0

Package: libtizen-dbus1
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}
Description: TizenOS D-Bus Integration Library
 Thư viện này cung cấp các tiện ích xử lý D-Bus (GDBus)
 cho các dịch vụ cốt lõi của TizenOS.

Package: libtizen-dbus-dev
Architecture: any
Depends: libtizen-dbus1 (= ${binary:Version}), ${misc:Depends}, libglib2.0-dev, libtizen-core-dev
Description: Development files for libtizen-dbus
 Cung cấp header files và pkg-config cho libtizen-dbus.
""",
    "core/libtizen-dbus/debian/rules": """\
#!/usr/bin/make -f
%:
	dh $@ --buildsystem=cmake
""",
    "core/libtizen-dbus/debian/changelog": """\
libtizen-dbus (1.0.0-1) unstable; urgency=medium

  * Phiên bản ban đầu với hỗ trợ GDBus tích hợp core system.
  * Hỗ trợ phát signal và đăng ký method.

 -- TizenOS Team <team@tizenos.org>  Sun, 09 Aug 2026 01:49:00 +0700
""",
    "core/libtizen-dbus/debian/copyright": """\
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: libtizen-dbus
Source: local

Files: *
Copyright: 2026 TizenOS Team
License: GPL-3.0+
""",
    "core/libtizen-dbus/debian/compat": """12\n""",
    "core/libtizen-dbus/debian/source/format": """3.0 (quilt)\n""",
    "core/libtizen-dbus/debian/install": """\
usr/lib/*/libtizen-dbus.so.*
""",
    "core/libtizen-dbus/debian/libtizen-dbus-dev.install": """\
usr/include/tizen/*
usr/lib/*/libtizen-dbus.so
usr/lib/*/pkgconfig/tizen-dbus.pc
""",
    "core/libtizen-dbus/debian/postinst": """\
#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    ldconfig
fi
exit 0
"""
}

for path, content in files.items():
    full_path = os.path.join(base_dir, path)
    os.makedirs(os.path.dirname(full_path), exist_ok=True)
    with open(full_path, "w", encoding="utf-8") as f:
        f.write(content)
        
print("Successfully generated all files.")
