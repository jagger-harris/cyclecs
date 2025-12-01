#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include "core/util/types.h"
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_GROWTH_FACTOR 1.5

struct array {
    size_t capacity;
    size_t length;
    size_t elem_size;
    void *data;
};

int array_create(struct array **out, size_t start_capacity, size_t elem_size) {
    if (!out)
        return CORE_NULLPTR;

    if (start_capacity == 0 || elem_size == 0)
        return CORE_INVALID_ARG;

    const size_t align = alignof(max_align_t);
    size_t total_size =
        sizeof(struct array) + start_capacity * elem_size + (align - 1);

    struct array *array = malloc(total_size);
    if (!array)
        return CORE_OUT_OF_MEMORY;

    uintptr_t base = (uintptr_t)array + sizeof(struct array);
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);

    array->data = (void *)aligned;
    array->capacity = start_capacity;
    array->length = 0;
    array->elem_size = elem_size;

    *out = array;
    return CORE_SUCCESS;
}

void array_destroy(struct array *in) {
    if (!in)
        return;

    free(in);
}

int array_length_get(size_t *out, struct array *in) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = in->length;
    return CORE_SUCCESS;
}

int array_clear(struct array *in) {
    if (!in)
        return CORE_NULLPTR;

    in->length = 0;
    return CORE_SUCCESS;
}

int array_elem_get_mut(void **out, struct array *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARG;

    *out = (u8 *)in->data + index * in->elem_size;
    return CORE_SUCCESS;
}

int array_elem_get(const void **out, const struct array *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARG;

    *out = (u8 *)in->data + index * in->elem_size;
    return CORE_SUCCESS;
}

int array_elem_get_cpy(void *out, const struct array *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARG;

    memcpy(out, (u8 *)in->data + index * in->elem_size, in->elem_size);
    return CORE_SUCCESS;
}

int array_elem_set(struct array *in, size_t index, const void *data) {
    if (!in || !data)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARG;

    memcpy((u8 *)in->data + index * in->elem_size, data, in->elem_size);
    return CORE_SUCCESS;
}

static int array_grow(struct array **in) {
    struct array *array = *in;
    size_t new_capacity =
        (size_t)((double)array->capacity * ARRAY_GROWTH_FACTOR);

    if (new_capacity <= array->capacity)
        new_capacity = array->capacity + 1;

    const size_t align = alignof(max_align_t);
    size_t total_size =
        sizeof(struct array) + new_capacity * array->elem_size + (align - 1);

    size_t old_data_offset = (size_t)((u8 *)array->data - (u8 *)array);

    struct array *temp = realloc(array, total_size);
    if (!temp)
        return CORE_OUT_OF_MEMORY;

    uintptr_t base = (uintptr_t)temp + sizeof(struct array);
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
    void *new_data = (void *)aligned;

    if (new_data != (void *)((u8 *)temp + old_data_offset)) {
        memmove(new_data, (u8 *)temp + old_data_offset,
                temp->length * temp->elem_size);
    }

    temp->data = new_data;
    temp->capacity = new_capacity;
    *in = temp;
    return CORE_SUCCESS;
}

int array_push(struct array **in, const void *data) {
    if (!in || !*in || !data)
        return CORE_NULLPTR;

    struct array *array = *in;

    if (array->length >= array->capacity) {
        int error = array_grow(in);
        if (error)
            return error;

        array = *in;
    }

    memcpy((u8 *)array->data + array->length * array->elem_size, data,
           array->elem_size);

    array->length++;
    return CORE_SUCCESS;
}

int array_pop(struct array *in) {
    if (!in)
        return CORE_NULLPTR;

    if (in->length == 0)
        return CORE_INVALID_ARG;

    in->length--;
    return CORE_SUCCESS;
}

int array_insert(struct array **in, size_t index, const void *data) {
    if (!in || !*in || !data)
        return CORE_NULLPTR;

    struct array *array = *in;

    if (index > array->length)
        return CORE_INVALID_ARG;

    if (array->length >= array->capacity) {
        int error = array_grow(in);
        if (error)
            return error;

        array = *in;
    }

    memmove((u8 *)array->data + (index + 1) * array->elem_size,
            (u8 *)array->data + index * array->elem_size,
            (array->length - index) * array->elem_size);
    memcpy((u8 *)array->data + index * array->elem_size, data,
           array->elem_size);

    array->length++;
    return CORE_SUCCESS;
}

int array_remove(struct array *in, size_t index) {
    if (!in)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARG;

    memmove((u8 *)in->data + index * in->elem_size,
            (u8 *)in->data + (index + 1) * in->elem_size,
            (in->length - index - 1) * in->elem_size);

    in->length--;
    return CORE_SUCCESS;
}

int array_concat(struct array **in, const struct array *b) {
    if (!in || !*in || !b)
        return CORE_NULLPTR;

    struct array *array = *in;

    if (array->elem_size != b->elem_size)
        return CORE_INVALID_ARG;

    size_t concat_length = array->length + b->length;
    if (concat_length > array->capacity) {
        size_t new_capacity = array->capacity;
        while (new_capacity < concat_length)
            new_capacity = (size_t)((double)new_capacity * ARRAY_GROWTH_FACTOR);

        const size_t align = alignof(max_align_t);
        size_t total_size = sizeof(struct array) +
                            new_capacity * array->elem_size + (align - 1);

        size_t old_data_offset = (size_t)((u8 *)array->data - (u8 *)array);
        struct array *temp = realloc(array, total_size);
        if (!temp)
            return CORE_OUT_OF_MEMORY;

        uintptr_t base = (uintptr_t)temp + sizeof(struct array);
        uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
        void *new_data = (void *)aligned;

        if (new_data != (void *)((u8 *)temp + old_data_offset)) {
            memmove(new_data, (u8 *)temp + old_data_offset,
                    temp->length * temp->elem_size);
        }

        temp->data = new_data;
        temp->capacity = new_capacity;
        *in = temp;
        array = temp;
    }

    memcpy((u8 *)array->data + array->length * array->elem_size, b->data,
           b->length * b->elem_size);

    array->length = concat_length;
    return CORE_SUCCESS;
}
