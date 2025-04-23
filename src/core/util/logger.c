#include "core/util/logger.h"
#include <stdio.h>
#include <time.h>

#define STR_MAX 16
#define ANSI_BLACK "\x1b[30m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_WHITE "\x1b[37m"
#define ANSI_BRIGHT_BLACK "\x1b[90m"
#define ANSI_BRIGHT_RED "\x1b[91m"
#define ANSI_BRIGHT_GREEN "\x1b[92m"
#define ANSI_BRIGHT_YELLOW "\x1b[93m"
#define ANSI_BRIGHT_BLUE "\x1b[94m"
#define ANSI_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_BRIGHT_CYAN "\x1b[96m"
#define ANSI_BRIGHT_WHITE "\x1b[97m"
#define ANSI_DEFAULT "\x1b[m"

void logger_log(enum logger_level level, const char *msg, int err) {
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

    time_t t;
    struct tm *t_info;
    time(&t);
    t_info = localtime(&t);

    if (err) {
        printf("%s[%02d:%02d:%02d] [%s] [%i]: %s%s\n", color_str,
               t_info->tm_hour, t_info->tm_min, t_info->tm_sec, level_str, err,
               msg, ANSI_DEFAULT);
    } else {
        printf("%s[%02d:%02d:%02d] [%s]: %s%s\n", color_str, t_info->tm_hour,
               t_info->tm_min, t_info->tm_sec, level_str, msg, ANSI_DEFAULT);
    }
}
