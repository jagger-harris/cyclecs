/**
 * @file cls/io/ascii.h
 * @brief Ascii text management for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/io/ascii.c
 */

#ifndef CLS_ASCII_H
#define CLS_ASCII_H

#include <cls/util/error.h>

/**
 * @defgroup ascii ASCII.
 * @ingroup io
 * @brief ASCII file loader.
 * @{
 */

/**
 * @brief Reads a file into a buffer.
 *
 * Opens `path`, reads the file contents into a newly allocated buffer, and
 * null terminates the buffer. Destroy the returned buffer with
 * cls_ascii_destroy().
 *
 * @param[out] ascii File contents.
 * @param[in]  path  File path.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_FILE_NOT_FOUND If `ascii` or `path` is NULL, or the file
 *                            cannot be opened.
 * @retval CLS_ACCESS_DENIED  If seeking or reading the file fails.
 * @retval CLS_OUT_OF_MEMORY  If allocating the buffer fails.
 *
 * @code
 * const char *ascii;
 * cls_ascii_init(&ascii, "shader.vert");
 * // Use ascii.
 * cls_ascii_destroy(&ascii);
 * @endcode
 */
cls_error cls_ascii_init(const char **ascii, const char *path);

/**
 * @brief Destroys a buffer.
 *
 * Releases a buffer created by cls_ascii_init() and sets it to NULL.
 *
 * @param[in,out] ascii Buffer to destroy.
 */
void cls_ascii_destroy(const char **ascii);

/** @} */

#endif // CLS_ASCII_H
