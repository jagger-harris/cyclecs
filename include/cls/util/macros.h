/**
 * @file cls/util/macros.h
 * @brief Macro utils for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/macros.c
 */

#ifndef CLS_MACROS_H
#define CLS_MACROS_H

/**
 * @defgroup macros Macros
 * @ingroup util
 * @brief Macro helpers used throughout.
 * @{
 */

/**
 * @def CLS_ARRAY_LENGTH
 * @brief Gets the length of an array.
 *
 * Calculates the number of elements in a fixed size array.
 *
 * @param a Array.
 *
 * @return Number of elements in `a`.
 *
 * @note Only works with arrays that have a compile time size. Passing a
 *       pointer will return an invalid result.
 *
 * @code
 * int values[10];
 * size_t len = CLS_ARRAY_LENGTH(values);
 * @endcode
 */
#define CLS_ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))

/** @} */

#endif // CLS_MACROS_H
