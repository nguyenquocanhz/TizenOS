#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <stdio.h>

void on_discovered(GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, gpointer user_data) {
    if (err) {
        g_printerr("Lỗi khi quét file: %s\n", err->message);
        return;
    }
    
    // Lấy thời lượng (duration) của media
    GstClockTime duration = gst_discoverer_info_get_duration(info);
    const gchar *uri = gst_discoverer_info_get_uri(info);
    
    g_print("Đã phát hiện file: %s, Thời lượng: %" GST_TIME_FORMAT "\n", uri, GST_TIME_ARGS(duration));
    // Lưu thông tin vào database ở đây (gọi media_db_insert)
}

void media_scanner_start(void) {
    gst_init(NULL, NULL);
    
    GError *err = NULL;
    // Khởi tạo GStreamer discoverer với timeout là 5 giây
    GstDiscoverer *discoverer = gst_discoverer_new(5 * GST_SECOND, &err);
    if (!discoverer) {
        g_printerr("Lỗi tạo discoverer: %s\n", err->message);
        g_clear_error(&err);
        return;
    }
    
    g_signal_connect(discoverer, "discovered", G_CALLBACK(on_discovered), NULL);
    
    // Quét một file mẫu (trong thực tế sẽ duyệt thư mục cấu hình)
    gst_discoverer_discover_uri_async(discoverer, "file:///usr/share/media/sample.mp4");
    
    gst_discoverer_start(discoverer);
    g_print("Đã khởi động quá trình quét media bằng GStreamer.\n");
}
