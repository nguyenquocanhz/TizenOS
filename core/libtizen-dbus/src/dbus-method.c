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
