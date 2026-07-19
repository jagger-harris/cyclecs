/**
 * @file cls/util/error.h
 * @brief Error codes used throughout the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 */

#ifndef CLS_ERROR_H
#define CLS_ERROR_H

/**
 * @defgroup error Error
 * @ingroup util
 * @brief Error codes used.
 * @{
 */

/**
 * @enum cls_error
 * @brief Error codes.
 */
typedef enum {
    CLS_SUCCESS = 0, /**< Operation completed successfully. */
    CLS_FAILURE, /**< Operation failed. */
    CLS_NULLPTR, /**< Required pointer was NULL. */
    CLS_OUT_OF_MEMORY, /**< Memory allocation failed. */
    CLS_DIVIDE_BY_ZERO, /**< Divide by zero. */
    CLS_ACCESS_DENIED, /**< Access was denied. */
    CLS_INVALID_ARG, /**< Invalid argument. */
    CLS_FILE_NOT_FOUND, /**< File was not found. */
    CLS_FILE_CORRUPT, /**< File contents are invalid. */
    CLS_GLFW, /**< GLFW operation failed. */
    CLS_GL, /**< OpenGL operation failed. */
} cls_error;

/** @} */

#endif // CLS_ERROR_H
