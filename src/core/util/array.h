#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include <stddef.h>

/**
 * @struct array
 * @brief Opaque dynamic array.
 */
struct array;

/**
 * @brief Creates a new dynamic array.
 *
 * Allocates a new array with enough space for `start_capacity` elements,
 * each of size `elem_size`. Returned array must be destroyed with
 * array_destroy().
 *
 * @param[out] out            Arena instance.
 * @param[in]  start_capacity Initial capacity of the array.
 * @param[in]  elem_size      Size of each element in bytes.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR      If `out` is NULL.
 * @retval CORE_INVALID_ARG  If start_capacity or elem_size are zero.
 * @retval CORE_OUT_OF_MEMORY If allocation fails.
 *
 * ### Example
 * @code
 * struct array *arr;
 * array_create(&arr, 16, sizeof(int));
 * // Use arr
 * array_destroy(arr);
 * @endcode
 */
int array_create(struct array **out, size_t start_capacity, size_t elem_size);

/**
 * @brief Destroys an array and frees its memory.
 *
 * @param[in] in Array instance.
 * Safe to pass NULL. (no-op)
 *
 * ### Example
 * @code
 * array_destroy(arr);
 * @endcode
 */
void array_destroy(struct array *in);

/**
 * @brief Gets the number of elements in the array.
 *
 * @param[out] out Pointer to receive the length.
 * @param[in]  in  Array instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If `out` or `in` is NULL.
 *
 * ### Example
 * @code
 * size_t len;
 * array_length_get(&len, arr);
 * @endcode
 */
int array_length_get(size_t *out, struct array *in);

/**
 * @brief Clears the array without freeing memory.
 *
 * Sets the length to zero. Existing data remains but considered invalid.
 *
 * @param[in] in Array instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If in is NULL.
 */
int array_clear(struct array *in);

/**
 * @brief Gets a mutable pointer to an element in the array.
 *
 * @param[out] out   Pointer to receive the element address.
 * @param[in]  in    Array instance.
 * @param[in]  index Index of the element.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If out or in is NULL.
 * @retval CORE_INVALID_ARG If index is out of bounds.
 */
int array_elem_get_mut(void **out, const struct array *in, size_t index);

/**
 * @brief Gets a const pointer to an element in the array.
 *
 * @param[out] out   Pointer to receive the element address.
 * @param[in]  in    Array instance.
 * @param[in]  index Index of the element.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If out or in is NULL.
 * @retval CORE_INVALID_ARG If index is out of bounds.
 */
int array_elem_get(const void **out, const struct array *in, size_t index);

/**
 * @brief Copies an element from the array into user-provided memory.
 *
 * @param[out] out   Destination buffer of size at least elem_size.
 * @param[in]  in    Array instance.
 * @param[in]  index Index of the element to copy.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If out or in is NULL.
 * @retval CORE_INVALID_ARG If index is out of range.
 */
int array_elem_get_cpy(void *out, const struct array *in, size_t index);

/**
 * @brief Sets the value of an element.
 *
 * Copies elem_size bytes from `data` into the element at `index`.
 *
 * @param[in] in    Array instance.
 * @param[in] index Index to set.
 * @param[in] data  Pointer to the element data to copy in.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If in or data is NULL.
 * @retval CORE_INVALID_ARG If index is out of bounds.
 */
int array_elem_set(struct array *in, size_t index, const void *data);

/**
 * @brief Appends an element to the array, growing if needed.
 *
 * @param[in,out] in    Pointer to array instance.
 * @param[in]     data  Pointer to the element to append.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR      If in, *in, or data is NULL.
 * @retval CORE_OUT_OF_MEMORY If resizing fails.
 *
 * @note If growth occurs, the array pointer will change.
 *
 * ### Example
 * @code
 * int v = 42;
 * array_push(&arr, &v);
 * @endcode
 */
int array_push(struct array **in, const void *data);

/**
 * @brief Removes last element from array.
 *
 * @param[in] in Array instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If in is NULL.
 * @retval CORE_INVALID_ARG If array is empty.
 */
int array_pop(struct array *in);

/**
 * @brief Inserts an element at given index.
 *
 * Shifts following elements to the right. Grows the array if needed.
 *
 * @param[in] in  Pointer to array pointer.
 * @param[in]     index Insert position.
 * @param[in]     data  Element to insert.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If in, *in, or data is NULL.
 * @retval CORE_INVALID_ARG If index is invalid.
 */
int array_insert(struct array **in, size_t index, const void *data);

/**
 * @brief Removes the element at the given index.
 *
 * Following elements are shifted left.
 *
 * @param[in] in Array instance.
 * @param[in] index Index of element to remove.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If in is NULL.
 * @retval CORE_INVALID_ARG If index is invalid.
 */
int array_remove(struct array *in, size_t index);

/**
 * @brief Appends all elements of array `b` into array `a`.
 *
 * Ensures sufficient capacity and performs a contiguous copy.
 *
 * @param[in] in Pointer to array instance.
 * @param[in] b  Source array. Must have same elem_size.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR     If in, *in, or b is NULL.
 * @retval CORE_INVALID_ARG If elem_size differs.
 * @retval CORE_OUT_OF_MEMORY If growth fails.
 */
int array_concat(struct array **a, const struct array *b);

#endif // UTIL_ARRAY_H
