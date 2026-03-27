#ifndef CLS_LOGGER_H
#define CLS_LOGGER_H

enum cls_logger_level {
    CLS_LOGGER_FATAL,
    CLS_LOGGER_ERROR,
    CLS_LOGGER_WARN,
    CLS_LOGGER_INFO,
    CLS_LOGGER_DEBUG
};

#ifdef DEBUG
#define CLS_LOGGER_LOG(level, fmt, ...)                                        \
    cls_logger_log_debug(level, __FILE__, __func__, __LINE__, fmt, __VA_ARGS__)
#define CLS_LOGGER_LOG_ERROR(level, err, fmt, ...)                             \
    cls_logger_log_error_debug(level, err, __FILE__, __func__, __LINE__, fmt,  \
                               __VA_ARGS__)
#else
#define CLS_LOGGER_LOG(level, fmt, ...) cls_logger_log(level, fmt, __VA_ARGS__)
#define CLS_LOGGER_LOG_ERROR(level, err, fmt, ...)                             \
    cls_logger_log_error(level, err, fmt, __VA_ARGS__)
#endif

void cls_logger_log(enum cls_logger_level level, const char *fmt, ...);
void cls_logger_log_debug(enum cls_logger_level level, const char *file,
                          const char *func, int line, const char *fmt, ...);
void cls_logger_log_error(enum cls_logger_level level, int err, const char *fmt,
                          ...);
void cls_logger_log_error_debug(enum cls_logger_level level, int err,
                                const char *file, const char *func, int line,
                                const char *fmt, ...);

#endif // CLS_LOGGER_H
