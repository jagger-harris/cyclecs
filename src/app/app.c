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

#define APP_PERM_SIZE (1024L * 1024L)
#define APP_FRAME_SIZE (1024L * 1024L * 50L)

static int allocator_arena_alloc(void **dest, void *ctx, size_t size,
                                 size_t align) {
    if (!dest || !ctx)
        return CLS_NULLPTR;

    return arena_alloc(dest, (struct arena *)ctx, size, align);
}

int app_init(struct app *app, struct gfx_api *api, ivec2 window_size,
             const char *title, ivec4 bg_color) {
    if (!app || !title)
        return CLS_NULLPTR;

    app->api = api;

    int error = arena_create(&app->arena_perm, APP_PERM_SIZE);
    if (error)
        goto cleanup;

    error = arena_create(&app->arena_frame, APP_FRAME_SIZE);
    if (error)
        goto cleanup;

    error = allocator_create(&app->alloc_perm, allocator_arena_alloc, NULL,
                             app->arena_perm);
    if (error)
        goto cleanup;

    error = allocator_create(&app->alloc_frame, allocator_arena_alloc, NULL,
                             app->arena_frame);
    if (error)
        goto cleanup;

    // TODO: Add checking vsync via an options file
    bool vsync = false;
    error = window_create(&app->window, app->alloc_perm, app->alloc_frame,
                          app->api, window_size, title, vsync, bg_color);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating app window failed");
        goto cleanup;
    }

    error = assets_create(&app->assets, app->alloc_perm, app->api);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating app assets failed");
        goto cleanup;
    }

    error = ecs_create(&app->ecs, app->alloc_perm);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Creating app ecs failed");
        goto cleanup;
    }

    return CLS_SUCCESS;

cleanup:
    app_destroy(app);
    return error;
}

void app_destroy(struct app *app) {
    if (!app)
        return;

    ecs_destroy(app->ecs);
    assets_destroy(app->assets);
    window_destroy(app->window);
    arena_destroy(app->arena_perm);
    arena_destroy(app->arena_frame);
    allocator_destroy(app->alloc_perm);
    allocator_destroy(app->alloc_frame);
}

int app_run(struct app *app) {
    if (!app)
        return CLS_NULLPTR;

    bool should_close = false;
    while (!should_close) {
        int error = window_update(&should_close, app->window);
        if (error)
            return error;

        error = ecs_world_update_all(app->ecs, app);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Updating all ecs worlds failed");
            return error;
        }

        error = window_renderer_update(app->window, app);
        if (error)
            return error;
    }

    return CLS_SUCCESS;
}
