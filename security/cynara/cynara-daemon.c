/*
 * TizenOS Cynara Policy Daemon - Implementation
 * =============================================================================
 * Lắng nghe trên D-Bus system bus (org.tizen.cynara) để phục vụ các yêu cầu
 * kiểm tra quyền (Check Privilege) của ứng dụng từ SQLite Policy DB.
 * Sử dụng GMainLoop & GDBusConnection nâng cao hiệu năng và độ tin cậy.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <sqlite3.h>

#define CYNARA_DB_PATH "/var/lib/cynara/cynara.db"
#define CYNARA_BUS_NAME "org.tizen.cynara"
#define CYNARA_OBJECT_PATH "/org/tizen/cynara"

static GMainLoop *main_loop = NULL;
static sqlite3 *db = NULL;

/* Khởi tạo SQLite Database cho Cynara Policies */
static bool init_database(void) {
    int rc = sqlite3_open(CYNARA_DB_PATH, &db);
    if (rc != SQLITE_OK) {
        // Fallback sang memory database nếu chưa tạo đường dẫn /var/lib/cynara
        g_warning("[CYNARA-DAEMON] Không thể mở %s, fallback dùng in-memory DB: %s",
                  CYNARA_DB_PATH, sqlite3_errmsg(db));
        rc = sqlite3_open(":memory:", &db);
        if (rc != SQLITE_OK) return false;
    }

    const char *sql = 
        "CREATE TABLE IF NOT EXISTS policies ("
        "client_label TEXT NOT NULL, "
        "user_id TEXT NOT NULL, "
        "privilege TEXT NOT NULL, "
        "result INTEGER NOT NULL, "
        "PRIMARY KEY (client_label, user_id, privilege));"
        "INSERT OR IGNORE INTO policies VALUES ('User', '*', 'http://tizen.org/privilege/internet', 1);"
        "INSERT OR IGNORE INTO policies VALUES ('System', '*', '*', 1);";

    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        g_printerr("[CYNARA-DAEMON] Lỗi khởi tạo bảng SQLite: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    g_print("[CYNARA-DAEMON] ✓ Khởi tạo SQLite Policy DB thành công.\n");
    return true;
}

/* Xử lý D-Bus Method Call kiểm tra quyền (CheckPrivilege) */
static void handle_method_call(GDBusConnection *conn,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data) {
    (void)conn; (void)sender; (void)object_path; (void)interface_name; (void)user_data;

    if (g_strcmp0(method_name, "CheckPrivilege") == 0) {
        const gchar *client_label, *user_id, *privilege;
        g_variant_get(parameters, "(&s&s&s)", &client_label, &user_id, &privilege);

        g_print("[CYNARA-DAEMON] Checking: Label=%s, User=%s, Privilege=%s\n",
                client_label, user_id, privilege);

        int result = 0; // Mặc định Deny (0)
        
        // System label luôn có full quyền (1)
        if (g_strcmp0(client_label, "System") == 0 || g_strcmp0(client_label, "root") == 0) {
            result = 1;
        } else {
            // Truy vấn SQLite Policy DB
            sqlite3_stmt *stmt;
            const char *query = "SELECT result FROM policies WHERE client_label = ? AND privilege = ?;";
            if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, client_label, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, privilege, -1, SQLITE_STATIC);

                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    result = sqlite3_column_int(stmt, 0);
                } else {
                    // Mặc định cho phép internet nếu không khai báo cụ thể
                    if (strstr(privilege, "internet")) result = 1;
                }
                sqlite3_finalize(stmt);
            }
        }

        g_print("[CYNARA-DAEMON] Result -> %s (%d)\n", result == 1 ? "ALLOW" : "DENY", result);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", result));
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call, NULL, NULL, {0}
};

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.tizen.cynara'>"
  "    <method name='CheckPrivilege'>"
  "      <arg type='s' name='client_label' direction='in'/>"
  "      <arg type='s' name='user_id' direction='in'/>"
  "      <arg type='s' name='privilege' direction='in'/>"
  "      <arg type='i' name='result' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    (void)name; (void)user_data;
    GError *error = NULL;

    GDBusNodeInfo *node_info = g_dbus_node_info_new_for_xml(introspection_xml, &error);
    if (!node_info) {
        g_printerr("[CYNARA-DAEMON] Introspection XML error: %s\n", error->message);
        g_error_free(error);
        return;
    }

    g_dbus_connection_register_object(connection,
                                       CYNARA_OBJECT_PATH,
                                       node_info->interfaces[0],
                                       &interface_vtable,
                                       NULL, NULL, &error);
    g_dbus_node_info_unref(node_info);

    if (error) {
        g_printerr("[CYNARA-DAEMON] Failed to register object: %s\n", error->message);
        g_error_free(error);
    } else {
        g_print("[CYNARA-DAEMON] ✓ Registered D-Bus interface %s at %s\n", CYNARA_BUS_NAME, CYNARA_OBJECT_PATH);
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    g_print("[CYNARA-DAEMON] Starting TizenOS Cynara Policy Daemon...\n");

    if (!init_database()) {
        g_printerr("[CYNARA-DAEMON] Failed to initialize SQLite database.\n");
        return EXIT_FAILURE;
    }

    main_loop = g_main_loop_new(NULL, FALSE);

    g_bus_own_name(G_BUS_TYPE_SYSTEM,
                   CYNARA_BUS_NAME,
                   G_BUS_NAME_OWNER_FLAGS_NONE,
                   on_bus_acquired,
                   NULL, NULL, NULL, NULL);

    g_print("[CYNARA-DAEMON] Daemon running on GMainLoop...\n");
    g_main_loop_run(main_loop);

    if (db) sqlite3_close(db);
    g_main_loop_unref(main_loop);
    return EXIT_SUCCESS;
}
