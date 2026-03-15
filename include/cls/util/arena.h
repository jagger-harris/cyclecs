#ifndef CLS_ARENA_H
#define CLS_ARENA_H

#include <stddef.h>

/**
 * @typedef arena_marker
 * @brief Represents a saved state of an arena's allocation position.
 *
 * This marker can be used with arena_marker_restore() to revert the arena
 * back to a previous allocation point.
 *
 * @note Does not free memory.
 */
typedef size_t arena_marker;

/**
 * @struct arena
 * @brief Linear memory allocator.
 */
struct arena;

/**
 * @brief Creates a new arena allocator.
 *
 * Allocates a contiguous block of memory containing both the arena metadata and
 * the aligned buffer region. Caller must free the arena using arena_destroy().
 *
 * @param[out] a  Arena instance.
 * @param[in]  size Size of the arena's usable buffer in bytes.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `a` is NULL.
 * @retval CLS_INVALID_ARG   If size is zero.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 *
 * ### Example
 * @code
 * struct arena *a;
 * if (arena_create(&a, 1024) == CLS_SUCCESS) {
 *     // use the arena
 *     arena_destroy(a);
 * }
 * @endcode
 */
int arena_create(struct arena **a, size_t size);

/**
 * @brief Destroys an arena created with arena_create().
 *
 * Frees the memory associated with the arena.
 *
 * @param[in] a Arena instance.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `a` is NULL.
 * @retval CLS_INVALID_ARG   If 'size' < 1.
 * @retval CLS_OUT_OF_MEMORY If os is out of memory.
 *
 * ### Example
 * @code
 * struct arena *a = NULL;
 * arena_create(&a, 1024);
 * arena_destroy(a);
 * @endcode
 */
void arena_destroy(struct arena *a);

/**
 * @brief Allocates memory from arena.
 *
 * Allocates `size` bytes with the provided `align` alignment.
 * Non-freeing. May instead clear the arena or restore a marker.
 *
 * @param[out] dest  Pointer to allocated memory.
 * @param[in]  a     Arena instance.
 * @param[in]  size  Number of bytes to allocate.
 * @param[in]  align Alignment (a power of two).
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `dest` or `a` is NULL.
 * @retval CLS_INVALID_ARG   If size is zero or align is not a power of two.
 * @retval CLS_OUT_OF_MEMORY If there is not enough space left.
 *
 * @warning Returned memory is only valid until the arena is cleared,
 *          destroyed, or restored to an earlier marker.
 *
 * ### Example
 * @code
 * void *values_ptr = NULL;
 * arena_alloc(&values_ptr, arena, 100 * sizeof(int), alignof(int));
 * int *values = values_ptr;
 * @endcode
 */
int arena_alloc(void **dest, struct arena *a, size_t size, size_t align);

/**
 * @brief Resets an arena to empty state.
 *
 * Sets the `used` counter to zero, making all previous allocations invalid.
 * Does not release underlying memory.
 *
 * @param[in] a Arena instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'a' is NULL.
 *
 * ### Example
 * @code
 * arena_clear(a); // All allocations are now invalid
 * @endcode
 */
int arena_clear(struct arena *a);

/**
 * @brief Saves the current allocation offset in the arena.
 *
 * Marker may later be passed to arena_marker_restore() in order to rewind the
 * arena to this point.
 *
 * @param[out] marker  Marker to store the current arena position.
 * @param[in]  a       Arena instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'a' or 'marker' is NULL.
 *
 * ### Example
 * @code
 * arena_marker m = 0;
 * arena_marker_save(&m, a);
 * @endcode
 */
int arena_marker_save(arena_marker *marker, struct arena *a);

/**
 * @brief Restores the arena to a previously saved marker.
 *
 * Invalidates all memory allocated after the marker.
 *
 * @param[in] a       Arena instance.
 * @param[in] marker  Marker returned by arena_marker_save().
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'a' or 'marker' is NULL.
 *
 * @note Restoring to a marker beyond arena is undefined behavior.
 *
 * ### Example
 * @code
 * arena_marker m = 0;
 * arena_marker_save(&m, a);
 *
 * // Allocate data
 *
 * arena_marker_restore(a, &m); // Revert to saved point
 * @endcode
 */
int arena_marker_restore(struct arena *a, arena_marker *marker);

#endif // CLS_ARENA_H
