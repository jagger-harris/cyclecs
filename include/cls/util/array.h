#ifndef CLS_ARRAY_H
#define CLS_ARRAY_H

#include <cls/util/error.h>
#include <stddef.h>

/**
 * @struct cls_array
 * @brief Dynamic array.
 */
struct cls_array;

/**
 * @brief Creates an array.
 *
 * Creates a dynamic array with space for `start_capacity` elements.
 * Destroy the returned array with cls_array_destroy().
 *
 * @param[out] a              Array.
 * @param[in]  start_capacity Initial capacity.
 * @param[in]  elem_size      Element size in bytes.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `a` is NULL.
 * @retval CLS_INVALID_ARG   If `start_capacity` or `elem_size` is less than 1.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 *
 * @code
 * struct cls_array *arr;
 * cls_array_create(&arr, 16, sizeof(int));
 * // Use arr.
 * cls_array_destroy(arr);
 * @endcode
 */
cls_error cls_array_create(struct cls_array **a, size_t start_capacity,
                           size_t elem_size);

/**
 * @brief Destroys an array.
 *
 * Releases the array memory.
 *
 * @param[in] a Array to destroy.
 */
void cls_array_destroy(struct cls_array *a);

/**
 * @brief Retrieves the array length.
 *
 * @param[out] len Array length.
 * @param[in]  a   Array.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `len` or `a` is NULL.
 */
cls_error cls_array_length_get(size_t *len, struct cls_array *a);

/**
 * @brief Retrieves the array data.
 *
 * @param[out] data Array data.
 * @param[in]  a    Array.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `data` or `a` is NULL.
 */
cls_error cls_array_data_get(void **data, struct cls_array *a);

/**
 * @brief Clears an array.
 *
 * Sets the array length to zero without releasing memory.
 *
 * @param[in] a Array.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `a` is NULL.
 */
cls_error cls_array_clear(struct cls_array *a);

/**
 * @brief Retrieves an element.
 *
 * @param[out] dest  Element address.
 * @param[in]  a     Array.
 * @param[in]  index Element index.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `dest` or `a` is NULL.
 * @retval CLS_INVALID_ARG If `index` is out of bounds.
 */
cls_error cls_array_elem_get(void **dest, const struct cls_array *a,
                             size_t index);

/**
 * @brief Copies an element.
 *
 * Copies the element at `index` into `dest`.
 *
 * @param[out] dest  Destination buffer.
 * @param[in]  a     Array.
 * @param[in]  index Element index.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `dest` or `a` is NULL.
 * @retval CLS_INVALID_ARG If `index` is out of bounds.
 */
cls_error cls_array_elem_get_cpy(void *dest, const struct cls_array *a,
                                 size_t index);

/**
 * @brief Sets an element.
 *
 * Copies `elem` into the element at `index`.
 *
 * @param[in] a     Array.
 * @param[in] index Element index.
 * @param[in] elem  Element data.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `a` or `elem` is NULL.
 * @retval CLS_INVALID_ARG If `index` is out of bounds.
 */
cls_error cls_array_elem_set(struct cls_array *a, size_t index,
                             const void *elem);

/**
 * @brief Appends an element.
 *
 * Grows the array if required.
 *
 * @param[in,out] a    Array.
 * @param[in]     elem Element data.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `a` or `elem` is NULL.
 * @retval CLS_OUT_OF_MEMORY If resizing fails.
 *
 * @note The array pointer may change if the array grows.
 *
 * @code
 * int v = 42;
 * cls_array_push(&arr, &v);
 * @endcode
 */
cls_error cls_array_push(struct cls_array **a, const void *elem);

/**
 * @brief Removes the last element.
 *
 * @param[out] last Last element value.
 * @param[in]  a    Array.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `a` is NULL.
 * @retval CLS_INVALID_ARG If the array is empty.
 */
cls_error cls_array_pop(void *last, struct cls_array *a);

/**
 * @brief Inserts an element.
 *
 * Inserts an element at `index` and shifts following elements.
 *
 * @param[in,out] a     Array.
 * @param[in]     index Element index.
 * @param[in]     elem  Element data.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `a` or `elem` is NULL.
 * @retval CLS_INVALID_ARG If `index` is invalid.
 */
cls_error cls_array_insert(struct cls_array **a, size_t index,
                           const void *elem);

/**
 * @brief Removes an element.
 *
 * Shifts following elements after removing the element at `index`.
 *
 * @param[in] a     Array.
 * @param[in] index Element index.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `a` is NULL.
 * @retval CLS_INVALID_ARG If `index` is out of bounds.
 */
cls_error cls_array_remove(struct cls_array *a, size_t index);

/**
 * @brief Concatenates arrays.
 *
 * Appends all elements from `src` to `dest`.
 *
 * @param[in,out] dest Destination array.
 * @param[in]     src  Source array.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `dest` or `src` is NULL.
 * @retval CLS_INVALID_ARG   If the element sizes differ.
 * @retval CLS_OUT_OF_MEMORY If resizing fails.
 */
cls_error cls_array_concat(struct cls_array **dest,
                           const struct cls_array *src);

#endif // CLS_ARRAY_H
