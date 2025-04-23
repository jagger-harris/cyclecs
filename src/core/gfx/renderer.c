#include "core/gfx/renderer.h"
#include "core/util/logger.h"

struct renderer {
    api_init init;
    api_swap_buffers swap;
    api_on_resize resize;
    api_render_frame render_frame;
};

err renderer_new(renderer **out, arena *mem, api_init init,
                 api_swap_buffers swap, api_on_resize resize,
                 api_render_frame render_frame) {
    err err = CORE_SUCCESS;

    if (!out || !mem || !init || !swap || !resize || !render_frame) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = arena_alloc((void **)out, mem, sizeof(renderer), _Alignof(renderer));
    if (err)
        goto err;

    (*out)->init = init;
    (*out)->swap = swap;
    (*out)->resize = resize;
    (*out)->render_frame = render_frame;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to init renderer api", err);
    return err;
}

err renderer_use(renderer *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = in->init();
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to use renderer api", err);
    return err;
}

err renderer_swap_buffers(renderer *in, GLFWwindow *window) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = in->swap(window);
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to swap buffers", err);
    return err;
}

err renderer_resize(renderer *in, int width, int height) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    in->resize(width, height);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to resize renderer", err);
    return err;
}

err renderer_render_frame(renderer *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    // Probably need to add the renderer to the render_frame, you decide
    in->render_frame();
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to clear renderer", err);
    return err;
}

err renderer_draw_enqueue(renderer *in, struct material *mat,
                          struct transform *tf) {
    err err = CORE_SUCCESS;

    if (!in || !mat || !tf) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    // Add to rendering queue somehow

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to add draw to render queue", err);
    return err;
}
