#ifndef UTIL_LOGGER_H
#define UTIL_LOGGER_H

enum logger_level {
    LOGGER_FATAL,
    LOGGER_ERR,
    LOGGER_WARN,
    LOGGER_INFO,
    LOGGER_DEBUG
};

void logger_log(enum logger_level level, const char *msg, int err);

#endif /* UTIL_LOGGER_H */
