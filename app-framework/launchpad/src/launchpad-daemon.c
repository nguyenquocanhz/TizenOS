/*
 * TizenOS Launchpad Pre-fork Daemon - Main Entrypoint
 * =============================================================================
 * Daemon chạy ngầm (launchpad.service) lắng nghe D-Bus system bus (org.tizen.Launchpad)
 * để đón nhận tín hiệu mở App và gán Worker Process từ Pre-fork Pool.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>

extern void launchpad_init_pool(void);
extern bool launchpad_launch_app(const char *app_so_path);

#define LAUNCHPAD_BUS_NAME "org.tizen.Launchpad"
#define LAUNCHPAD_OBJECT_PATH "/org/tizen/Launchpad"

static GMainLoop *main_loop = NULL;

static void handle_method_call(GDBusConnection *conn,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data) {
    (void)conn; (void)sender; (void)object_path; (void)interface_name; (void)user_data;

    if (g_strcmp0(method_name, "LaunchApp") == 0) {
        const gchar *app_id, *app_so_path;
        g_variant_get(parameters, "(&s&s)", &app_id, &app_so_path);

        g_print("[LAUNCHPAD-DAEMON] Received LaunchApp request: AppID='%s', Path='%s'\n", app_id, app_so_path);

        bool success = launchpad_launch_app(app_so_path);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call, NULL, NULL, {0}
};

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.tizen.Launchpad'>"
  "    <method name='LaunchApp'>"
  "      <arg type='s' name='app_id' direction='in'/>"
  "      <arg type='s' name='app_path' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    (void)name; (void)user_data;
    GError *error = NULL;

    GDBusNodeInfo *node_info = g_dbus_node_info_new_for_xml(introspection_xml, &error);
    if (node_info) {
        g_dbus_connection_register_object(connection,
                                           LAUNCHPAD_OBJECT_PATH,
                                           node_info->interfaces[0],
                                           &interface_vtable,
                                           NULL, NULL, &error);
        g_dbus_node_info_unref(node_info);
    }

    if (error) {
        g_printerr("[LAUNCHPAD-DAEMON-ERROR] Failed to register D-Bus object: %s\n", error->message);
        g_error_free(error);
    } else {
        g_print("[LAUNCHPAD-DAEMON] ✓ Registered D-Bus interface %s at %s\n", LAUNCHPAD_BUS_NAME, LAUNCHPAD_OBJECT_PATH);
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    g_print("==============================================================\n");
    g_print(" TizenOS Launchpad Pre-fork Daemon (org.tizen.Launchpad)\n");
    g_print("==============================================================\n");

    // 1. Khởi tạo Pre-fork Process Pool
    launchpad_init_pool();

    // 2. Chạy GMainLoop và đăng ký D-Bus Service
    main_loop = g_main_loop_new(NULL, FALSE);

    g_bus_own_name(G_BUS_TYPE_SYSTEM,
                   LAUNCHPAD_BUS_NAME,
                   G_BUS_NAME_OWNER_FLAGS_NONE,
                   on_bus_acquired,
                   NULL, NULL, NULL, NULL);

    g_print("[LAUNCHPAD-DAEMON] Daemon listening for App Launch requests...\n");
    g_main_loop_run(main_loop);

    g_main_loop_unref(main_loop);
    return EXIT_SUCCESS;
}
