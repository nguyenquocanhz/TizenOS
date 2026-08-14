/*
 * Tizen App Store - Async Catalog Engine & Installer (GTK4)
 * =============================================================================
 * Kho ứng dụng phong phú cho TizenOS (60+ Ứng dụng phổ biến):
 * - Cài đặt Asynchronous không lag/giật UI chính (GThread + GMainContext)
 * - Thanh tiến trình cài đặt mượt mà (GtkProgressBar Pulse Animation)
 * - Tự động nhận diện trạng thái cài đặt & chuyển đổi nút "Mở App" tức thì.
 * =============================================================================
 */

#include "app-store.h"
#include "tizen/pkg.h"
#include "tizen/theme.h"         /* tizen_button_new — nút dùng icon theme */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TAG "APP_STORE_CATALOG"

/* -----------------------------------------------------------------------------
 * Ngữ cảnh cài đặt bất đồng bộ
 * -----------------------------------------------------------------------------
 * VÒNG ĐỜI WIDGET — vì sao phải giữ tham chiếu MẠNH
 * Hai widget dưới đây là con của thẻ trong catalog. refresh_app_store_catalog()
 * gỡ sạch con của flowbox mỗi lần người dùng gõ tìm kiếm, đổi danh mục hoặc bấm
 * "Cập nhật kho". Nếu apt còn đang chạy lúc đó, thẻ bị huỷ trong khi:
 *
 *   - timer 80ms vẫn gọi gtk_progress_bar_pulse() trên progress_bar
 *   - callback hoàn tất vẫn gọi set_sensitive()/set_label() trên btn_install
 *
 * Cả hai đều đọc vào bộ nhớ đã giải phóng — use-after-free, và rất dễ gặp vì
 * người dùng hay gõ tìm kiếm tiếp trong lúc chờ cài.
 *
 * Giữ tham chiếu mạnh (g_object_ref) khiến widget vẫn SỐNG sau khi bị gỡ khỏi
 * cây: thao tác lên nó chỉ đơn giản là vô hình, không còn nguy hiểm. Tham chiếu
 * được nhả trong install_task_free().
 * -------------------------------------------------------------------------- */
typedef struct {
    AppStoreItem *item;
    GtkWidget *btn_install;    /* giữ ref mạnh */
    GtkWidget *progress_bar;   /* giữ ref mạnh */
    guint pulse_timer_id;
    bool success;
} InstallTaskData;

static void install_task_free(InstallTaskData *task) {
    if (!task) return;
    if (task->pulse_timer_id > 0) {
        g_source_remove(task->pulse_timer_id);
        task->pulse_timer_id = 0;
    }
    g_clear_object(&task->btn_install);
    g_clear_object(&task->progress_bar);
    g_free(task);
}

static AppStoreItem store_catalog[] = {
    // 🌐 Internet & Communication
    {"firefox", "Firefox ESR", "internet", "Mozilla", "firefox-esr", "Trình duyệt Web bảo mật, tốc độ cao chuẩn Debian 12.", "firefox-esr", "85 MB", "4.9 ★", false},
    {"chrome", "Google Chrome", "internet", "Google", "google-chrome-stable", "Trình duyệt hàng đầu thế giới với kho extension phong phú.", "google-chrome-stable", "95 MB", "4.8 ★", false},
    {"brave", "Brave Browser", "internet", "Brave Software", "brave-browser", "Trình duyệt chặn quảng cáo & bảo vệ quyền riêng tư mặc định.", "brave-browser", "90 MB", "4.8 ★", false},
    {"chromium", "Chromium", "internet", "Chromium Project", "chromium", "Trình duyệt mã nguồn mở nền tảng cho Google Chrome.", "chromium", "88 MB", "4.7 ★", false},
    {"telegram", "Telegram Desktop", "internet", "Telegram FZ-LLC", "telegram-desktop", "Ứng dụng nhắn tin bảo mật, mã hóa đầu cuối cực nhanh.", "telegram-desktop", "45 MB", "4.9 ★", false},
    {"discord", "Discord", "internet", "Discord Inc.", "discord", "Nền tảng trò chuyện thoại & video trực tuyến cho game thủ và team.", "discord", "80 MB", "4.7 ★", false},
    {"thunderbird", "Thunderbird Mail", "internet", "Mozilla", "thunderbird", "Trình quản lý Email, Lịch làm việc và Danh bạ chuyên nghiệp.", "thunderbird", "70 MB", "4.8 ★", false},
    {"filezilla", "FileZilla FTP", "internet", "FileZilla Team", "filezilla", "Trình quản lý truyền tải tệp tin FTP / SFTP phổ biến nhất.", "filezilla", "20 MB", "4.7 ★", false},
    {"transmission", "Transmission Torrent", "internet", "Transmission", "transmission-gtk", "Trình tải Torrent cực nhẹ, giao diện GTK đơn giản.", "transmission-gtk", "12 MB", "4.8 ★", false},
    {"wireshark", "Wireshark Network", "internet", "Wireshark Foundation", "wireshark", "Công cụ phân tích gói tin mạng chuyên sâu cho quản trị viên.", "wireshark", "40 MB", "4.9 ★", false},

    // 🎨 Graphics & Multimedia
    {"vlc", "VLC Media Player", "graphics", "VideoLAN", "vlc", "Trình phát đa phương tiện đọc mọi định dạng Audio/Video MP4, MKV, FLAC.", "vlc", "60 MB", "4.9 ★", false},
    {"gimp", "GIMP Image Editor", "graphics", "GIMP Team", "gimp", "Công cụ chỉnh sửa ảnh chuyên nghiệp thay thế Photoshop miễn phí.", "gimp", "120 MB", "4.8 ★", false},
    {"obs", "OBS Studio", "graphics", "OBS Project", "com.obsproject.Studio", "Phần mềm quay màn hình & Livestream trực tuyến chuyên nghiệp.", "obs-studio", "110 MB", "4.9 ★", false},
    {"inkscape", "Inkscape Vector", "graphics", "Inkscape Team", "org.inkscape.Inkscape", "Trình thiết kế đồ họa vector chuyên nghiệp thay thế Illustrator.", "inkscape", "90 MB", "4.7 ★", false},
    {"blender", "Blender 3D", "graphics", "Blender Foundation", "blender", "Bộ phần mềm dựng hình & hoạt họa 3D đẳng cấp thế giới.", "blender", "250 MB", "5.0 ★", false},
    {"kdenlive", "Kdenlive Video Editor", "graphics", "KDE", "kdenlive", "Trình dựng & biên tập video chuyên nghiệp đa tầng timeline.", "kdenlive", "130 MB", "4.8 ★", false},
    {"audacity", "Audacity Audio Editor", "graphics", "Audacity Team", "audacity", "Phần mềm thu âm & xử lý âm thanh kỹ thuật số đa năng.", "audacity", "35 MB", "4.8 ★", false},
    {"shotcut", "Shotcut Video", "graphics", "Meltytech", "org.shotcut.Shotcut", "Trình biên tập video 4K mã nguồn mở miễn phí.", "shotcut", "105 MB", "4.7 ★", false},
    {"darktable", "Darktable RAW", "graphics", "Darktable Team", "org.darktable.darktable", "Công cụ xử lý ảnh RAW chuyên nghiệp cho nhiếp ảnh gia.", "darktable", "80 MB", "4.8 ★", false},
    {"mpv", "mpv Media Player", "graphics", "mpv Team", "mpv", "Trình xem video siêu nhẹ, hỗ trợ GPU HW Acceleration.", "mpv", "15 MB", "4.9 ★", false},
    {"handbrake", "HandBrake Transcoder", "graphics", "HandBrake Team", "fr.handbrake.ghb", "Công cụ chuyển đổi định dạng video MP4/MKV đa luồng.", "handbrake", "50 MB", "4.8 ★", false},
    {"krita", "Krita Digital Painting", "graphics", "Krita Foundation", "org.kde.krita", "Ứng dụng vẽ phác thảo & minh họa kỹ thuật số hàng đầu.", "krita", "160 MB", "4.9 ★", false},

    // 💼 Office & Productivity
    {"libreoffice", "LibreOffice Suite", "office", "Document Foundation", "libreoffice-startcenter", "Bộ ứng dụng văn phòng đầy đủ (Word, Excel, PowerPoint alternative).", "libreoffice", "310 MB", "4.8 ★", false},
    {"evince", "Evince PDF Reader", "office", "GNOME", "org.gnome.Evince", "Trình đọc tài liệu PDF nhẹ và đọc nhanh.", "evince", "25 MB", "4.6 ★", false},
    {"mousepad", "Mousepad Text Editor", "office", "XFCE", "org.xfce.mousepad", "Trình soạn thảo văn bản đơn giản, giao diện sạch sẽ.", "mousepad", "10 MB", "4.5 ★", false},
    {"onlyoffice", "ONLYOFFICE Desktop", "office", "Ascensio System", "onlyoffice-desktopeditors", "Bộ ứng dụng văn phòng tương thích cao nhất với Microsoft Office.", "onlyoffice-desktopeditors", "280 MB", "4.9 ★", false},
    {"okular", "Okular Reader", "office", "KDE", "org.kde.okular", "Trình xem tài liệu đa định dạng (PDF, EPub, DjVu, CHM).", "okular", "65 MB", "4.7 ★", false},
    {"obsidian", "Obsidian Notes", "office", "Dynalist Inc.", "obsidian", "Trình quản lý ghi chú tri thức & liên kết tư duy Markdown.", "obsidian", "85 MB", "5.0 ★", false},
    {"xournalpp", "Xournal++ Annotator", "office", "Xournal++ Team", "com.github.xournalpp.xournalpp", "Ứng dụng ghi chú viết tay & chú thích PDF hỗ trợ bút cảm ứng.", "xournalpp", "30 MB", "4.8 ★", false},

    // 💻 Developer Tools & Utilities
    {"vscode", "Visual Studio Code", "dev", "Microsoft", "code", "Trình soạn thảo mã nguồn phổ biến nhất thế giới cho lập trình viên.", "code", "100 MB", "4.9 ★", false},
    {"sublime", "Sublime Text", "dev", "Sublime HQ", "sublime-text", "Trình chỉnh sửa code siêu nhẹ, mở file lớn tức thì.", "sublime-text", "30 MB", "4.8 ★", false},
    {"git", "Git Version Control", "dev", "Git Team", "git", "Hệ thống quản lý phiên bản mã nguồn phân tán.", "git", "15 MB", "5.0 ★", false},
    {"docker", "Docker Engine", "dev", "Docker Inc.", "docker", "Nền tảng containerization cho các ứng dụng đám mây & DevOps.", "docker-ce", "150 MB", "4.9 ★", false},
    {"python", "Python 3 Development", "dev", "Python Foundation", "python3", "Ngôn ngữ lập trình phổ biến cho AI, Data Science và Web.", "python3-full", "50 MB", "5.0 ★", false},
    {"nodejs", "Node.js JavaScript", "dev", "OpenJS Foundation", "nodejs", "Môi trường thực thi JavaScript server-side hiệu năng cao.", "nodejs", "40 MB", "4.9 ★", false},
    {"golang", "Go Programming", "dev", "Google", "golang", "Ngôn ngữ lập trình hệ thống & microservices tốc độ cao.", "golang-go", "110 MB", "4.9 ★", false},
    {"gcc", "GCC & G++ Compiler", "dev", "GNU Project", "gcc", "Bộ biên dịch C/C++ tiêu chuẩn cho Linux Desktop.", "build-essential", "80 MB", "5.0 ★", false},
    {"neovim", "Neovim Editor", "dev", "Neovim Team", "nvim", "Vim-fork thế hệ mới hỗ trợ Lua plugin & LSP.", "neovim", "20 MB", "4.9 ★", false},
    {"qtcreator", "Qt Creator IDE", "dev", "Qt Company", "org.qt-project.qtcreator", "IDE phát triển ứng dụng C++ GUI GTK/Qt chuyên nghiệp.", "qtcreator", "180 MB", "4.8 ★", false},

    // 🎮 Games & Entertainment
    {"steam", "Steam Gaming Platform", "games", "Valve", "steam", "Kho game PC số 1 thế giới với hàng ngàn tựa game Proton Linux.", "steam", "75 MB", "4.9 ★", false},
    {"supertuxkart", "SuperTuxKart", "games", "STK Team", "supertuxkart", "Trò chơi đua xe 3D vui nhộn dành cho gia đình.", "supertuxkart", "600 MB", "4.7 ★", false},
    {"lutris", "Lutris Game Manager", "games", "Lutris Team", "net.lutris.Lutris", "Trình quản lý & chạy game Windows/Retro trên Linux.", "lutris", "40 MB", "4.8 ★", false},
    {"heroic", "Heroic Games Launcher", "games", "Heroic Games", "com.heroicgameslauncher.hgl", "Trình chạy game Epic Games & GOG Native mã nguồn mở.", "heroic", "95 MB", "4.8 ★", false},
    {"retroarch", "RetroArch Emulator", "games", "Libretro", "org.libretro.RetroArch", "Hệ thống giả lập game cổ điển (NES, PS1, GBA, Arcade).", "retroarch", "120 MB", "4.9 ★", false},
    {"minetest", "Minetest 3D", "games", "Minetest Team", "net.minetest.minetest", "Trò chơi thế giới mở tạo hình khối 3D mã nguồn mở.", "minetest", "35 MB", "4.6 ★", false},
    {"supertux", "SuperTux 2D", "games", "SuperTux Team", "supertux2", "Game đi màn 2D phiêu lưu kinh điển chú chim cánh cụt Tux.", "supertux2", "80 MB", "4.7 ★", false},

    // 🛡️ System & Security
    {"htop", "Htop System Monitor", "system", "Htop Team", "htop", "Công cụ theo dõi CPU, RAM, tiến trình trong Terminal.", "htop", "5 MB", "4.9 ★", false},
    {"btop", "Btop++ Resource Monitor", "system", "aristocratos", "btop", "Giao diện giám sát tài nguyên phần cứng cực đẹp.", "btop", "8 MB", "5.0 ★", false},
    {"gparted", "GParted Partition Manager", "system", "GParted Team", "gparted", "Công cụ phân chia & quản lý ổ đĩa định dạng ext4, ntfs, fat32.", "gparted", "30 MB", "4.8 ★", false},
    {"neofetch", "Neofetch System Info", "system", "dylanaraps", "neofetch", "Hiển thị logo TizenOS và thông tin cấu hình máy trong Terminal.", "neofetch", "2 MB", "4.9 ★", false},
    {"synaptic", "Synaptic Package Manager", "system", "Debian Team", "synaptic", "Công cụ quản lý gói APT nâng cao GUI cho Debian 12.", "synaptic", "20 MB", "4.8 ★", false},
    {"timeshift", "Timeshift Backup", "system", "Teejee Tech", "timeshift", "Hệ thống tạo điểm khôi phục & sao lưu hệ điều hành TizenOS.", "timeshift", "25 MB", "4.9 ★", false},
    {"bleachbit", "BleachBit Cleaner", "system", "BleachBit", "bleachbit", "Dọn dẹp bộ nhớ đệm, tệp rác & bảo vệ riêng tư hệ thống.", "bleachbit", "10 MB", "4.7 ★", false},
    {"stacer", "Stacer System Optimizer", "system", "oguzhaninan", "stacer", "Bộ công cụ tối ưu hóa, dọn dẹp & quản lý Startup TizenOS.", "stacer", "35 MB", "4.8 ★", false},

    // 🚀 TizenOS Native Utilities
    {"tizen-album", "Tizen Album Photo & Video", "graphics", "TizenOS Team", "tizen-album", "Trình xem & biên tập ảnh, video GTK4 chuyên nghiệp.", "tizen-album", "14 MB", "5.0 ★", true},
    {"tizenos-notepad", "Tizen Notepad", "office", "TizenOS Team", "tizenos-notepad", "Trình soạn thảo văn bản GTK4 đa tab & tô màu cú pháp.", "tizenos-notepad", "12 MB", "5.0 ★", true},
    {"tizen-store", "Tizen Store", "system", "TizenOS Team", "tizen-store", "Trung tâm phần mềm Tizen Software Center gốc.", "tizen-store", "15 MB", "5.0 ★", true},
    {"tizen-app-manager", "Tizen App Manager", "system", "TizenOS Team", "tizen-app-manager", "Quản lý ứng dụng cài đặt .deb & .tpk TizenOS.", "tizen-app-manager", "12 MB", "5.0 ★", true},
    {"tizenos-files", "Tizen File Manager", "system", "TizenOS Team", "tizenos-files", "Trình quản lý tệp tin GTK4 kính mờ mount đĩa ISO.", "tizenos-files", "18 MB", "5.0 ★", true},
    {"tizenos-screenshot", "Tizen Screenshot Tool", "system", "TizenOS Team", "tizenos-screenshot", "Chụp màn hình toàn bộ/vùng chọn/cửa sổ GUI.", "tizenos-screenshot", "2 MB", "5.0 ★", true},
    {"tizenos-screenrecord", "Tizen Screen Recorder", "system", "TizenOS Team", "tizenos-screenrecord", "Quay video màn hình MP4 âm thanh PipeWire.", "tizenos-screenrecord", "3 MB", "5.0 ★", true},
    {"tizenos-script-runner", "Tizen Script Runner", "dev", "TizenOS Team", "utilities-terminal", "Thực thi kịch bản Bash Shell (.sh) và lệnh hệ thống Real-time.", "tizenos-script-runner", "5 MB", "5.0 ★", true}
};

static int catalog_size = sizeof(store_catalog) / sizeof(store_catalog[0]);
static GtkWidget *catalog_flowbox = NULL;

/* =============================================================================
 * SEC-015 — KIỂM TRA GÓI ĐÃ CÀI
 * =============================================================================
 * Bản trước:
 *     "dpkg-query -W -f='${Status}' '%s' | grep -q 'installed' || command -v '%s'"
 *
 * `grep -q 'installed'` KHỚP CẢ chuỗi "unknown ok not-installed" — tức là gói
 * CHƯA cài vẫn được báo là đã cài. Hậu quả: mọi thẻ đều hiện "Mở App", nút
 * "Cài Đặt" gần như không bao giờ xuất hiện. Vế `command -v` cũng sai vì tên
 * gói apt thường khác tên binary (obs-studio -> obs, python3-full -> không có
 * binary nào).
 *
 * tizen_pkg_is_installed() hỏi `${db:Status-Status}` rồi SO SÁNH BẰNG với
 * "installed", nên phân biệt đúng cả trạng thái "config-files" (đã gỡ nhưng
 * còn tệp cấu hình — với người dùng thì coi như chưa cài).
 * ============================================================================= */
static bool check_package_installed(const char *pkg_name) {
    return tizen_pkg_is_installed(pkg_name);
}

/* Trạng thái cài đặt được quét MỘT lần rồi dùng lại.
 * Bản trước gọi check_package_installed() cho từng app bên trong vòng lặp của
 * refresh_app_store_catalog(), mà refresh chạy theo signal "search-changed" —
 * tức 60+ lần fork+exec cho MỖI ký tự người dùng gõ vào ô tìm kiếm. */
static bool states_scanned = false;

static void rescan_installed_states(void) {
    for (int i = 0; i < catalog_size; i++)
        store_catalog[i].is_installed = check_package_installed(store_catalog[i].package_name);
    states_scanned = true;
}

void app_store_invalidate_states(void) {
    states_scanned = false;
}

/* Pulse animation callback cho GtkProgressBar */
static gboolean on_install_pulse_timer(gpointer user_data) {
    InstallTaskData *task = (InstallTaskData*)user_data;
    if (task && task->progress_bar) {
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(task->progress_bar));
    }
    return G_SOURCE_CONTINUE;
}

/* Main thread completion handler khi cài đặt xong */
static gboolean on_install_finished_main_thread(gpointer user_data) {
    InstallTaskData *task = (InstallTaskData*)user_data;
    if (!task) return G_SOURCE_REMOVE;

    // Dừng Pulse Timer Animation
    if (task->pulse_timer_id > 0) {
        g_source_remove(task->pulse_timer_id);
        task->pulse_timer_id = 0;
    }

    if (task->progress_bar) {
        gtk_widget_set_visible(task->progress_bar, FALSE);
    }

    /* Trạng thái vừa đổi -> lần refresh kế tiếp phải hỏi lại dpkg. */
    app_store_invalidate_states();

    if (task->btn_install) {
        gtk_widget_set_sensitive(task->btn_install, TRUE);
        if (task->success) {
            task->item->is_installed = true;
            /* tizen_button_set_icon_label, KHÔNG phải gtk_button_set_label:
             * hàm sau vứt bỏ hộp icon+chữ và thay bằng một GtkLabel trần, nên
             * nút mất sạch biểu tượng ngay lần đổi nhãn đầu tiên. */
            tizen_button_set_icon_label(task->btn_install,
                                        "media-playback-start-symbolic", "Mở App");
        } else {
            tizen_button_set_icon_label(task->btn_install,
                                        "folder-download-symbolic", "Cài Đặt");
        }
    }

    if (task->success) {
        /* Cửa sổ cha lấy từ flowbox catalog, KHÔNG từ nút.
         * Nếu catalog đã được dựng lại thì nút đang mồ côi và
         * gtk_widget_get_root() trả NULL -> GTK_WINDOW(NULL) sinh cảnh báo và
         * hộp thoại không có cha. Flowbox thì luôn còn trong cây. */
        GtkWidget *root = catalog_flowbox
            ? GTK_WIDGET(gtk_widget_get_root(catalog_flowbox)) : NULL;
        GtkWidget *dialog = gtk_message_dialog_new(
            (root && GTK_IS_WINDOW(root)) ? GTK_WINDOW(root) : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Đã cài đặt thành công '%s' vào TizenOS.", task->item->name
        );
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_window_present(GTK_WINDOW(dialog));
    }

    install_task_free(task);
    return G_SOURCE_REMOVE;
}

/* Worker thread chạy apt-get install ngầm (Không bao giờ gây lag UI) */
static gpointer install_worker_thread(gpointer user_data) {
    InstallTaskData *task = (InstallTaskData*)user_data;
    if (!task) return NULL;

    /* Chạy trong worker thread nên tizen_exec_sync() ở đây KHÔNG chặn main
     * loop — giao diện và thanh tiến trình vẫn mượt.
     *
     * Ba thay đổi so với bản cũ:
     *   1. Không dựng chuỗi shell nữa. Tên gói đi qua argv nên nội dung của nó
     *      không bao giờ được diễn giải thành cú pháp.
     *   2. Chỉ MỘT lần pkexec cho cả update lẫn install. Bản cũ gọi pkexec hai
     *      lần liên tiếp nên người dùng phải nhập mật khẩu hai lần.
     *   3. Bỏ nhánh fallback gọi gdbus tới org.tizen.Installer. Daemon đó giờ
     *      đã bắt buộc xác thực người gọi (SEC-001), và `|| true` ở cuối chuỗi
     *      cũ khiến mọi thất bại đều bị nuốt im lặng. */
    const char *argv[] = {
        "pkexec", "env", "DEBIAN_FRONTEND=noninteractive",
        "sh", "-c",
        "apt-get update -qq; exec apt-get install -y -- \"$1\"",
        "tizen-store",                  /* $0 */
        task->item->package_name,       /* $1 — tham số vị trí, không nối chuỗi */
        NULL
    };
    tizen_exec_sync(argv, NULL, NULL, NULL);

    /* Không tin mã thoát của apt: hỏi lại dpkg xem gói có thực sự vào không. */
    task->success = check_package_installed(task->item->package_name);

    // Gửi sự kiện về Main Thread
    g_idle_add(on_install_finished_main_thread, task);
    return NULL;
}

/* Callback Cài đặt 1-Click Asynchronous */
static void on_install_app_clicked(GtkButton *btn, gpointer user_data) {
    AppStoreItem *item = (AppStoreItem*)user_data;
    if (!item) return;

    GtkWidget *card = gtk_widget_get_parent(gtk_widget_get_parent(GTK_WIDGET(btn)));
    GtkWidget *pbar = NULL;

    // Tìm hoặc tạo GtkProgressBar trong Card
    GtkWidget *child = gtk_widget_get_first_child(card);
    while (child) {
        if (GTK_IS_PROGRESS_BAR(child)) {
            pbar = child;
            break;
        }
        child = gtk_widget_get_next_sibling(child);
    }

    if (!pbar) {
        pbar = gtk_progress_bar_new();
        gtk_widget_set_margin_start(pbar, 12);
        gtk_widget_set_margin_end(pbar, 12);
        gtk_widget_set_margin_bottom(pbar, 4);
        gtk_box_append(GTK_BOX(card), pbar);
    }

    gtk_widget_set_visible(pbar, TRUE);
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(pbar), 0.1);
    tizen_button_set_icon_label(GTK_WIDGET(btn), "content-loading-symbolic", "Đang cài...");
    gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);

    InstallTaskData *task = g_new0(InstallTaskData, 1);
    task->item = item;
    /* Ref MẠNH: catalog có thể bị dựng lại trong lúc apt chạy — xem ghi chú ở
     * InstallTaskData. Thiếu ref là use-after-free. */
    task->btn_install  = GTK_WIDGET(g_object_ref(btn));
    task->progress_bar = GTK_WIDGET(g_object_ref(pbar));
    task->pulse_timer_id = g_timeout_add(80, on_install_pulse_timer, task);

    // Khởi tạo Worker Thread cài đặt
    g_thread_new("app_installer_worker", install_worker_thread, task);
}

/* =============================================================================
 * SEC-016 — MỞ ỨNG DỤNG ĐÃ CÀI
 * =============================================================================
 * Bản trước:  snprintf(cmd, ..., "%s &", item->package_name);
 *             g_spawn_command_line_async(cmd, NULL);
 *
 * Ba lỗi chồng nhau:
 *   1. g_spawn_command_line_async KHÔNG chạy qua shell — nó tách chuỗi bằng
 *      g_shell_parse_argv, nên dấu "&" trở thành argv[1] thật gửi cho chương
 *      trình. Đã kiểm chứng: argc=3 ["xdg-open", "<đường dẫn>", "&"].
 *   2. package_name là tên gói APT, KHÔNG phải tên binary. obs-studio có binary
 *      tên `obs`; python3-full, docker-ce, build-essential không có binary nào
 *      cùng tên. Bấm "Mở App" với các gói này không bao giờ hoạt động.
 *   3. GError bị bỏ qua (NULL) nên khi thất bại thì im lặng hoàn toàn, người
 *      dùng không hiểu vì sao bấm mà không có gì xảy ra.
 *
 * Bản vá thử theo thứ tự: desktop id suy từ trường icon, rồi từ tên gói, cuối
 * cùng mới thử chạy trực tiếp. GIO lo phần field code %U/%F và tách tiến trình.
 * ============================================================================= */
static void on_launch_app_clicked(GtkButton *btn, gpointer user_data) {
    AppStoreItem *item = (AppStoreItem*)user_data;
    if (!item) return;

    g_autoptr(GError) err = NULL;
    bool ok = false;

    /* Trường `icon` trong danh mục thường chính là desktop id kiểu reverse-DNS
     * (com.obsproject.Studio, org.gnome.Evince, org.kde.krita...). */
    g_autofree char *id_from_icon = g_strdup_printf("%s.desktop", item->icon);
    ok = tizen_app_launch_desktop_id(id_from_icon, &err);

    if (!ok) {
        g_clear_error(&err);
        g_autofree char *id_from_pkg = g_strdup_printf("%s.desktop", item->package_name);
        ok = tizen_app_launch_desktop_id(id_from_pkg, &err);
    }
    if (!ok) {
        g_clear_error(&err);
        ok = tizen_app_launch_exec(item->package_name, &err);
    }

    if (!ok) {
        GtkWidget *parent = GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(btn)));
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_IS_WINDOW(parent) ? GTK_WINDOW(parent) : NULL,
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Không mở được \"%s\".\n\n%s\n\nMột số gói (git, docker, python3) là "
            "công cụ dòng lệnh, hãy dùng qua Terminal.",
            item->name, err ? err->message : "Không tìm thấy tệp .desktop tương ứng.");
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_window_present(GTK_WINDOW(dialog));
    }
}

static const char* resolve_app_icon(GtkIconTheme *theme, const char *raw_icon, const char *category) {
    if (!theme) return "application-x-executable";

    if (gtk_icon_theme_has_icon(theme, raw_icon)) return raw_icon;

    if (g_strcmp0(raw_icon, "firefox-esr") == 0 && gtk_icon_theme_has_icon(theme, "firefox")) return "firefox";
    if (g_strcmp0(raw_icon, "google-chrome-stable") == 0 && gtk_icon_theme_has_icon(theme, "google-chrome")) return "google-chrome";
    if (g_strcmp0(raw_icon, "brave-browser") == 0 && gtk_icon_theme_has_icon(theme, "brave")) return "brave";
    if (g_strcmp0(raw_icon, "telegram-desktop") == 0 && gtk_icon_theme_has_icon(theme, "telegram")) return "telegram";
    if (g_strcmp0(raw_icon, "thunderbird") == 0 && gtk_icon_theme_has_icon(theme, "mail-client")) return "mail-client";
    if (g_strcmp0(raw_icon, "com.obsproject.Studio") == 0 || g_strcmp0(raw_icon, "obs-studio") == 0) {
        if (gtk_icon_theme_has_icon(theme, "obs")) return "obs";
        if (gtk_icon_theme_has_icon(theme, "com.obsproject.Studio")) return "com.obsproject.Studio";
    }
    if (g_strcmp0(raw_icon, "org.inkscape.Inkscape") == 0 && gtk_icon_theme_has_icon(theme, "inkscape")) return "inkscape";
    if (g_strcmp0(raw_icon, "org.shotcut.Shotcut") == 0 && gtk_icon_theme_has_icon(theme, "shotcut")) return "shotcut";
    if (g_strcmp0(raw_icon, "org.darktable.darktable") == 0 && gtk_icon_theme_has_icon(theme, "darktable")) return "darktable";
    if (g_strcmp0(raw_icon, "fr.handbrake.ghb") == 0 && gtk_icon_theme_has_icon(theme, "handbrake")) return "handbrake";
    if (g_strcmp0(raw_icon, "org.kde.krita") == 0 && gtk_icon_theme_has_icon(theme, "krita")) return "krita";
    if (g_strcmp0(raw_icon, "libreoffice-startcenter") == 0 && gtk_icon_theme_has_icon(theme, "libreoffice-main")) return "libreoffice-main";
    if (g_strcmp0(raw_icon, "org.gnome.Evince") == 0 && gtk_icon_theme_has_icon(theme, "evince")) return "evince";
    if (g_strcmp0(raw_icon, "onlyoffice-desktopeditors") == 0 && gtk_icon_theme_has_icon(theme, "onlyoffice")) return "onlyoffice";
    if (g_strcmp0(raw_icon, "sublime-text") == 0 && gtk_icon_theme_has_icon(theme, "sublime")) return "sublime";
    if (g_strcmp0(raw_icon, "docker-ce") == 0 && gtk_icon_theme_has_icon(theme, "docker")) return "docker";
    if (g_strcmp0(raw_icon, "python3-full") == 0 && gtk_icon_theme_has_icon(theme, "python3")) return "python3";
    if (g_strcmp0(raw_icon, "net.lutris.Lutris") == 0 && gtk_icon_theme_has_icon(theme, "lutris")) return "lutris";
    if (g_strcmp0(raw_icon, "com.heroicgameslauncher.hgl") == 0 && gtk_icon_theme_has_icon(theme, "heroic")) return "heroic";

    if (g_strcmp0(category, "internet") == 0) return "web-browser";
    if (g_strcmp0(category, "graphics") == 0) return "multimedia-player";
    if (g_strcmp0(category, "office") == 0) return "x-office-document";
    if (g_strcmp0(category, "dev") == 0) return "utilities-terminal";
    if (g_strcmp0(category, "games") == 0) return "input-gaming";
    if (g_strcmp0(category, "system") == 0) return "preferences-system";

    return "application-x-executable";
}

static void show_app_details_modal(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AppStoreItem *item = (AppStoreItem*)user_data;
    if (!item) return;

    GtkWidget *parent = GTK_WIDGET(gtk_widget_get_root(catalog_flowbox));
    GtkWidget *dialog = gtk_window_new();
    if (parent && GTK_IS_WINDOW(parent)) {
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    }
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_title(GTK_WINDOW(dialog), item->name);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 640, 520);

    GtkWidget *content_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(content_vbox, 20);
    gtk_widget_set_margin_end(content_vbox, 20);
    gtk_widget_set_margin_top(content_vbox, 20);
    gtk_widget_set_margin_bottom(content_vbox, 20);

    // Header Info
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    GtkIconTheme *theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    const char *icon_name = resolve_app_icon(theme, item->icon, item->category);
    GtkWidget *img_icon = gtk_image_new_from_icon_name(icon_name);
    gtk_image_set_pixel_size(GTK_IMAGE(img_icon), 64);
    gtk_box_append(GTK_BOX(header_box), img_icon);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *lbl_name = gtk_label_new(NULL);
    /* g_markup_printf_escaped: escape phần thay thế %s, giữ nguyên markup literal.
     * Xem ghi chú ở refresh_app_store_catalog() về lý do bắt buộc phải escape. */
    char *title_markup = g_markup_printf_escaped(
        "<span size='xx-large' weight='bold'>%s</span>", item->name);
    gtk_label_set_markup(GTK_LABEL(lbl_name), title_markup);
    g_free(title_markup);
    gtk_label_set_xalign(GTK_LABEL(lbl_name), 0.0f);

    char dev_str[256];
    snprintf(dev_str, sizeof(dev_str), "Phát triển bởi %s  •  ⭐ %s  •  Dung lượng: %s", item->developer, item->rating, item->size_str);
    GtkWidget *lbl_dev = gtk_label_new(dev_str);
    gtk_label_set_xalign(GTK_LABEL(lbl_dev), 0.0f);

    gtk_box_append(GTK_BOX(title_box), lbl_name);
    gtk_box_append(GTK_BOX(title_box), lbl_dev);
    gtk_box_append(GTK_BOX(header_box), title_box);
    gtk_box_append(GTK_BOX(content_vbox), header_box);

    // Screenshot Box Preview
    char screenshot_file[512];
    snprintf(screenshot_file, sizeof(screenshot_file), "/usr/share/tizen-store/screenshots/%s.jpg", item->id);
    GtkWidget *img_screenshot = NULL;
    if (g_file_test(screenshot_file, G_FILE_TEST_EXISTS)) {
        img_screenshot = gtk_picture_new_for_filename(screenshot_file);
        gtk_widget_set_size_request(img_screenshot, 600, 280);
    } else {
        // Fallback banner card
        img_screenshot = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_add_css_class(img_screenshot, "card");
        gtk_widget_set_size_request(img_screenshot, 600, 180);
        GtkWidget *lbl_preview = gtk_label_new("🖼️ Ảnh xem trước giao diện ứng dụng TizenOS");
        gtk_widget_set_vexpand(lbl_preview, TRUE);
        gtk_box_append(GTK_BOX(img_screenshot), lbl_preview);
    }
    gtk_box_append(GTK_BOX(content_vbox), img_screenshot);

    // Description text
    GtkWidget *lbl_desc = gtk_label_new(item->description);
    gtk_label_set_wrap(GTK_LABEL(lbl_desc), TRUE);
    gtk_label_set_xalign(GTK_LABEL(lbl_desc), 0.0f);
    gtk_box_append(GTK_BOX(content_vbox), lbl_desc);

    // Action buttons
    GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_close = tizen_button_new("window-close-symbolic", "Đóng");
    g_signal_connect_swapped(btn_close, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    gtk_box_append(GTK_BOX(action_box), btn_close);

    if (item->is_installed) {
        GtkWidget *btn_launch = tizen_button_new("media-playback-start-symbolic", "Mở Ứng Dụng");
        g_signal_connect(btn_launch, "clicked", G_CALLBACK(on_launch_app_clicked), item);
        gtk_box_append(GTK_BOX(action_box), btn_launch);
    } else {
        GtkWidget *btn_install = tizen_button_new("folder-download-symbolic", "Cài Đặt Ngay");
        gtk_widget_add_css_class(btn_install, "suggested-action");
        g_signal_connect(btn_install, "clicked", G_CALLBACK(on_install_app_clicked), item);
        gtk_box_append(GTK_BOX(action_box), btn_install);
    }
    gtk_box_append(GTK_BOX(content_vbox), action_box);

    gtk_window_set_child(GTK_WINDOW(dialog), content_vbox);
    gtk_window_present(GTK_WINDOW(dialog));
}

void refresh_app_store_catalog(const char *category_filter, const char *search_query) {
    if (!catalog_flowbox) return;

    // Clear old children safely
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(catalog_flowbox)) != NULL) {
        gtk_flow_box_remove(GTK_FLOW_BOX(catalog_flowbox), child);
    }

    /* Quét trạng thái đúng MỘT lần cho cả phiên, thay vì mỗi lần vẽ lại.
     * Dùng app_store_invalidate_states() sau khi cài/gỡ để buộc quét lại. */
    if (!states_scanned)
        rescan_installed_states();

    char *query_fold = (search_query && strlen(search_query) > 0) ? g_utf8_casefold(search_query, -1) : NULL;

    int rendered = 0;
    for (int i = 0; i < catalog_size; i++) {
        AppStoreItem *item = &store_catalog[i];

        // Category Filter
        if (category_filter && strcmp(category_filter, "all") != 0) {
            if (strcmp(category_filter, item->category) != 0) continue;
        }

        // Search Filter
        if (query_fold) {
            char *name_fold = g_utf8_casefold(item->name, -1);
            char *desc_fold = g_utf8_casefold(item->description, -1);
            bool match = (strstr(name_fold, query_fold) != NULL || strstr(desc_fold, query_fold) != NULL);
            g_free(name_fold); g_free(desc_fold);
            if (!match) continue;
        }

        // App Card Widget (GTK4 Card Box)
        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_add_css_class(card, "card");
        gtk_widget_set_size_request(card, 260, 220);
        gtk_widget_set_margin_start(card, 8);
        gtk_widget_set_margin_end(card, 8);
        gtk_widget_set_margin_top(card, 8);
        gtk_widget_set_margin_bottom(card, 8);

        // Header (Icon + Name + Developer)
        GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

        GtkIconTheme *theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
        const char *icon_name = resolve_app_icon(theme, item->icon, item->category);
        GtkWidget *img_icon = gtk_image_new_from_icon_name(icon_name);
        gtk_image_set_pixel_size(GTK_IMAGE(img_icon), 52);
        gtk_box_append(GTK_BOX(top_box), img_icon);

        GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget *lbl_name = gtk_label_new(NULL);
        /* ---------------------------------------------------------------------
         * PHẢI escape tên ứng dụng trước khi đưa vào markup.
         * ---------------------------------------------------------------------
         * gtk_label_set_markup() phân tích Pango markup, nên một dấu & trần làm
         * hỏng cú pháp và Pango BỎ TOÀN BỘ chuỗi — nhãn hiện ra trống trơn.
         * Không phải lỗi lý thuyết: chính danh mục này có
         *      "GCC & G++ Compiler"  và  "Tizen Album Photo & Video"
         * và cả hai thẻ đều mất tên. Chạy app in ra:
         *      Failed to set text '<b>GCC & G++ Compiler</b>' from markup due to
         *      error parsing markup: Entity did not end with a semicolon
         * ------------------------------------------------------------------ */
        char *title_str = g_markup_printf_escaped("<b>%s</b>", item->name);
        gtk_label_set_markup(GTK_LABEL(lbl_name), title_str);
        g_free(title_str);
        gtk_label_set_xalign(GTK_LABEL(lbl_name), 0.0f);

        GtkWidget *lbl_dev = gtk_label_new(item->developer);
        gtk_label_set_xalign(GTK_LABEL(lbl_dev), 0.0f);
        gtk_widget_add_css_class(lbl_dev, "dim-label");

        gtk_box_append(GTK_BOX(name_box), lbl_name);
        gtk_box_append(GTK_BOX(name_box), lbl_dev);
        gtk_box_append(GTK_BOX(top_box), name_box);
        gtk_box_append(GTK_BOX(card), top_box);

        // Description
        GtkWidget *lbl_desc = gtk_label_new(item->description);
        gtk_label_set_wrap(GTK_LABEL(lbl_desc), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(lbl_desc), 28);
        gtk_label_set_xalign(GTK_LABEL(lbl_desc), 0.0f);
        gtk_widget_set_vexpand(lbl_desc, TRUE);
        gtk_box_append(GTK_BOX(card), lbl_desc);

        /* Không tạo sẵn progress bar ở đây.
         * Dòng cũ `GtkWidget *pbar = gtk_progress_bar_new();` không bao giờ được
         * gắn vào thẻ và cũng không được dùng: on_install_app_clicked() tự tìm
         * hoặc tự tạo progress bar khi thật sự bắt đầu cài. Đó là một GObject
         * floating không ai sink, rò rỉ một cái cho MỖI thẻ, mỗi lần vẽ lại danh
         * mục — tức mỗi ký tự gõ vào ô tìm kiếm. */
        // Footer (Rating + Size + Action Button + Details Button)
        GtkWidget *bot_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        char meta_str[128];
        snprintf(meta_str, sizeof(meta_str), "⭐ %s • %s", item->rating, item->size_str);
        GtkWidget *lbl_meta = gtk_label_new(meta_str);
        gtk_widget_set_hexpand(lbl_meta, TRUE);
        gtk_label_set_xalign(GTK_LABEL(lbl_meta), 0.0f);
        gtk_box_append(GTK_BOX(bot_box), lbl_meta);

        GtkWidget *btn_details = tizen_button_new("dialog-information-symbolic", "Chi tiết");
        g_signal_connect(btn_details, "clicked", G_CALLBACK(show_app_details_modal), item);
        gtk_box_append(GTK_BOX(bot_box), btn_details);

        if (item->is_installed) {
            GtkWidget *btn_launch = tizen_button_new("media-playback-start-symbolic", "Mở App");
            g_signal_connect(btn_launch, "clicked", G_CALLBACK(on_launch_app_clicked), item);
            gtk_box_append(GTK_BOX(bot_box), btn_launch);
        } else {
            GtkWidget *btn_install = tizen_button_new("folder-download-symbolic", "Cài Đặt");
            g_signal_connect(btn_install, "clicked", G_CALLBACK(on_install_app_clicked), item);
            gtk_box_append(GTK_BOX(bot_box), btn_install);
        }

        gtk_box_append(GTK_BOX(card), bot_box);
        gtk_flow_box_append(GTK_FLOW_BOX(catalog_flowbox), card);
        rendered++;
    }

    if (query_fold) g_free(query_fold);

    if (rendered == 0) {
        GtkWidget *empty_lbl = gtk_label_new("Không tìm thấy ứng dụng phù hợp trong kho.");
        gtk_widget_set_margin_top(empty_lbl, 24);
        gtk_flow_box_append(GTK_FLOW_BOX(catalog_flowbox), empty_lbl);
    }
}

GtkWidget* create_app_store_catalog_view(const char *category_filter, const char *search_query) {
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);

    catalog_flowbox = gtk_flow_box_new();
    gtk_widget_set_valign(GTK_WIDGET(catalog_flowbox), GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(catalog_flowbox), 4);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(catalog_flowbox), GTK_SELECTION_NONE);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), catalog_flowbox);

    refresh_app_store_catalog(category_filter, search_query);

    return scrolled;
}
