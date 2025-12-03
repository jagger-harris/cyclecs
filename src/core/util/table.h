#ifndef UTIL_TABLE_H
#define UTIL_TABLE_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @struct table
 * @brief Opaque hash table.
 */
struct table;

/**
 * @struct table_iterator
 * @brief Iterator for traversing a table.
 */
struct table_iterator;

/**
 * @brief Creates a new hash table.
 *
 * Allocates a new table with capacity `start_capacity`, where each slot holds
 * a key of size `key_size` and a value of size `value_size`. Table must be
 * destroyed using table_destroy().
 *
 * @param[out] out            Pointer to receive the created table.
 * @param[in]  start_capacity Initial slot count. Must be > 0.
 * @param[in]  key_size       Size of each key in bytes.
 * @param[in]  value_size     Size of each value in bytes.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR       If `out` is NULL.
 * @retval CORE_INVALID_ARG   If any size parameter is zero.
 * @retval CORE_OUT_OF_MEMORY If allocation fails.
 *
 * ### Example
 * @code
 * struct table *t;
 * table_create(&t, 16, sizeof(int), sizeof(float));
 * // Use t
 * table_destroy(t);
 * @endcode
 */
int table_create(struct table **out, size_t start_capacity, size_t key_size,
                 size_t value_size);

/**
 * @brief Destroys a table and frees all memory.
 *
 * @param[in] in Table instance.
 * Safe to pass NULL. (no-op)
 *
 * ### Example
 * @code
 * table_destroy(t);
 * @endcode
 */
void table_destroy(struct table *in);

/**
 * @brief Inserts a key/value pair into the table.
 *
 * Copies both key and value into the table. If load factor exceeds the
 * configured maximum, the table automatically grows.
 *
 * @param[in] in    Table instance.
 * @param[in] key   Pointer to key data of size `key_size`.
 * @param[in] value Pointer to value data of size `value_size`.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR      If in, key, or value is NULL.
 * @retval CORE_OUT_OF_MEMORY If resizing fails or table is full.
 *
 * ### Example
 * @code
 * int k = 10;
 * float v = 3.14f;
 * table_insert(t, &k, &v);
 * @endcode
 */
int table_insert(struct table *in, const void *key, const void *value);

/**
 * @brief Removes a key from the table.
 *
 * If the key exists, removes it and optionally copies the associated value
 * into `out`. If key does not exist, this function succeeds but does nothing.
 *
 * Table may shrink if load factor becomes too small.
 *
 * @param[out] out Optional buffer to receive the removed value.
 *                 Must be at least `value_size` bytes.
 * @param[in]  in  Table instance.
 * @param[in]  key Pointer to the key to remove.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If in or key is NULL.
 * @retval CORE_OUT_OF_MEMORY If shrinking fails.
 *
 * ### Example
 * @code
 * float removed;
 * table_remove(&removed, t, &k);
 * @endcode
 */
int table_remove(void *out, struct table *in, const void *key);

/**
 * @brief Finds a value and returns a mutable pointer to it.
 *
 * If the key exists, `*out` receives a direct pointer to the stored value.
 * Caller may modify the value in place.
 *
 * @param[out] out Pointer to receive the address of the value, or NULL if
 * missing.
 * @param[in]  in  Table instance.
 * @param[in]  key Pointer to key to search for.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR    If out, in, or key is NULL.
 * @retval CORE_INVALID_ARG If key_size is zero.
 *
 * ### Example
 * @code
 * float *vp;
 * table_find_mut((void**)&vp, t, &k);
 * if (vp) *vp = 9.0f;
 * @endcode
 */
int table_find_mut(void **out, const struct table *in, const void *key);

/**
 * @brief Finds a value and returns a const pointer to it.
 *
 * Behaves like table_find_mut() but exposes a const pointer.
 *
 * @param[out] out Pointer to receive const value pointer, or NULL.
 * @param[in]  in  Table instance.
 * @param[in]  key Pointer to key data.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If out or in is NULL.
 *
 * ### Example
 * @code
 * const float *vp;
 * table_find(&vp, t, &k);
 * if (vp) printf("value = %f\n", *vp);
 * @endcode
 */
int table_find(const void **out, const struct table *in, const void *key);

/**
 * @brief Copies a value associated with a key into user-provided memory.
 *
 * If key is present, copies `value_size` bytes into `out`.
 *
 * @param[out] out Destination buffer.
 * @param[in]  in  Table instance.
 * @param[in]  key Key to search for.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If out or in is NULL.
 *
 * ### Example
 * @code
 * float value;
 * table_find_cpy(&value, t, &k);
 * @endcode
 */
int table_find_cpy(void *out, const struct table *in, const void *key);

/**
 * @brief Removes all entries from the table without freeing memory.
 *
 * Capacity remains unchanged. Length becomes zero.
 *
 * @param[in] in Table instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If in is NULL.
 */
int table_clear(struct table *in);

/**
 * @brief Creates an iterator for traversing a table.
 *
 * The iterator starts at the first occupied slot. Must be destroyed using
 * table_iterator_destroy().
 *
 * @param[out] out   Pointer to receive iterator.
 * @param[in]  table Table instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR      If out or table is NULL.
 * @retval CORE_OUT_OF_MEMORY If allocation fails.
 *
 * ### Example
 * @code
 * struct table_iterator *it;
 * table_iterator_create(&it, t);
 * bool ok;
 * while (table_iterator_next(&ok, it), ok) {
 *     void *k;
 *     void *v;
 *     table_iterator_key_get(&k, it);
 *     table_iterator_value_get(&v, it);
 *     // Use k and v
 * }
 * table_iterator_destroy(it);
 * @endcode
 */
int table_iterator_create(struct table_iterator **out,
                          const struct table *table);

/**
 * @brief Destroys table iterator.
 *
 * @param[in] in Iterator instance.
 */
void table_iterator_destroy(struct table_iterator *in);

/**
 * @brief Advances the iterator to the next occupied slot.
 *
 * @param[out] out Set to true if a next element exists. False otherwise.
 * @param[in]  in  Iterator instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If out or in is NULL.
 *
 * ### Example
 * @code
 * bool ok;
 * table_iterator_next(&ok, it);
 * @endcode
 */
int table_iterator_next(bool *out, struct table_iterator *in);

/**
 * @brief Resets the iterator to the beginning of the table.
 *
 * @param[in] in Iterator instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If in is NULL.
 */
int table_iterator_clear(struct table_iterator *in);

/**
 * @brief Retrieves the mutable key for the current iterator position.
 *
 * @param[out] out Pointer to receive the key pointer.
 * @param[in]  in  Iterator instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If out or in is NULL.
 */
int table_iterator_key_get_mut(void **out, const struct table_iterator *in);

/**
 * @brief Retrieves the mutable value for the current iterator position.
 *
 * @param[out] out Pointer to receive the value pointer.
 * @param[in]  in  Iterator instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If out or in is NULL.
 */
int table_iterator_value_get_mut(void **out, const struct table_iterator *in);

/**
 * @brief Retrieves the key for the current iterator position.
 *
 * @param[out] out Pointer to receive the key pointer.
 * @param[in]  in  Iterator instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If out or in is NULL.
 */
int table_iterator_key_get(const void **out, const struct table_iterator *in);

/**
 * @brief Retrieves the value for the current iterator position.
 *
 * @param[out] out Pointer to receive the value pointer.
 * @param[in]  in  Iterator instance.
 *
 * @return CORE_SUCCESS on success.
 * @retval CORE_NULLPTR If out or in is NULL.
 */
int table_iterator_value_get(const void **out, const struct table_iterator *in);

#endif // UTIL_TABLE_H
