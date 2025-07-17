#include "core/util/array.h"
#include "core/util/logger.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_GROWTH_FACTOR 1.5

err array_init(struct array *out, size_t start_capacity, size_t elem_size) {
    err status = CORE_SUCCESS;

    out->capacity = start_capacity;
    out->length = 0;
    out->elem_size = elem_size;
    out->data = malloc(start_capacity * elem_size);
    if (!out->data) {
        status = CORE_OUT_OF_MEMORY;
        goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init array failed");
    return status;
}

void array_destroy(struct array *in) {
    if (!in)
        return;

    free(in->data);
}

err array_clear(struct array *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    in->length = 0;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Clearing array failed");
    return status;
}

err array_deep_cpy(struct array *out, const struct array *in) {
    err status = CORE_SUCCESS;

    if (!out || !in) {
        status = CORE_NULLPTR;
        goto err;
    }

    status = array_init(out, in->capacity, in->elem_size);
    if (status)
        goto err;

    if (0 < in->length && in->data) {
        memcpy(out->data, in->data, in->length * in->elem_size);
        out->length = in->length;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Deep copying array failed");
    return status;
}

err array_get_ptr(void **out, const struct array *in, size_t index) {
    err status = CORE_SUCCESS;

    if (!out || !in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (index >= in->length)
        return status;

    *out = (uint8_t *)in->data + (index * in->elem_size);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Getting array element failed");
    return status;
}

err array_get_cpy(void *out, const struct array *in, size_t index) {
    err status = CORE_SUCCESS;

    if (!out || !in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (index >= in->length) {
        status = CORE_ARGS;
        goto err;
    }

    memcpy(out, (uint8_t *)in->data + (index * in->elem_size), in->elem_size);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Getting array element copy failed");
    return status;
}

err array_push(struct array *in, void *data) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    status = array_insert(in, in->length, data);
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Pushing array element failed");
    return status;
}

err array_pop(struct array *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    status = array_remove(in, in->length);
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Popping array element failed");
    return status;
}

err array_push_multiple(struct array *in, void *data, size_t count) {
    err status = CORE_SUCCESS;

    if (!in || !data) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (1 > count)
        return status;

    while (in->length + count > in->capacity) {
        size_t new_capacity = in->capacity * ARRAY_GROWTH_FACTOR;
        void *temp = realloc(in->data, new_capacity * in->elem_size);

        if (!temp) {
            status = CORE_OUT_OF_MEMORY;
            goto err;
        }

        in->data = temp;
        in->capacity = new_capacity;
    }

    memcpy((uint8_t *)in->data + in->length * in->elem_size, data,
           count * in->elem_size);

    in->length += count;
    return status;

err:
    logger_log_err(LOGGER_ERR, status,
                   "Pushing array multiple elements failed");
    return status;
}

err array_insert(struct array *in, size_t index, void *data) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (in->length > in->capacity) {
        size_t new_capacity = in->capacity * ARRAY_GROWTH_FACTOR;
        void *temp = realloc(in->data, new_capacity * in->elem_size);
        if (!temp) {
            status = CORE_OUT_OF_MEMORY;
            goto err;
        }

        in->data = temp;
        in->capacity = new_capacity;
    }

    memcpy((uint8_t *)in->data + (index * in->elem_size), data, in->elem_size);
    in->length++;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Inserting array element failed");
    return status;
}

err array_remove(struct array *in, size_t index) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (index >= in->length) {
        status = CORE_ARGS;
        goto err;
    }

    memmove((uint8_t *)in->data + (index * in->elem_size),
            (uint8_t *)in->data + ((index + 1) * in->elem_size),
            (in->length - index - 1) * in->elem_size);
    in->length--;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Failed to pop element from array");
    return status;
}
