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
