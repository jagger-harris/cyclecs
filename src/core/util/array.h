#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include "core/util/err.h"
#include <stddef.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct array {
    void *data;
    size_t capacity;
    size_t length;
    size_t elem_size;
};

err array_init(struct array *out, size_t start_capacity, size_t elem_size);
void array_destroy(struct array *in);
err array_clear(struct array *in);
err array_deep_cpy(struct array *out, const struct array *in);
err array_get_ptr(void **out, const struct array *in, size_t index);
err array_get_cpy(void *out, const struct array *in, size_t index);
err array_push(struct array *in, void *data);
err array_pop(struct array *in);
err array_push_multiple(struct array *in, void *data, size_t count);
err array_insert(struct array *in, size_t index, void *data);
err array_remove(struct array *in, size_t index);

#endif // UTIL_ARRAY_H
