#ifndef CLS_LOGGER_H
#define CLS_LOGGER_H

enum logger_level {
    LOGGER_FATAL,
    LOGGER_ERROR,
    LOGGER_WARN,
    LOGGER_INFO,
    LOGGER_DEBUG
};

#ifdef DEBUG
#define LOGGER_LOG(level, fmt, ...)                                            \
    logger_log_debug(level, __FILE__, __func__, __LINE__, fmt, __VA_ARGS__)
#define LOGGER_LOG_ERROR(level, err, fmt, ...)                                 \
    logger_log_error_debug(level, err, __FILE__, __func__, __LINE__, fmt,      \
                           __VA_ARGS__)
#else
#define LOGGER_LOG(level, fmt, ...) logger_log(level, fmt, __VA_ARGS__)
#define LOGGER_LOG_ERROR(level, err, fmt, ...)                                 \
    logger_log_error(level, err, fmt, __VA_ARGS__)
#endif

void logger_log(enum logger_level level, const char *fmt, ...);
void logger_log_debug(enum logger_level level, const char *file,
                      const char *func, int line, const char *fmt, ...);
void logger_log_error(enum logger_level level, int err, const char *fmt, ...);
void logger_log_error_debug(enum logger_level level, int err, const char *file,
                            const char *func, int line, const char *fmt, ...);

#endif // CLS_LOGGER_H
