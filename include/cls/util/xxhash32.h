#ifndef CLS_XXHASH32_H
#define CLS_XXHASH32_H

#include <cls/util/error.h>
#include <cls/util/types.h>
#include <stddef.h>

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

#endif // CLS_XXHASH32_H
