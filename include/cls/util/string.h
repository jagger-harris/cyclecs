/**
 * @file cls/util/string.h
 * @brief String utils for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/string.c
 */

#ifndef CLS_STRING_H
#define CLS_STRING_H

/**
 * @defgroup string String
 * @ingroup util
 * @brief String helper utils.
 * @{
 */

/**
 * @brief Formats a string into a newly allocated, null-terminated buffer.
 *
 * @param[in] fmt Format string.
 * @param[in] ... Format arguments.
 *
 * @return Formatted string, or NULL on failure. Caller must free().
 *
 * @warning `fmt` must not be NULL.
 */
char *cls_str_fmt(const char *fmt, ...);

/** @} */

#endif // CLS_STRING_H
