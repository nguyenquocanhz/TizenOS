#ifndef ARCHIVE_ENGINE_H
#define ARCHIVE_ENGINE_H

#include <stdbool.h>

/**
 * Giải nén tập tin archive.
 * Hỗ trợ: .zip, .tar.gz, .tar.xz, .tar.zst, .7z, .rar, .tpk, .deb
 */
bool extract_archive(const char *archive_path, const char *dest_dir);

/**
 * Nén thư mục/tập tin.
 */
bool create_archive(const char *archive_path, const char **files, int num_files);

#endif // ARCHIVE_ENGINE_H
