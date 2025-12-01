#ifndef UTIL_TABLE_H
#define UTIL_TABLE_H

#include <stdbool.h>
#include <stddef.h>

struct table;
struct table_iterator;

int table_create(struct table **out, size_t start_capacity, size_t key_size,
                 size_t value_size);
void table_destroy(struct table *in);
int table_insert(struct table *in, const void *key, const void *value);
int table_remove(void *out, struct table *in, const void *key);
int table_find(void **out, const struct table *in, const void *key);
int table_find_cpy(void *out, const struct table *in, const void *key);
int table_clear(struct table *in);
int table_iterator_create(struct table_iterator **out,
                          const struct table *table);
void table_iterator_destroy(struct table_iterator *in);
int table_iterator_next(bool *out, struct table_iterator *in);
int table_iterator_clear(struct table_iterator *in);
int table_iterator_key_get(void **out, struct table_iterator *in);
int table_iterator_value_get(void **out, struct table_iterator *in);

#endif // UTIL_TABLE_H
