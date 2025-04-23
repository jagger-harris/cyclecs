#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include "core/util/err.h"
#include <stddef.h>

typedef struct array array;

err array_new(array **out, size_t start_capacity, size_t elem_size);
err array_delete(array *in);
err array_length(size_t *out, array *in);
err array_at(void **out, array *in, size_t index);
err array_at_cpy(void *out, array *in, size_t index);
err array_push(array *in, void *data);
err array_pop(array *in);
err array_insert(array *in, size_t index, void *data);
err array_remove(array *in, size_t index);

#endif /* UTIL_ARRAY_H */
