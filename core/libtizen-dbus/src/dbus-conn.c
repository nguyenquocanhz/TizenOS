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
