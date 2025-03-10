#include "logger.h"
#include <stdio.h>

#define MAX_STR_SIZE 16

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

void logger_log(enum logger_level level, const char *message, int error_code) {
    char level_str[MAX_STR_SIZE];
    char color_str[MAX_STR_SIZE];

    switch (level) {
    case LOGGER_FATAL:
        snprintf(level_str, MAX_STR_SIZE, "FATAL");
        snprintf(color_str, MAX_STR_SIZE, ANSI_RED);
        break;
    case LOGGER_ERROR:
        snprintf(level_str, MAX_STR_SIZE, "ERROR");
        snprintf(color_str, MAX_STR_SIZE, ANSI_BRIGHT_RED);
        break;
    case LOGGER_WARNING:
        snprintf(level_str, MAX_STR_SIZE, "WARNING");
        snprintf(color_str, MAX_STR_SIZE, ANSI_YELLOW);
        break;
    case LOGGER_INFO:
        snprintf(level_str, MAX_STR_SIZE, "INFO");
        snprintf(color_str, MAX_STR_SIZE, ANSI_BRIGHT_WHITE);
        break;
    case LOGGER_DEBUG:
        snprintf(level_str, MAX_STR_SIZE, "DEBUG");
        snprintf(color_str, MAX_STR_SIZE, ANSI_CYAN);
        break;
    default:
        snprintf(level_str, MAX_STR_SIZE, "LOG");
        snprintf(color_str, MAX_STR_SIZE, ANSI_WHITE);
        break;
    }

    printf("%s[%s] %u : %s%s\n", color_str, level_str, error_code, message,
           ANSI_DEFAULT);
}
