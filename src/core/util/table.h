#ifndef UTIL_TABLE_H
#define UTIL_TABLE_H

#include "core/util/err.h"
#include <stddef.h>
#include <stdint.h>

typedef struct table table;

err table_new(table **out, size_t start_capacity, size_t key_size,
              size_t value_size);
err table_delete(table *in);
err table_insert(table *in, const void *key, const void *value);
err table_remove(void *out, table *in, const void *key);
err table_find(void *out, table *in, const void *key);

#endif /* UTIL_TABLE_H */
