/**
 * @file cls/util/types.h
 * @brief Simplified types for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 */

#ifndef CLS_TYPES_H
#define CLS_TYPES_H

#include <stdint.h>

/**
 * @defgroup types Types
 * @ingroup util
 * @brief Basic type definitions used.
 * @{
 */

#define I8_MIN INT8_MIN /**< Minimum value of an i8. */
#define I16_MIN INT16_MIN /**< Minimum value of an i16. */
#define I32_MIN INT32_MIN /**< Minimum value of an i32. */
#define I64_MIN INT64_MIN /**< Minimum value of an i64. */
#define I8_MAX INT8_MAX /**< Maximum value of an i8. */
#define I16_MAX INT16_MAX /**< Maximum value of an i16. */
#define I32_MAX INT32_MAX /**< Maximum value of an i32. */
#define I64_MAX INT64_MAX /**< Maximum value of an i64. */
#define U8_MAX UINT8_MAX /**< Maximum value of a u8. */
#define U16_MAX UINT16_MAX /**< Maximum value of a u16. */
#define U32_MAX UINT32_MAX /**< Maximum value of a u32. */
#define U64_MAX UINT64_MAX /**< Maximum value of a u64. */

typedef int8_t i8; /**< Signed 8-bit integer. */
typedef int16_t i16; /**< Signed 16-bit integer. */
typedef int32_t i32; /**< Signed 32-bit integer. */
typedef int64_t i64; /**< Signed 64-bit integer. */
typedef uint8_t u8; /**< Unsigned 8-bit integer. */
typedef uint16_t u16; /**< Unsigned 16-bit integer. */
typedef uint32_t u32; /**< Unsigned 32-bit integer. */
typedef uint64_t u64; /**< Unsigned 64-bit integer. */

/** @} */

#endif // CLS_TYPES_H
