#include "renderer.h"
#include "../util/error.h"
#include "../util/logger.h"

struct renderer {
    api_swap_buffers swap;
    api_on_resize resize;
    api_clear clear;
};

int renderer_init(renderer *out, api_swap_buffers swap, api_on_resize resize,
                  api_clear clear) {
    int error = CORE_ERROR_SUCCESS;

    if (!out || !swap || !resize || !clear) {
        logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to init renderer api",
                   error);
        error = CORE_ERROR_INVALID_NULLPTR;
        return error;
    }

    out->swap = swap;
    out->resize = resize;
    out->clear = clear;

    return error;
}

int renderer_swap_buffers(renderer *in, GLFWwindow *window) {
    if (!in) {
        logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to swap buffers",
                   CORE_ERROR_INVALID_NULLPTR);
        return CORE_ERROR_INVALID_NULLPTR;
    }

    return in->swap(window);
}

int renderer_resize(renderer *in, int width, int height) {
    if (!in) {
        logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to resize renderer",
                   CORE_ERROR_INVALID_NULLPTR);
        return CORE_ERROR_INVALID_NULLPTR;
    }

    in->resize(width, height);
    return CORE_ERROR_SUCCESS;
}

int renderer_clear(renderer *in) {
    if (!in) {
        logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to clear renderer",
                   CORE_ERROR_INVALID_NULLPTR);
        return CORE_ERROR_INVALID_NULLPTR;
    }

    in->clear();
    return CORE_ERROR_SUCCESS;
}
