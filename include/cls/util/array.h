#ifndef CLS_ARRAY_H
#define CLS_ARRAY_H

#include <stddef.h>

/**
 * @struct array
 * @brief Dynamic array.
 */
struct cls_array;

/**
 * @brief Creates a new dynamic array.
 *
 * Allocates a new array with enough space for `start_capacity` elements,
 * each of size `elem_size`. Returned array must be destroyed with
 * cls_array_destroy().
 *
 * @param[out] a              Array instance.
 * @param[in]  start_capacity Initial capacity of the array.
 * @param[in]  elem_size      Size of each element in bytes.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `a` is NULL.
 * @retval CLS_INVALID_ARG   If start_capacity or elem_size are < 1.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 *
 * ### Example
 * @code
 * struct cls_array *arr;
 * cls_array_create(&arr, 16, sizeof(int));
 * // Use arr
 * array_destroy(arr);
 * @endcode
 */
int cls_array_create(struct cls_array **a, size_t start_capacity,
                     size_t elem_size);

/**
 * @brief Destroys an array and frees its memory.
 *
 * @param[in] a Array instance. NULL valid.
 *
 * ### Example
 * @code
 * cls_array_destroy(arr);
 * @endcode
 */
void cls_array_destroy(struct cls_array *a);

/**
 * @brief Gets the number of elements in the array.
 *
 * @param[out] len Pointer to receive the length.
 * @param[in]  a   Array instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `len` or `a` is NULL.
 *
 * ### Example
 * @code
 * size_t len;
 * cls_array_length_get(&len, arr);
 * @endcode
 */
int cls_array_length_get(size_t *len, struct cls_array *a);

/**
 * @brief Gets the raw data in the array.
 *
 * @param[out] data Pointer to receive the raw data.
 * @param[in]  a    Array instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `data` or `a` is NULL.
 */
int cls_array_data_get(void **data, struct cls_array *a);

/**
 * @brief Clears the array without freeing memory.
 *
 * Sets the length to zero. Existing data remains but considered invalid.
 *
 * @param[in] a Array instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'a' is NULL.
 */
int cls_array_clear(struct cls_array *a);

/**
 * @brief Gets a const pointer to an element in the array.
 *
 * @param[out] dest  Pointer to receive the element address.
 * @param[in]  a     Array instance.
 * @param[in]  index Index of the element.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If 'dest' or 'a' is NULL.
 * @retval CLS_INVALID_ARG If 'index' is out of bounds.
 */
int cls_array_elem_get(void **dest, const struct cls_array *a, size_t index);

/**
 * @brief Copies an element from the array into user-provided memory.
 *
 * @param[out] dest  Destination buffer of size at least elem_size.
 * @param[in]  a     Array instance.
 * @param[in]  index Index of the element to copy.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If 'dest' or 'a' is NULL.
 * @retval CLS_INVALID_ARG If 'index' is out of bounds.
 */
int cls_array_elem_get_cpy(void *dest, const struct cls_array *a, size_t index);

/**
 * @brief Sets the value of an element.
 *
 * Copies elem_size bytes from `data` into the element at `index`.
 *
 * @param[in] a     Array instance.
 * @param[in] index Index to set.
 * @param[in] elem  Pointer to the element data to copy in.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If 'a' or 'elem' is NULL.
 * @retval CLS_INVALID_ARG If 'index' is out of bounds.
 */
int cls_array_elem_set(struct cls_array *a, size_t index, const void *elem);

/**
 * @brief Appends an element to the array, growing if needed.
 *
 * @param[in,out] a    Pointer to array instance.
 * @param[in]     elem Pointer to the element to append.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If 'in' or 'elem' is NULL.
 * @retval CLS_OUT_OF_MEMORY If resizing fails.
 *
 * @note If growth occurs, the array pointer will change.
 *
 * ### Example
 * @code
 * int v = 42;
 * cls_array_push(&arr, &v);
 * @endcode
 */
int cls_array_push(struct cls_array **a, const void *elem);

/**
 * @brief Removes last element from array.
 *
 * @param[out] last Pointer to store popped value. NULL allowed.
 * @param[in]  a    Array instance.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If 'a' is NULL.
 * @retval CLS_INVALID_ARG If 'a' is empty.
 */
int cls_array_pop(void *last, struct cls_array *a);

/**
 * @brief Inserts an element at given index.
 *
 * Shifts following elements to the right. Grows the array if needed.
 *
 * @param[in] a     Pointer to array pointer.
 * @param[in] index Insert position.
 * @param[in] elem  Element to insert.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If 'a' or 'elem' is NULL.
 * @retval CLS_INVALID_ARG If 'index' is invalid.
 */
int cls_array_insert(struct cls_array **a, size_t index, const void *elem);

/**
 * @brief Removes the element at the given index.
 *
 * Following elements are shifted left.
 *
 * @param[in] a     Array instance.
 * @param[in] index Index of element to remove.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If 'a' is NULL.
 * @retval CLS_INVALID_ARG If 'index' is out of bounds.
 */
int cls_array_remove(struct cls_array *a, size_t index);

/**
 * @brief Appends all elements of array `src` into array `dest`.
 *
 * Ensures sufficient capacity and performs a contiguous copy.
 *
 * @param[in,out] dest Pointer to array instance.
 * @param[in]     src  Source array. Must have same elem_size.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If 'dest' or 'src' is NULL.
 * @retval CLS_INVALID_ARG   If array element size differs.
 * @retval CLS_OUT_OF_MEMORY If growth fails.
 */
int cls_array_concat(struct cls_array **dest, const struct cls_array *src);

#endif // CLS_ARRAY_H
