#include <tizen/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <systemd/sd-journal.h>

// Hàm ghi log tích hợp sd_journal
void tizen_log_print(tizen_log_level_t level, const char* tag, const char* format, ...) {
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    int priority = LOG_INFO;
    const char* level_str = "INFO";
    switch (level) {
        case TIZEN_LOG_DEBUG:   priority = LOG_DEBUG;   level_str = "DEBUG"; break;
        case TIZEN_LOG_INFO:    priority = LOG_INFO;    level_str = "INFO"; break;
        case TIZEN_LOG_WARNING: priority = LOG_WARNING; level_str = "WARN"; break;
        case TIZEN_LOG_ERROR:   priority = LOG_ERR;     level_str = "ERROR"; break;
        case TIZEN_LOG_FATAL:   priority = LOG_CRIT;    level_str = "FATAL"; break;
    }

    // Ghi ra sd_journal (systemd)
    sd_journal_print(priority, "[%s] %s", tag, message);

    // Ghi ra stdout với severity tags
    printf("[%s] [%s] %s\n", level_str, tag, message);
}
