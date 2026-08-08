#include <stdio.h>
#include <sqlite3.h>

// Quản lý cơ sở dữ liệu các gói đã cài đặt sử dụng SQLite
// Bảng 'packages' lưu thông tin: pkg_id, version, installed_time

sqlite3 *db;

void pkgmgr_db_init() {
    printf("Khởi tạo SQLite DB cho pkgmgr...\n");
    int rc = sqlite3_open("/var/lib/pkgmgr/pkgmgr.db", &db);
    if (rc) {
        fprintf(stderr, "Không thể mở CSDL: %s\n", sqlite3_errmsg(db));
    } else {
        const char *sql = "CREATE TABLE IF NOT EXISTS packages (pkg_id TEXT PRIMARY KEY, version TEXT, installed_time INTEGER);";
        char *err_msg = NULL;
        sqlite3_exec(db, sql, 0, 0, &err_msg);
    }
}
