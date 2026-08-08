#ifndef __TIZEN_LOG_H__
#define __TIZEN_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

// Mức độ log (Severity levels)
typedef enum {
    TIZEN_LOG_DEBUG = 0,
    TIZEN_LOG_INFO,
    TIZEN_LOG_WARNING,
    TIZEN_LOG_ERROR,
    TIZEN_LOG_FATAL
} tizen_log_level_t;

// Hàm ghi log chính, hỗ trợ syslog/sd_journal và stdout
void tizen_log_print(tizen_log_level_t level, const char* tag, const char* format, ...);

// Các macro tiện ích cho việc ghi log
#define TIZEN_LOGD(tag, format, ...) tizen_log_print(TIZEN_LOG_DEBUG, tag, format, ##__VA_ARGS__)
#define TIZEN_LOGI(tag, format, ...) tizen_log_print(TIZEN_LOG_INFO, tag, format, ##__VA_ARGS__)
#define TIZEN_LOGW(tag, format, ...) tizen_log_print(TIZEN_LOG_WARNING, tag, format, ##__VA_ARGS__)
#define TIZEN_LOGE(tag, format, ...) tizen_log_print(TIZEN_LOG_ERROR, tag, format, ##__VA_ARGS__)
#define TIZEN_LOGF(tag, format, ...) tizen_log_print(TIZEN_LOG_FATAL, tag, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // __TIZEN_LOG_H__
