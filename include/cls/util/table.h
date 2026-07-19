/**
 * @file cls/util/table.h
 * @brief Hash table for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/table.c
 */

#ifndef CLS_TABLE_H
#define CLS_TABLE_H

#include <cls/util/error.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @defgroup table Table
 * @ingroup util
 * @brief Generic and dynamic hash table implementation.
 * @{
 */

/**
 * @struct cls_table
 * @brief Hash table.
 */
struct cls_table;

/**
 * @struct cls_table_iterator
 * @brief Hash table iterator.
 */
struct cls_table_iterator;

/**
 * @brief Creates a table.
 *
 * Creates a hash table with the given key and value sizes. Destroy the
 * returned table with cls_table_destroy().
 *
 * @param[out] t              Table.
 * @param[in]  start_capacity Initial capacity.
 * @param[in]  key_size       Key size in bytes.
 * @param[in]  value_size     Value size in bytes.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `t` is NULL.
 * @retval CLS_INVALID_ARG   If a size parameter is less than 1.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 *
 * @code
 * struct cls_table *t;
 * cls_table_create(&t, 16, sizeof(int), sizeof(float));
 * // Use t.
 * cls_table_destroy(t);
 * @endcode
 */
cls_error cls_table_create(struct cls_table **t, size_t start_capacity,
                           size_t key_size, size_t value_size);

/**
 * @brief Destroys a table.
 *
 * Releases the table memory.
 *
 * @param[in] t Table to destroy.
 */
void cls_table_destroy(struct cls_table *t);

/**
 * @brief Inserts a key and value.
 *
 * Copies the key and value into the table. Grows the table if required.
 *
 * @param[in] t     Table.
 * @param[in] key   Key data.
 * @param[in] value Value data.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `t`, `key`, or `value` is NULL.
 * @retval CLS_OUT_OF_MEMORY If resizing fails.
 *
 * @code
 * int key = 10;
 * float value = 3.14f;
 * cls_table_insert(t, &key, &value);
 * @endcode
 */
cls_error cls_table_insert(struct cls_table *t, const void *key,
                           const void *value);

/**
 * @brief Removes a key.
 *
 * Removes the key and optionally copies its value.
 *
 * @param[out] value Removed value.
 * @param[in]  t     Table.
 * @param[in]  key   Key to remove.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `t` or `key` is NULL.
 * @retval CLS_OUT_OF_MEMORY If resizing fails.
 *
 * @code
 * float value;
 * cls_table_remove(&value, t, &key);
 * @endcode
 */
cls_error cls_table_remove(void *value, struct cls_table *t, const void *key);

/**
 * @brief Finds a value.
 *
 * Returns a pointer to the stored value.
 *
 * @param[out] value Value pointer.
 * @param[in]  t     Table.
 * @param[in]  key   Key to find.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `value`, `t`, or `key` is NULL.
 */
cls_error cls_table_find(void **value, const struct cls_table *t,
                         const void *key);

/**
 * @brief Copies a value.
 *
 * Copies the value associated with a key into `value`.
 *
 * @param[out] value Value buffer.
 * @param[in]  t     Table.
 * @param[in]  key   Key to find.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `value`, `t`, or `key` is NULL.
 */
cls_error cls_table_find_cpy(void *value, const struct cls_table *t,
                             const void *key);

/**
 * @brief Clears a table.
 *
 * Removes all entries without releasing memory.
 *
 * @param[in] t Table.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `t` is NULL.
 */
cls_error cls_table_clear(struct cls_table *t);

/**
 * @brief Creates a table iterator.
 *
 * Creates an iterator for traversing the table. Destroy the iterator with
 * cls_table_iterator_destroy().
 *
 * @param[out] it Iterator.
 * @param[in]  t  Table.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `it` or `t` is NULL.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 *
 * @code
 * struct cls_table_iterator *it;
 * cls_table_iterator_create(&it, t);
 * // Use it.
 * cls_table_iterator_destroy(it);
 * @endcode
 */
cls_error cls_table_iterator_create(struct cls_table_iterator **it,
                                    const struct cls_table *t);

/**
 * @brief Destroys a table iterator.
 *
 * @param[in] it Iterator to destroy.
 */
void cls_table_iterator_destroy(struct cls_table_iterator *it);

/**
 * @brief Advances an iterator.
 *
 * Moves the iterator to the next table entry.
 *
 * @param[out] exists Whether an entry exists.
 * @param[in]  it     Iterator.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `exists` or `it` is NULL.
 */
cls_error cls_table_iterator_next(bool *exists, struct cls_table_iterator *it);

/**
 * @brief Clears an iterator.
 *
 * Resets the iterator to the start of the table.
 *
 * @param[in] it Iterator.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `it` is NULL.
 */
cls_error cls_table_iterator_clear(struct cls_table_iterator *it);

/**
 * @brief Retrieves the iterator key.
 *
 * @param[out] key Key pointer.
 * @param[in]  it  Iterator.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `key` or `it` is NULL.
 */
cls_error cls_table_iterator_key_get(void **key,
                                     const struct cls_table_iterator *it);

/**
 * @brief Retrieves the iterator value.
 *
 * @param[out] value Value pointer.
 * @param[in]  it    Iterator.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `value` or `it` is NULL.
 */
cls_error cls_table_iterator_value_get(void **value,
                                       const struct cls_table_iterator *it);

/** @} */

#endif // CLS_TABLE_H
