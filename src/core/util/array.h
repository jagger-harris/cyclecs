#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include <stddef.h>

struct array;

int array_create(struct array **out, size_t start_capacity, size_t elem_size);
void array_destroy(struct array *in);
int array_length_get(size_t *out, struct array *in);
int array_clear(struct array *in);
int array_elem_get_mut(void **out, struct array *in, size_t index);
int array_elem_get(const void **out, const struct array *in, size_t index);
int array_elem_get_cpy(void *out, const struct array *in, size_t index);
int array_elem_set(struct array *in, size_t index, const void *data);
int array_push(struct array **in, const void *data);
int array_pop(struct array *in);
int array_insert(struct array **in, size_t index, const void *data);
int array_remove(struct array *in, size_t index);
int array_concat(struct array **a, const struct array *b);

#endif // UTIL_ARRAY_H
