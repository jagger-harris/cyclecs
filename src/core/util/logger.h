#ifndef LOGGER_H
#define LOGGER_H

enum logger_level {
    LOGGER_LOG_LEVEL_FATAL,
    LOGGER_LOG_LEVEL_ERROR,
    LOGGER_LOG_LEVEL_WARNING,
    LOGGER_LOG_LEVEL_INFO,
    LOGGER_LOG_LEVEL_DEBUG
};

void logger_log(enum logger_level level, const char *message, int error_code);

#endif /* LOGGER_H */
