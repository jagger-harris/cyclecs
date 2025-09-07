#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include <stddef.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct array {
    void *data;
    size_t capacity;
    size_t length;
    size_t elem_size;
};

int array_init(struct array *out, size_t start_capacity, size_t elem_size);
void array_destroy(struct array *in);
int array_clear(struct array *in);
int array_get(void **out, const struct array *in, size_t index);
int array_get_cpy(void *out, const struct array *in, size_t index);
int array_set(struct array *in, size_t index, void *data);
int array_push(struct array *in, void *data);
int array_pop(struct array *in);
int array_insert(struct array *in, size_t index, void *data);
int array_remove(struct array *in, size_t index);

#endif // UTIL_ARRAY_H
