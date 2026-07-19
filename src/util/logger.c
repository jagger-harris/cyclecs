/**
 * @file cls/util/logger.c
 * @brief Logger for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/logger.h
 */

#include <cls/util/logger.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

static void logger_log_all(enum cls_logger_level level, bool has_error,
                           bool is_debug, cls_error error, const char *file,
                           const char *func, int line, const char *fmt,
                           va_list args) {
    char level_str[STR_MAX];
    char color_str[STR_MAX];

    int ret = -1;
    switch (level) {
    case CLS_LOGGER_FATAL:
        ret = snprintf(level_str, STR_MAX, "FATAL");
        if (ret < 0)
            return;

        ret = snprintf(color_str, STR_MAX, ANSI_RED);
        if (ret < 0)
            return;

        break;
    case CLS_LOGGER_ERROR:
        ret = snprintf(level_str, STR_MAX, "ERROR");
        if (ret < 0)
            return;

        ret = snprintf(color_str, STR_MAX, ANSI_BRIGHT_RED);
        if (ret < 0)
            return;

        break;
    case CLS_LOGGER_WARN:
        ret = snprintf(level_str, STR_MAX, "WARN");
        if (ret < 0)
            return;

        ret = snprintf(color_str, STR_MAX, ANSI_YELLOW);
        if (ret < 0)
            return;

        break;
    case CLS_LOGGER_INFO:
        ret = snprintf(level_str, STR_MAX, "INFO");
        if (ret < 0)
            return;

        ret = snprintf(color_str, STR_MAX, ANSI_BRIGHT_WHITE);
        if (ret < 0)
            return;

        break;
    case CLS_LOGGER_DEBUG:
        ret = snprintf(level_str, STR_MAX, "DEBUG");
        if (ret < 0)
            return;

        ret = snprintf(color_str, STR_MAX, ANSI_CYAN);
        if (ret < 0)
            return;

        break;
    default:
        ret = snprintf(level_str, STR_MAX, "UNKWN");
        if (ret < 0)
            return;

        ret = snprintf(color_str, STR_MAX, ANSI_WHITE);
        if (ret < 0)
            return;

        break;
    }

    time_t t = time(NULL);
    struct tm t_info;

    if (localtime_r(&t, &t_info) == NULL)
        memset(&t_info, 0, sizeof(t_info));

    printf("%s[%02d:%02d:%02d] [%s]", color_str, t_info.tm_hour, t_info.tm_min,
           t_info.tm_sec, level_str);

    if (has_error)
        printf(" [%d]", error);

    if (is_debug)
        printf(" [%s:%d]", func, line);

    printf(": ");
    vprintf(fmt, args);

    if (has_error && is_debug)
        printf("\n[%s]", file);

    printf("%s\n", ANSI_DEFAULT);
}

void cls_logger_log(enum cls_logger_level level, const char *fmt, ...) {
    if (!fmt)
        return;

    va_list args;
    va_start(args, fmt);
    logger_log_all(level, false, false, 0, NULL, NULL, 0, fmt, args);
    va_end(args);
}

void cls_logger_log_debug(enum cls_logger_level level, const char *file,
                          const char *func, int line, const char *fmt, ...) {
    if (!fmt)
        return;

    va_list args;
    va_start(args, fmt);
    logger_log_all(level, false, true, 0, file, func, line, fmt, args);
    va_end(args);
}

void cls_logger_log_error(enum cls_logger_level level, cls_error error,
                          const char *fmt, ...) {
    if (!fmt)
        return;

    va_list args;
    va_start(args, fmt);
    logger_log_all(level, true, false, error, NULL, NULL, 0, fmt, args);
    va_end(args);
}

void cls_logger_log_error_debug(enum cls_logger_level level, cls_error error,
                                const char *file, const char *func, int line,
                                const char *fmt, ...) {
    if (!fmt)
        return;

    va_list args;
    va_start(args, fmt);
    logger_log_all(level, true, true, error, file, func, line, fmt, args);
    va_end(args);
}
