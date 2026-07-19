/**
 * @file cls/util/logger.h
 * @brief Logger for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/logger.c
 */

#ifndef CLS_LOGGER_H
#define CLS_LOGGER_H

#include <cls/util/error.h>

/**
 * @defgroup logger Logger
 * @ingroup util
 * @brief Logger utilty.
 * @{
 */

/**
 * @enum cls_logger_level
 * @brief Logger level.
 */
enum cls_logger_level {
    CLS_LOGGER_FATAL,
    CLS_LOGGER_ERROR,
    CLS_LOGGER_WARN,
    CLS_LOGGER_INFO,
    CLS_LOGGER_DEBUG
};

/**
 * @def CLS_LOGGER_LOG
 * @brief Logs a formatted message.
 *
 * Logs a message at the given severity level.
 *
 * @param level Severity level.
 * @param fmt   Format string.
 * @param ...   Format arguments.
 */

/**
 * @def CLS_LOGGER_LOG_ERROR
 * @brief Logs a formatted error message.
 *
 * Logs a message and an error code at the given severity level.
 *
 * @param level Severity level.
 * @param err   Error code.
 * @param fmt   Format string.
 * @param ...   Format arguments.
 */
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

/**
 * @brief Logs a formatted message.
 *
 * Logs a message at the given severity level.
 *
 * @param[in] level Severity level.
 * @param[in] fmt   Format string.
 * @param[in] ...   Format arguments.
 */
void cls_logger_log(enum cls_logger_level level, const char *fmt, ...);

/**
 * @brief Logs a formatted message.
 *
 * Logs a message at the given severity level.
 *
 * @param[in] level Severity level.
 * @param[in] file  Source file.
 * @param[in] func  Function name.
 * @param[in] line  Source line.
 * @param[in] fmt   Format string.
 * @param[in] ...   Format arguments.
 */
void cls_logger_log_debug(enum cls_logger_level level, const char *file,
                          const char *func, int line, const char *fmt, ...);

/**
 * @brief Logs a formatted error message.
 *
 * Logs a message and an error code at the given severity level.
 *
 * @param[in] level Severity level.
 * @param[in] err   Error code.
 * @param[in] fmt   Format string.
 * @param[in] ...   Format arguments.
 */
void cls_logger_log_error(enum cls_logger_level level, cls_error err,
                          const char *fmt, ...);

/**
 * @brief Logs a formatted error message.
 *
 * Logs a message and an error code at the given severity level.
 *
 * @param[in] level Severity level.
 * @param[in] err   Error code.
 * @param[in] file  Source file.
 * @param[in] func  Function name.
 * @param[in] line  Source line.
 * @param[in] fmt   Format string.
 * @param[in] ...   Format arguments.
 */
void cls_logger_log_error_debug(enum cls_logger_level level, cls_error err,
                                const char *file, const char *func, int line,
                                const char *fmt, ...);

/** @} */

#endif // CLS_LOGGER_H
