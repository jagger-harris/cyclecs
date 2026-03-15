#ifndef CLS_TABLE_H
#define CLS_TABLE_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @struct table
 * @brief Hash table.
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
 * @param[out] t              Pointer to receive the created table.
 * @param[in]  start_capacity Initial slot count. Must be > 0.
 * @param[in]  key_size       Size of each key in bytes.
 * @param[in]  value_size     Size of each value in bytes.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `t` is NULL.
 * @retval CLS_INVALID_ARG   If any size parameter is < 1.
 * @retval CLS_OUT_OF_MEMORY If os is out of memory.
 *
 * ### Example
 * @code
 * struct table *t = NULL;
 * table_create(&t, 16, sizeof(int), sizeof(float));
 * // Use t
 * table_destroy(t);
 * @endcode
 */
int table_create(struct table **t, size_t start_capacity, size_t key_size,
                 size_t value_size);

/**
 * @brief Destroys a table and frees all memory.
 *
 * @param[in] in Table instance. NULL allowed.
 *
 * ### Example
 * @code
 * table_destroy(t);
 * @endcode
 */
void table_destroy(struct table *t);

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
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If 'in', 'key', or 'value' is NULL.
 * @retval CLS_OUT_OF_MEMORY If resizing fails.
 *
 * ### Example
 * @code
 * int k = 10;
 * float v = 3.14f;
 * table_insert(t, &k, &v);
 * @endcode
 */
int table_insert(struct table *t, const void *key, const void *value);

/**
 * @brief Removes a key from the table.
 *
 * If the key exists, removes it and optionally copies the associated value
 * into `out`. If key does not exist, this function succeeds but does nothing.
 *
 * Table may shrink if load factor becomes too small.
 *
 * @param[out] value Buffer to receive the removed value. NULL allowed.
 * @param[in]  t     Table instance.
 * @param[in]  key   Pointer to the key to remove.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If 't' or 'key' is NULL.
 * @retval CLS_OUT_OF_MEMORY If shrinking fails.
 *
 * ### Example
 * @code
 * float removed;
 * table_remove(&removed, t, &k);
 * @endcode
 */
int table_remove(void *value, struct table *t, const void *key);

/**
 * @brief Finds a value and returns a pointer to it.
 *
 * If the key exists, receives a direct pointer to the stored value. Caller may
 * modify the value in place.
 *
 * @param[out] value Pointer to receive the address of the value.
 * @param[in]  t     Table instance.
 * @param[in]  key   Pointer to key to search for.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If 'value', 't', or 'key' is NULL.
 * @retval CLS_INVALID_ARG If 'key' size < 1.
 *
 * ### Example
 * @code
 * void *v_ptr;
 * table_find(&v_ptr, t, &k);
 * float *v = vp_ptr;
 * @endcode
 */
int table_find(void **value, const struct table *t, const void *key);

/**
 * @brief Copies a value associated with a key into user provided memory.
 *
 * @param[out] value Value buffer.
 * @param[in]  t     Table instance.
 * @param[in]  key   Key to search for.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'value', 't', or 'key' is NULL.
 *
 * ### Example
 * @code
 * float value;
 * table_find_cpy(&value, t, &k);
 * @endcode
 */
int table_find_cpy(void *value, const struct table *t, const void *key);

/**
 * @brief Removes all entries from the table without freeing memory.
 *
 * Capacity remains unchanged. Length becomes zero.
 *
 * @param[in] t Table instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 't' is NULL.
 */
int table_clear(struct table *t);

/**
 * @brief Creates an iterator for traversing a table.
 *
 * The iterator starts at the first occupied slot. Must be destroyed using
 * table_iterator_destroy().
 *
 * @param[out] it Pointer to receive iterator.
 * @param[in]  t  Table instance.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If 'it' or 't' is NULL.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
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
int table_iterator_create(struct table_iterator **it, const struct table *t);

/**
 * @brief Destroys table iterator.
 *
 * @param[in] it Iterator instance. NULL allowed.
 */
void table_iterator_destroy(struct table_iterator *it);

/**
 * @brief Advances the iterator to the next occupied slot.
 *
 * @param[out] exists Set to true if a next element exists. False otherwise.
 * @param[in]  it     Iterator instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'exists' or 'it' is NULL.
 *
 * ### Example
 * @code
 * bool ok;
 * table_iterator_next(&ok, it);
 * @endcode
 */
int table_iterator_next(bool *exists, struct table_iterator *it);

/**
 * @brief Resets the iterator to the beginning of the table.
 *
 * @param[in] it Iterator instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'it' is NULL.
 */
int table_iterator_clear(struct table_iterator *it);

/**
 * @brief Retrieves the key for the current iterator position.
 *
 * @param[out] key Pointer to receive the key pointer.
 * @param[in]  it  Iterator instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'key' or 'it' is NULL.
 */
int table_iterator_key_get(void **key, const struct table_iterator *it);

/**
 * @brief Retrieves the value for the current iterator position.
 *
 * @param[out] value Pointer to receive the value pointer.
 * @param[in]  it    Iterator instance.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If 'value' or 'it' is NULL.
 */
int table_iterator_value_get(void **value, const struct table_iterator *it);

#endif // CLS_TABLE_H
