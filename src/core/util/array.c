#include "core/util/array.h"
#include "core/util/logger.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_GROWTH_FACTOR 1.5

struct array {
    void *data;
    size_t capacity;
    size_t length;
    size_t elem_size;
};

err array_new(array **out, size_t start_capacity, size_t elem_size) {
    err err = CORE_SUCCESS;

    /* TODO: Use arena or other allocator if possible */
    *out = malloc(sizeof(array));
    if (!out) {
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    (*out)->capacity = start_capacity;
    (*out)->length = 0;
    (*out)->elem_size = elem_size;
    (*out)->data = malloc(start_capacity * elem_size);
    if (!(*out)->data) {
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new array", err);

    if (*out)
        free(*out);

    return err;
}

err array_delete(array *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    free(in->data);
    free(in);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete array", err);
    return err;
}

err array_length(size_t *out, array *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = in->length;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get array length", err);
    return err;
}

err array_at(void **out, array *in, size_t index) {
    err err = CORE_SUCCESS;

    if (!out || !in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (index >= in->length) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    *out = (uint8_t *)in->data + (index * in->elem_size);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get element ptr in array", err);
    return err;
}

err array_at_cpy(void *out, array *in, size_t index) {
    err err = CORE_SUCCESS;

    if (!out || !in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (index >= in->length) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    memcpy(out, (uint8_t *)in->data + (index * in->elem_size), in->elem_size);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get element in array", err);
    return err;
}

err array_push(array *in, void *data) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = array_insert(in, in->length, data);
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to push element to array", err);
    return err;
}

err array_pop(array *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = array_remove(in, in->length);
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to push element to array", err);
    return err;
}

err array_insert(array *in, size_t index, void *data) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (in->length >= in->capacity) {
        size_t new_capacity = in->capacity * ARRAY_GROWTH_FACTOR;
        void *temp = realloc(in->data, new_capacity * in->elem_size);
        if (!temp) {
            err = CORE_OUT_OF_MEMORY;
            goto err;
        }

        in->data = temp;
        in->capacity = new_capacity;
    }

    memcpy((uint8_t *)in->data + (index * in->elem_size), data, in->elem_size);
    in->length++;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to push element to array", err);
    return err;
}

err array_remove(array *in, size_t index) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (index >= in->length) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    memmove((uint8_t *)in->data + (index * in->elem_size),
            (uint8_t *)in->data + ((index + 1) * in->elem_size),
            (in->length - index - 1) * in->elem_size);
    in->length--;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to pop element from array", err);
    return err;
}
