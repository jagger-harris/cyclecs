#ifndef UTIL_ARENA_H
#define UTIL_ARENA_H

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
 * @brief Opaque linear memory allocator.
 */
struct arena;

/**
 * @brief Creates a new arena allocator.
 *
 * Allocates a contiguous block of memory containing both the arena metadata and
 * the aligned buffer region. Caller must free the arena using arena_destroy().
 *
 * @param[out] out  Arena instance.
 * @param[in]  size Size of the arena's usable buffer in bytes.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If `out` is NULL.
 * @retval CORE_INVALID_ARG If size is zero.
 * @retval CORE_OUT_OF_MEMORY If allocation fails.
 *
 * ### Example
 * @code
 * struct arena *a;
 * if (arena_create(&a, 1024) == CORE_SUCCESS) {
 *     // use the arena
 *     arena_destroy(a);
 * }
 * @endcode
 */
int arena_create(struct arena **out, size_t size);

/**
 * @brief Destroys an arena created with arena_create().
 *
 * Frees the memory associated with the arena.
 * Safe to pass NULL (no-op).
 *
 * @param[in] in Arena instance.
 *
 * ### Example
 * @code
 * struct arena *a;
 * arena_create(&a, 1024);
 * arena_destroy(a);
 * @endcode
 */
void arena_destroy(struct arena *in);

/**
 * @brief Allocates memory from arena.
 *
 * Allocates `size` bytes with the provided `align` alignment.
 * Non-freeing. May instead clear the arena or restore a marker.
 *
 * @param[out] out   Pointer to allocated memory.
 * @param[in]  in    Arena instance.
 * @param[in]  size  Number of bytes to allocate.
 * @param[in]  align Alignment (a power of two).
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR      If `out` or `in` is NULL.
 * @retval CORE_INVALID_ARG  If size is zero or align is not a power of two.
 * @retval CORE_OUT_OF_MEMORY If there is not enough space left.
 *
 * @warning Returned memory is only valid until the arena is cleared,
 *          destroyed, or restored to an earlier marker.
 *
 * ### Example
 * @code
 * int *values;
 * arena_alloc((void**)&values, arena, 100 * sizeof(int), alignof(int));
 * @endcode
 */
int arena_alloc(void **out, struct arena *in, size_t size, size_t align);

/**
 * @brief Resets an arena to empty state.
 *
 * Sets the `used` counter to zero, making all previous allocations invalid.
 * Does not release underlying memory.
 *
 * @param[in] in Arena instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If in is NULL.
 *
 * ### Example
 * @code
 * arena_clear(a); // All allocations are now invalid
 * @endcode
 */
int arena_clear(struct arena *in);

/**
 * @brief Saves the current allocation offset in the arena.
 *
 * Marker may later be passed to arena_marker_restore() in order to rewind the
 * arena to this point.
 *
 * @param[out] out Marker to store the current arena position.
 * @param[in] in   Arena instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If out or in is NULL.
 *
 * ### Example
 * @code
 * arena_marker m;
 * arena_marker_save(&m, a);
 * @endcode
 */
int arena_marker_save(arena_marker *out, struct arena *in);

/**
 * @brief Restores the arena to a previously saved marker.
 *
 * Invalidates all memory allocated after the marker.
 *
 * @param[in] in      Arena instance.
 * @param[in] marker  Marker returned by arena_marker_save().
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If in or marker is NULL.
 *
 * @note Restoring to a marker beyond `used` is undefined behavior.
 *
 * ### Example
 * @code
 * arena_marker m;
 * arena_marker_save(&m, a);
 *
 * // Allocate data
 *
 * arena_marker_restore(a, &m); // Revert to saved point
 * @endcode
 */
int arena_marker_restore(struct arena *in, arena_marker *marker);

#endif // UTIL_ARENA_H
