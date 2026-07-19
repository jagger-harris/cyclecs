/**
 * @file cls/util/xxhash32.h
 * @brief xxHash32 for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/xxhash32.c
 */

#ifndef CLS_XXHASH32_H
#define CLS_XXHASH32_H

#include <cls/util/error.h>
#include <cls/util/types.h>
#include <stddef.h>

/**
 * @defgroup xxhash32 xxHash32
 * @ingroup util
 * @brief xxHash32 algorithm used throughout.
 * @{
 */

/**
 * @brief Computes a 32 bit hash.
 *
 * Computes the xxHash32 value of a buffer.
 *
 * @param[out] hash   Hash value.
 * @param[in]  input  Data to hash.
 * @param[in]  length Data size in bytes.
 * @param[in]  seed   Hash seed.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `hash` or `input` is NULL.
 * @retval (error)     If reading the input data fails.
 */
cls_error cls_xxhash32(u32 *hash, const void *input, size_t length, u32 seed);

/** @} */

#endif // CLS_XXHASH32_H
