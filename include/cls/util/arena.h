#ifndef CLS_ARENA_H
#define CLS_ARENA_H

#include <stddef.h>

/**
 * @typedef cls_arena_marker
 * @brief Arena allocation position.
 *
 * Stores a saved allocation position in an arena. Can be used with
 * cls_arena_marker_restore() to restore the arena state.
 *
 * @note Does not release memory.
 */
typedef size_t cls_arena_marker;

/**
 * @struct cls_arena
 * @brief Linear allocator.
 */
struct cls_arena;

/**
 * @brief Creates an arena.
 *
 * Allocates an arena with the given size. Destroy the returned arena with
 * cls_arena_destroy().
 *
 * @param[out] a    Arena.
 * @param[in]  size Arena size in bytes.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `a` is NULL.
 * @retval CLS_INVALID_ARG   If `size` is zero.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 *
 * @code
 * struct cls_arena *a;
 * cls_arena_create(&a, 1024);
 * // Use a.
 * cls_arena_destroy(a);
 * @endcode
 */
int cls_arena_create(struct cls_arena **a, size_t size);

/**
 * @brief Destroys an arena.
 *
 * Releases the arena memory.
 *
 * @param[in] a Arena to destroy.
 */
void cls_arena_destroy(struct cls_arena *a);

/**
 * @brief Allocates arena memory.
 *
 * Allocates `size` bytes with the requested alignment.
 *
 * @param[out] dest  Allocated memory.
 * @param[in]  a     Arena.
 * @param[in]  size  Allocation size.
 * @param[in]  align Allocation alignment.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `dest` or `a` is NULL.
 * @retval CLS_INVALID_ARG   If `size` is zero or `align` is invalid.
 * @retval CLS_OUT_OF_MEMORY If there is not enough space.
 *
 * @warning Memory is invalid after clearing, destroying, or restoring the
 *          arena.
 *
 * @code
 * void *ptr;
 * cls_arena_alloc(&ptr, a, 100, alignof(int));
 * @endcode
 */
int cls_arena_alloc(void **dest, struct cls_arena *a, size_t size,
                    size_t align);

/**
 * @brief Clears an arena.
 *
 * Resets the allocation position without releasing memory.
 *
 * @param[in] a Arena.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `a` is NULL.
 *
 * @code
 * cls_arena_clear(a);
 * @endcode
 */
int cls_arena_clear(struct cls_arena *a);

/**
 * @brief Saves an arena marker.
 *
 * Stores the current allocation position for later restoration.
 *
 * @param[out] marker Arena marker.
 * @param[in]  a      Arena.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `marker` or `a` is NULL.
 *
 * @code
 * cls_arena_marker marker;
 * cls_arena_marker_save(&marker, a);
 * @endcode
 */
int cls_arena_marker_save(cls_arena_marker *marker, struct cls_arena *a);

/**
 * @brief Restores an arena marker.
 *
 * Restores the arena allocation position and invalidates allocations made
 * after the marker.
 *
 * @param[in] a      Arena.
 * @param[in] marker Arena marker.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `a` or `marker` is NULL.
 *
 * @code
 * cls_arena_marker marker;
 * cls_arena_marker_save(&marker, a);
 *
 * // Allocate memory.
 *
 * cls_arena_marker_restore(a, &marker);
 * @endcode
 */
int cls_arena_marker_restore(struct cls_arena *a, cls_arena_marker *marker);

#endif // CLS_ARENA_H
