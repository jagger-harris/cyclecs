#include "core/util/logger.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define STR_MAX 32

// ANSI Colors
#define ANSI_RED "\x1b[31m"
#define ANSI_BRIGHT_RED "\x1b[91m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BRIGHT_WHITE "\x1b[97m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_WHITE "\x1b[37m"
#define ANSI_DEFAULT "\x1b[0m"

static void logger_log_(enum logger_level level, int err, bool has_err,
                        const char *fmt, va_list args) {
    char level_str[STR_MAX];
    char color_str[STR_MAX];

    switch (level) {
    case LOGGER_FATAL:
        snprintf(level_str, STR_MAX, "FATAL");
        snprintf(color_str, STR_MAX, ANSI_RED);
        break;
    case LOGGER_ERR:
        snprintf(level_str, STR_MAX, "ERROR");
        snprintf(color_str, STR_MAX, ANSI_BRIGHT_RED);
        break;
    case LOGGER_WARN:
        snprintf(level_str, STR_MAX, "WARN");
        snprintf(color_str, STR_MAX, ANSI_YELLOW);
        break;
    case LOGGER_INFO:
        snprintf(level_str, STR_MAX, "INFO");
        snprintf(color_str, STR_MAX, ANSI_BRIGHT_WHITE);
        break;
    case LOGGER_DEBUG:
        snprintf(level_str, STR_MAX, "DEBUG");
        snprintf(color_str, STR_MAX, ANSI_CYAN);
        break;
    default:
        snprintf(level_str, STR_MAX, "UNKWN");
        snprintf(color_str, STR_MAX, ANSI_WHITE);
        break;
    }

    time_t t = time(NULL);
    struct tm *t_info = localtime(&t);

    printf("%s[%02d:%02d:%02d] [%s]", color_str, t_info->tm_hour,
           t_info->tm_min, t_info->tm_sec, level_str);

    if (has_err)
        printf(" [%d]", err);

    printf(": ");
    vprintf(fmt, args);
    printf("%s\n", ANSI_DEFAULT);
}

void logger_log_err(enum logger_level level, int err, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log_(level, err, 1, fmt, args);
    va_end(args);
}

void logger_log(enum logger_level level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log_(level, 0, 0, fmt, args);
    va_end(args);
}
