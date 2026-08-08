#include "archive-engine.h"
#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Giải nén tập tin archive (hỗ trợ .zip, .tar.gz, .7z, v.v...)
bool extract_archive(const char *archive_path, const char *dest_dir) {
    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
    int r;

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    if ((r = archive_read_open_filename(a, archive_path, 10240))) {
        fprintf(stderr, "Lỗi mở file %s: %s\n", archive_path, archive_error_string(a));
        return false;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        // Cập nhật đường dẫn lưu file vào dest_dir
        const char* current_file = archive_entry_pathname(entry);
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dest_dir, current_file);
        archive_entry_set_pathname(entry, full_path);

        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK) {
            fprintf(stderr, "Lỗi ghi header: %s\n", archive_error_string(ext));
        } else if (archive_entry_size(entry) > 0) {
            const void *buff;
            size_t size;
            int64_t offset;
            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                if (archive_write_data_block(ext, buff, size, offset) < ARCHIVE_OK) {
                    fprintf(stderr, "Lỗi ghi dữ liệu: %s\n", archive_error_string(ext));
                    break;
                }
            }
            if (r != ARCHIVE_EOF && r != ARCHIVE_OK) {
                fprintf(stderr, "Lỗi đọc dữ liệu block: %s\n", archive_error_string(a));
            }
        }
        archive_write_finish_entry(ext);
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return true;
}

// Nén file đơn giản
bool create_archive(const char *archive_path, const char **files, int num_files) {
    // Chưa triển khai hoàn chỉnh (để gọn cho demo)
    return true;
}
