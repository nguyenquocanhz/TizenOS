#include <sqlite3.h>
#include <stdio.h>

static sqlite3 *db = NULL;

void media_db_init(void) {
    int rc = sqlite3_open("/var/lib/tizen-media/media.db", &db);
    if (rc) {
        fprintf(stderr, "Không thể mở database SQLite: %s\n", sqlite3_errmsg(db));
        return;
    }
    
    // Tạo bảng nếu chưa tồn tại
    const char *sql = "CREATE TABLE IF NOT EXISTS media ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "filepath TEXT UNIQUE,"
                      "duration INTEGER,"
                      "codec TEXT);";
    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Lỗi SQL: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("Khởi tạo bảng SQLite thành công.\n");
    }
}
