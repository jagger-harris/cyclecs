/**
 * @file cls/util/mem.h
 * @brief Generic memory management for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/mem.c
 */

#ifndef CLS_MEM_H
#define CLS_MEM_H

#include <cls/util/error.h>
#include <stddef.h>

/**
 * @defgroup mem Mem
 * @ingroup util
 * @brief Generic memory management.
 * @{
 */

/**
 * @typedef cls_mem_alloc_fn
 * @brief Memory allocation callback.
 *
 * Allocates memory and stores the result in `dest`.
 *
 * @param[out] dest  Allocated memory.
 * @param[in]  ctx   Allocator context.
 * @param[in]  size  Allocation size.
 * @param[in]  align Allocation alignment.
 *
 * @return CLS_SUCCESS On success.
 * @retval (error)     Error if allocation fails.
 */
typedef cls_error (*cls_mem_alloc_fn)(void **dest, void *ctx, size_t size,
                                      size_t align);

/**
 * @typedef cls_mem_free_fn
 * @brief Memory free callback.
 *
 * Releases memory previously allocated by cls_mem_alloc_fn.
 *
 * @param[in] src Memory to free.
 * @param[in] ctx Allocator context.
 */
typedef void (*cls_mem_free_fn)(void *src, void *ctx);

/**
 * @struct cls_mem
 * @brief Memory allocator.
 */
struct cls_mem;

/**
 * @brief Creates an allocator.
 *
 * Creates a memory allocator using the provided allocation and free
 * callbacks. Destroy the returned allocator with cls_mem_destroy().
 *
 * @param[out] alloc    Allocator.
 * @param[in]  alloc_fn Allocation callback.
 * @param[in]  free     Free callback.
 * @param[in]  ctx      Allocator context.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR       If `alloc` or `alloc_fn` is NULL.
 * @retval CLS_OUT_OF_MEMORY If allocating the allocator fails.
 *
 * @note `free` may be NULL for allocators that only release memory in bulk.
 *
 * @code
 * struct cls_mem *mem;
 * cls_mem_create(&mem, allocator_arena_alloc, NULL, arena);
 * // Use mem.
 * cls_mem_destroy(mem);
 * @endcode
 */
cls_error cls_mem_create(struct cls_mem **alloc, cls_mem_alloc_fn alloc_fn,
                         cls_mem_free_fn free, void *ctx);

/**
 * @brief Destroys an allocator.
 *
 * Releases the allocator instance.
 *
 * @param[in] alloc Allocator to destroy.
 */
void cls_mem_destroy(struct cls_mem *alloc);

/**
 * @brief Allocates memory.
 *
 * Allocates memory through the allocator callback.
 *
 * @param[out] dest  Allocated memory.
 * @param[in]  alloc Allocator.
 * @param[in]  size  Allocation size.
 * @param[in]  align Allocation alignment.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `dest` or `alloc` is NULL.
 * @retval (error)     If the allocation callback fails.
 */
cls_error cls_mem_alloc(void **dest, struct cls_mem *alloc, size_t size,
                        size_t align);

/**
 * @brief Frees memory.
 *
 * Releases memory through the allocator callback.
 *
 * @param[in] alloc Allocator.
 * @param[in] src   Memory to free.
 */
void cls_mem_free(struct cls_mem *alloc, void *src);

/** @} */

#endif // CLS_MEM_H
