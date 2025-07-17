#ifndef UTIL_TABLE_H
#define UTIL_TABLE_H

#include "core/util/err.h"
#include <stddef.h>
#include <stdint.h>

struct table {
    uint8_t *data;
    uint8_t *metadata;
    size_t capacity;
    size_t length;
    size_t key_size;
    size_t value_size;
};

err table_init(struct table *out, size_t start_capacity, size_t key_size,
               size_t value_size);
void table_destroy(struct table *in);
err table_insert(struct table *in, const void *key, const void *value);
err table_remove(void *out, struct table *in, const void *key);
err table_find(void *out, const struct table *in, const void *key);
err table_find_ptr(void **out, const struct table *in, const void *key);

#endif // UTIL_TABLE_H
