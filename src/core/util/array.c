#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include "core/util/types.h"
#include <stdlib.h>
#include <string.h>

#define ARRAY_GROWTH_FACTOR 1.5

int array_init(struct array *out, size_t start_capacity, size_t elem_size) {
    if (!out)
        return CORE_NULLPTR;

    if (start_capacity < 1 || elem_size < 1)
        return CORE_INVALID_ARG;

    *out = (struct array){.capacity = start_capacity,
                          .length = 0,
                          .elem_size = elem_size,
                          .data = NULL};

    out->data = malloc(start_capacity * elem_size);
    if (!out->data) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_OUT_OF_MEMORY, "%s",
                         "Running app failed");
        return CORE_OUT_OF_MEMORY;
    }

    return CORE_SUCCESS;
}

void array_destroy(struct array *in) {
    if (!in)
        return;

    if (in->data) {
        free(in->data);
        in->data = NULL;
    }
}

int array_clear(struct array *in) {
    if (!in)
        return CORE_NULLPTR;

    in->length = 0;
    return CORE_SUCCESS;
}

int array_get(void **out, const struct array *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARG;

    *out = (u8 *)in->data + (index * in->elem_size);
    return CORE_SUCCESS;
}

int array_get_cpy(void *out, const struct array *in, size_t index) {
    void *element = NULL;
    int error = array_get(&element, in, index);
    if (error)
        return error;

    memcpy(out, (u8 *)in->data + (index * in->elem_size), in->elem_size);
    return CORE_SUCCESS;
}

int array_set(struct array *in, size_t index, void *data) {
    if (!in || !data)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARG;

    memcpy((u8 *)in->data + (index * in->elem_size), data, in->elem_size);
    return CORE_SUCCESS;
}

int array_push(struct array *in, void *data) {
    if (!in)
        return CORE_NULLPTR;

    int error = array_insert(in, in->length, data);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int array_pop(struct array *in) {
    if (!in)
        return CORE_NULLPTR;

    int error = array_remove(in, in->length - 1);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int array_insert(struct array *in, size_t index, void *data) {
    if (!in || !data)
        return CORE_NULLPTR;

    if (index > in->length)
        return CORE_INVALID_ARG;

    if (in->length >= in->capacity) {
        size_t new_capacity =
            (size_t)((double)in->capacity * ARRAY_GROWTH_FACTOR);

        if (new_capacity <= in->capacity)
            new_capacity = in->capacity + 1;

        void *temp = realloc(in->data, new_capacity * in->elem_size);
        if (!temp) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_OUT_OF_MEMORY, "%s",
                             "Inserting new element into array failed");
            return CORE_OUT_OF_MEMORY;
        }

        in->data = temp;
        in->capacity = new_capacity;
    }

    if (index < in->length) {
        memmove((u8 *)in->data + ((index + 1) * in->elem_size),
                (u8 *)in->data + (index * in->elem_size),
                (in->length - index) * in->elem_size);
    }

    memcpy((u8 *)in->data + (index * in->elem_size), data, in->elem_size);
    in->length++;
    return CORE_SUCCESS;
}

int array_remove(struct array *in, size_t index) {
    if (!in)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARG;

    memmove((u8 *)in->data + (index * in->elem_size),
            (u8 *)in->data + ((index + 1) * in->elem_size),
            (in->length - index - 1) * in->elem_size);
    in->length--;
    return CORE_SUCCESS;
}

int array_concat(struct array *a, const struct array *b) {
    if (!a || !b)
        return CORE_NULLPTR;

    if (a->elem_size != b->elem_size)
        return CORE_INVALID_ARG;

    if (b->length == 0)
        return CORE_SUCCESS;

    size_t required_capacity = a->length + b->length;
    if (required_capacity > a->capacity) {
        size_t new_capacity = a->capacity;

        while (new_capacity < required_capacity) {
            new_capacity = (size_t)((double)new_capacity * ARRAY_GROWTH_FACTOR);
            if (new_capacity < a->capacity + 1)
                new_capacity = required_capacity;
        }

        void *temp = realloc(a->data, new_capacity * a->elem_size);
        if (!temp)
            return CORE_OUT_OF_MEMORY;

        a->data = temp;
        a->capacity = new_capacity;
    }

    memcpy((char *)a->data + (a->length * a->elem_size), b->data,
           b->length * b->elem_size);
    a->length += b->length;
    return CORE_SUCCESS;
}
