#ifndef UTIL_TABLE_H
#define UTIL_TABLE_H

#include <stdbool.h>
#include <stddef.h>

struct table {
    size_t capacity;
    size_t length;
    size_t key_size;
    size_t value_size;
    size_t slot_size;
    void *slots;
};

int table_init(struct table *out, size_t start_capacity, size_t key_size,
               size_t value_size);
void table_destroy(struct table *in);
int table_insert(struct table *in, const void *key, const void *value);
int table_remove(void *out, struct table *in, const void *key);
int table_find(void **out, const struct table *in, const void *key);
int table_find_cpy(void *out, const struct table *in, const void *key);

struct table_iterator {
    const struct table *table;
    size_t current_index;
    void *key;
    void *value;
};

int table_iterator_init(struct table_iterator *iter, const struct table *table);
bool table_iterator_next(struct table_iterator *iter);
int table_iterator_reset(struct table_iterator *iter);

#endif // UTIL_TABLE_H
