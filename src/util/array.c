/**
 * @file cls/util/array.c
 * @brief Dynamic array implementation for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/array.h
 */

#include <assert.h>
#include <cls/util/array.h>
#include <cls/util/types.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

static const double ARRAY_GROWTH_FACTOR = 1.5;

struct cls_array {
    size_t capacity;
    size_t length;
    size_t elem_size;
    void *data;
};

static cls_error array_grow(struct cls_array **arr) {
    assert(arr && "in is NULL");

    struct cls_array *array = *arr;
    size_t new_capacity =
        (size_t)((double)array->capacity * ARRAY_GROWTH_FACTOR);

    if (new_capacity <= array->capacity)
        new_capacity = array->capacity + 1;

    const size_t align = alignof(max_align_t);
    size_t total_size = sizeof(struct cls_array) +
                        new_capacity * array->elem_size + (align - 1);

    size_t old_data_offset = (size_t)((u8 *)array->data - (u8 *)array);

    struct cls_array *temp = realloc(array, total_size);
    if (!temp)
        return CLS_OUT_OF_MEMORY;

    uintptr_t base = (uintptr_t)temp + sizeof(struct cls_array);
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
    void *new_data = (void *)aligned;

    if (new_data != (void *)((u8 *)temp + old_data_offset)) {
        memmove(new_data, (u8 *)temp + old_data_offset,
                temp->length * temp->elem_size);
    }

    temp->data = new_data;
    temp->capacity = new_capacity;
    *arr = temp;
    return CLS_SUCCESS;
}

cls_error cls_array_create(struct cls_array **a, size_t start_capacity,
                           size_t elem_size) {
    if (!a)
        return CLS_NULLPTR;

    if (start_capacity == 0 || elem_size == 0)
        return CLS_INVALID_ARG;

    const size_t align = alignof(max_align_t);
    size_t total_size =
        sizeof(struct cls_array) + start_capacity * elem_size + (align - 1);

    struct cls_array *array = malloc(total_size);
    if (!array)
        return CLS_OUT_OF_MEMORY;

    uintptr_t base = (uintptr_t)array + sizeof(struct cls_array);
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);

    array->data = (void *)aligned;
    array->capacity = start_capacity;
    array->length = 0;
    array->elem_size = elem_size;

    *a = array;
    return CLS_SUCCESS;
}

void cls_array_destroy(struct cls_array *a) {
    if (!a)
        return;

    free(a);
}

cls_error cls_array_length_get(size_t *out, struct cls_array *a) {
    if (!out || !a)
        return CLS_NULLPTR;

    *out = a->length;
    return CLS_SUCCESS;
}

cls_error cls_array_data_get(void **out, struct cls_array *a) {
    if (!out || !a)
        return CLS_NULLPTR;

    *out = a->data;
    return CLS_SUCCESS;
}

cls_error cls_array_clear(struct cls_array *a) {
    if (!a)
        return CLS_NULLPTR;

    a->length = 0;
    return CLS_SUCCESS;
}

cls_error cls_array_elem_get(void **out, const struct cls_array *a,
                             size_t index) {
    if (!out || !a)
        return CLS_NULLPTR;

    if (index >= a->length)
        return CLS_INVALID_ARG;

    *out = (u8 *)a->data + index * a->elem_size;
    return CLS_SUCCESS;
}

cls_error cls_array_elem_get_cpy(void *out, const struct cls_array *a,
                                 size_t index) {
    if (!out || !a)
        return CLS_NULLPTR;

    if (index >= a->length)
        return CLS_INVALID_ARG;

    memcpy(out, (u8 *)a->data + index * a->elem_size, a->elem_size);
    return CLS_SUCCESS;
}

cls_error cls_array_elem_set(struct cls_array *a, size_t index,
                             const void *data) {
    if (!a || !data)
        return CLS_NULLPTR;

    if (index >= a->length)
        return CLS_INVALID_ARG;

    memcpy((u8 *)a->data + index * a->elem_size, data, a->elem_size);
    return CLS_SUCCESS;
}

cls_error cls_array_push(struct cls_array **in, const void *data) {
    if (!in || !*in || !data)
        return CLS_NULLPTR;

    struct cls_array *array = *in;

    if (array->length >= array->capacity) {
        cls_error error = array_grow(in);
        if (error)
            return error;

        array = *in;
    }

    memcpy((u8 *)array->data + array->length * array->elem_size, data,
           array->elem_size);

    array->length++;
    return CLS_SUCCESS;
}

cls_error cls_array_pop(void *last, struct cls_array *a) {
    if (!a)
        return CLS_NULLPTR;

    if (a->length == 0)
        return CLS_INVALID_ARG;

    if (last) {
        size_t last_index = a->length - 1;
        memcpy(last, (u8 *)a->data + last_index * a->elem_size, a->elem_size);
    }

    a->length--;
    return CLS_SUCCESS;
}

cls_error cls_array_insert(struct cls_array **a, size_t index,
                           const void *data) {
    if (!a || !*a || !data)
        return CLS_NULLPTR;

    struct cls_array *array = *a;

    if (index > array->length)
        return CLS_INVALID_ARG;

    if (array->length >= array->capacity) {
        cls_error error = array_grow(a);
        if (error)
            return error;

        array = *a;
    }

    memmove((u8 *)array->data + (index + 1) * array->elem_size,
            (u8 *)array->data + index * array->elem_size,
            (array->length - index) * array->elem_size);
    memcpy((u8 *)array->data + index * array->elem_size, data,
           array->elem_size);

    array->length++;
    return CLS_SUCCESS;
}

cls_error cls_array_remove(struct cls_array *a, size_t index) {
    if (!a)
        return CLS_NULLPTR;

    if (index >= a->length)
        return CLS_INVALID_ARG;

    memmove((u8 *)a->data + index * a->elem_size,
            (u8 *)a->data + (index + 1) * a->elem_size,
            (a->length - index - 1) * a->elem_size);

    a->length--;
    return CLS_SUCCESS;
}

cls_error cls_array_concat(struct cls_array **dest,
                           const struct cls_array *src) {
    if (!dest || !*dest || !src)
        return CLS_NULLPTR;

    struct cls_array *array = *dest;

    if (array->elem_size != src->elem_size)
        return CLS_INVALID_ARG;

    size_t concat_length = array->length + src->length;
    if (concat_length > array->capacity) {
        size_t new_capacity = array->capacity;
        while (new_capacity < concat_length)
            new_capacity = (size_t)((double)new_capacity * ARRAY_GROWTH_FACTOR);

        const size_t align = alignof(max_align_t);
        size_t total_size = sizeof(struct cls_array) +
                            new_capacity * array->elem_size + (align - 1);

        size_t old_data_offset = (size_t)((u8 *)array->data - (u8 *)array);
        struct cls_array *temp = realloc(array, total_size);
        if (!temp)
            return CLS_OUT_OF_MEMORY;

        uintptr_t base = (uintptr_t)temp + sizeof(struct cls_array);
        uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
        void *new_data = (void *)aligned;

        if (new_data != (void *)((u8 *)temp + old_data_offset)) {
            memmove(new_data, (u8 *)temp + old_data_offset,
                    temp->length * temp->elem_size);
        }

        temp->data = new_data;
        temp->capacity = new_capacity;
        *dest = temp;
        array = temp;
    }

    memcpy((u8 *)array->data + array->length * array->elem_size, src->data,
           src->length * src->elem_size);

    array->length = concat_length;
    return CLS_SUCCESS;
}
