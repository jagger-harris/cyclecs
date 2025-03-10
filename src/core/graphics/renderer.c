#include "renderer.h"
#include "../util/error.h"
#include "../util/logger.h"

struct renderer {
    api_init init;
    api_swap_buffers swap;
    api_on_resize resize;
    api_clear clear;
};

int renderer_init(renderer **out, arena *in, api_init init,
                  api_swap_buffers swap, api_on_resize resize,
                  api_clear clear) {
    int error = CORE_SUCCESS;

    error = arena_alloc((void **)out, in, sizeof(renderer), _Alignof(renderer));
    if (error)
        goto error;

    if (!out) {
        error = CORE_FAILURE;
        goto error;
    }

    if (!init || !swap || !resize || !clear) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    (*out)->init = init;
    (*out)->swap = swap;
    (*out)->resize = resize;
    (*out)->clear = clear;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to init renderer api", error);
    return error;
}

int renderer_use(renderer *in) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    error = in->init();
    if (error)
        goto error;

    return in->init();

error:
    logger_log(LOGGER_ERROR, "Failed to use renderer api", error);
    return error;
    return CORE_INVALID_NULLPTR;
}

int renderer_swap_buffers(renderer *in, GLFWwindow *window) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    error = in->swap(window);
    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to swap buffers", error);
    return error;
}

int renderer_resize(renderer *in, int width, int height) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    in->resize(width, height);
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to resize renderer", error);
    return error;
}

int renderer_clear(renderer *in) {
    int error;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    in->clear();
    return CORE_SUCCESS;
error:
    logger_log(LOGGER_ERROR, "Failed to clear renderer", error);
    return error;
}
