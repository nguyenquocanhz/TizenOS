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
