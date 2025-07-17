#ifndef UTIL_LOGGER_H
#define UTIL_LOGGER_H

enum logger_level {
    LOGGER_FATAL,
    LOGGER_ERR,
    LOGGER_WARN,
    LOGGER_INFO,
    LOGGER_DEBUG
};

void logger_log_err(enum logger_level level, int err, const char *fmt, ...);
void logger_log(enum logger_level level, const char *fmt, ...);

#endif // UTIL_LOGGER_H
