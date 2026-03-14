#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/ecs.h>
#include <cls/gfx/api.h>
#include <cls/gfx/gl/renderer.h>
#include <cls/gfx/gl/shader.h>
#include <cls/gfx/renderer.h>
#include <cls/util/allocator.h>
#include <cls/util/arena.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>

#define ARENA_PERSISTANT_BUFFER_SIZE (1024L * 1024L)
#define ARENA_FRAME_BUFFER_SIZE (1024L * 1024L * 50L)

static int allocator_arena_alloc(void **out, void *ctx, size_t size,
                                 size_t align) {
    if (!out || !ctx)
        return CLS_NULLPTR;

    return arena_alloc(out, (struct arena *)ctx, size, align);
}

int app_init(struct app *out, struct gfx_api *api, ivec2 window_size,
             const char *title, ivec4 bg_color) {
    if (!out || !title)
        return CLS_NULLPTR;

    out->api = api;

    int error =
        arena_create(&out->arena_persistant, ARENA_PERSISTANT_BUFFER_SIZE);
    if (error)
        goto cleanup;

    error = arena_create(&out->arena_frame, ARENA_FRAME_BUFFER_SIZE);
    if (error)
        goto cleanup;

    error = allocator_create(&out->alloc_persistant, allocator_arena_alloc,
                             NULL, out->arena_persistant);
    if (error)
        goto cleanup;

    error = allocator_create(&out->alloc_frame, allocator_arena_alloc, NULL,
                             out->arena_frame);
    if (error)
        goto cleanup;

    // TODO: Add checking vsync via an options file
    bool vsync = false;
    error = window_create(&out->window, out->alloc_persistant, out->alloc_frame,
                          out->api, window_size, title, vsync, bg_color);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating app window failed");
        goto cleanup;
    }

    error = assets_create(&out->assets, out->alloc_persistant, out->api);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating app assets failed");
        goto cleanup;
    }

    error = ecs_create(&out->ecs, out->alloc_persistant);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Creating app ecs failed");
        goto cleanup;
    }

    return CLS_SUCCESS;

cleanup:
    app_destroy(out);
    return error;
}

void app_destroy(struct app *in) {
    if (!in)
        return;

    ecs_destroy(in->ecs);
    assets_destroy(in->assets);
    window_destroy(in->window);
    arena_destroy(in->arena_persistant);
    arena_destroy(in->arena_frame);
    allocator_destroy(in->alloc_persistant);
    allocator_destroy(in->alloc_frame);
}

int app_run(struct app *in) {
    if (!in)
        return CLS_NULLPTR;

    bool close = false;
    int error = window_should_close(&close, in->window);
    if (error)
        return error;

    while (!close) {

        error = window_update(&close, in->window);
        if (error)
            return error;

        error = ecs_world_update_all(in->ecs, in);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Updating all ecs worlds failed");
            return error;
        }

        error = window_renderer_update(in->window, in);
        if (error)
            return error;
    }

    return CLS_SUCCESS;
}
