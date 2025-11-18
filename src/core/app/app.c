#include "core/app/app.h"
#include "core/app/assets.h"
#include "core/app/window.h"
#include "core/ecs/ecs.h"
#include "core/gfx/api.h"
#include "core/gfx/gl/renderer.h"
#include "core/gfx/gl/shader.h"
#include "core/gfx/renderer.h"
#include "core/util/arena.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include "core/util/mem.h"

#define ARENA_PERSISTANT_BUFFER_SIZE (1024L * 1024L)
#define ARENA_FRAME_BUFFER_SIZE (1024L * 1024L * 50L)

static int mem_arena_alloc(void **out, void *ctx, size_t size, size_t align) {
    if (!out || !ctx)
        return CORE_NULLPTR;

    return arena_alloc(out, (struct arena *)ctx, size, align);
}

int app_init(struct app *out, struct gfx_api *api, void *game_state,
             ivec2 window_size, const char *title, ivec4 bg_color) {
    if (!out || !title)
        return CORE_NULLPTR;

    out->api = api;
    out->game_state = game_state;

    int error =
        arena_create(&out->arena_persistant, ARENA_PERSISTANT_BUFFER_SIZE);
    if (error)
        goto cleanup;

    error = arena_create(&out->arena_frame, ARENA_FRAME_BUFFER_SIZE);
    if (error)
        goto cleanup;

    error = mem_create(&out->mem_persistant, mem_arena_alloc, NULL,
                       out->arena_persistant);
    if (error)
        goto cleanup;

    error =
        mem_create(&out->mem_frame, mem_arena_alloc, NULL, out->arena_frame);
    if (error)
        goto cleanup;

    // TODO: Add checking vsync via an options file
    bool vsync = false;
    error = window_create(&out->window, out->mem_persistant, out->mem_frame,
                          out->api, window_size, title, vsync, bg_color);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating app window failed");
        goto cleanup;
    }

    error = assets_create(&out->assets, out->mem_persistant, out->api);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating app assets failed");
        goto cleanup;
    }

    error = ecs_create(&out->ecs, out->mem_persistant);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Creating app ecs failed");
        goto cleanup;
    }

    return CORE_SUCCESS;

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
    mem_destroy(in->mem_persistant);
    mem_destroy(in->mem_frame);
}

int app_run(struct app *in, const game_update_fn game_update) {
    if (!in || !game_update)
        return CORE_NULLPTR;

    bool close = false;
    int error = window_should_close(&close, in->window);
    if (error)
        return error;

    while (!close) {
        error = window_update(&close, in->window);
        if (error)
            return error;

        error = game_update(in);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Updating game failed");
            return error;
        }

        error = ecs_update_all_worlds(in->ecs, in);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Updating all ecs worlds failed");
            return error;
        }

        error = window_renderer_update(in->window, in);
        if (error)
            return error;
    }

    return CORE_SUCCESS;
}
