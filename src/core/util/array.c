#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include "core/util/types.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_GROWTH_FACTOR 1.5

int array_init(struct array *out, size_t start_capacity, size_t elem_size) {
    if (!out)
        return CORE_NULLPTR;

    if (start_capacity < 1 || elem_size < 1)
        return CORE_INVALID_ARGS;

    out->capacity = start_capacity;
    out->length = 0;
    out->elem_size = elem_size;
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

    free(in->data);
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
        return CORE_INVALID_ARGS;

    *out = (u8 *)in->data + (index * in->elem_size);
    return CORE_SUCCESS;
}

int array_get_cpy(void *out, const struct array *in, size_t index) {
    void *element = NULL;
    int status = CORE_SUCCESS;
    status = array_get(&element, in, index);
    if (status)
        return status;

    memcpy(out, (u8 *)in->data + (index * in->elem_size), in->elem_size);
    return CORE_SUCCESS;
}

int array_set(struct array *in, size_t index, void *data) {
    if (!in || !data)
        return CORE_NULLPTR;

    if (index >= in->length)
        return CORE_INVALID_ARGS;

    memcpy((u8 *)in->data + (index * in->elem_size), data, in->elem_size);
    return CORE_SUCCESS;
}

int array_push(struct array *in, void *data) {
    if (!in)
        return CORE_NULLPTR;

    int status = CORE_SUCCESS;
    status = array_insert(in, in->length, data);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int array_pop(struct array *in) {
    if (!in)
        return CORE_NULLPTR;

    int status = CORE_SUCCESS;
    status = array_remove(in, in->length - 1);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int array_insert(struct array *in, size_t index, void *data) {
    if (!in || !data)
        return CORE_NULLPTR;

    if (index > in->length)
        return CORE_INVALID_ARGS;

    if (in->length >= in->capacity) {
        size_t new_capacity = in->capacity * ARRAY_GROWTH_FACTOR;
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
        return CORE_INVALID_ARGS;

    memmove((u8 *)in->data + (index * in->elem_size),
            (u8 *)in->data + ((index + 1) * in->elem_size),
            (in->length - index - 1) * in->elem_size);
    in->length--;
    return CORE_SUCCESS;
}
