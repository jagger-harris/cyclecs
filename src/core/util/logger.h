#ifndef UTIL_LOGGER_H
#define UTIL_LOGGER_H

enum logger_level {
    LOGGER_FATAL,
    LOGGER_ERROR,
    LOGGER_WARNING,
    LOGGER_INFO,
    LOGGER_DEBUG
};

void logger_log(enum logger_level level, const char *message, int error_code);

#endif /* UTIL_LOGGER_H */
