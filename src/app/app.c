#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/ecs.h>
#include <cls/gfx/gfx_api.h>
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

    return cls_arena_alloc(dest, (struct cls_arena *)ctx, size, align);
}

int cls_app_init(struct cls_app *app, struct cls_gfx_api *api,
                 ivec2 window_size, const char *title, ivec4 bg_color) {
    if (!app || !title)
        return CLS_NULLPTR;

    app->api = api;

    int error = cls_arena_create(&app->arena_perm, APP_PERM_SIZE);
    if (error)
        goto cleanup;

    error = cls_arena_create(&app->arena_frame, APP_FRAME_SIZE);
    if (error)
        goto cleanup;

    error = cls_allocator_create(&app->alloc_perm, allocator_arena_alloc, NULL,
                                 app->arena_perm);
    if (error)
        goto cleanup;

    error = cls_allocator_create(&app->alloc_frame, allocator_arena_alloc, NULL,
                                 app->arena_frame);
    if (error)
        goto cleanup;

    // TODO: Add checking vsync via an options file
    bool vsync = false;
    error = cls_window_create(&app->window, app->alloc_perm, app->alloc_frame,
                              app->api, window_size, title, vsync, bg_color);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Creating app window failed");
        goto cleanup;
    }

    error = cls_assets_create(&app->assets, app->alloc_perm, app->api);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Creating app assets failed");
        goto cleanup;
    }

    error = cls_ecs_create(&app->ecs, app->alloc_perm);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Creating app ecs failed");
        goto cleanup;
    }

    return CLS_SUCCESS;

cleanup:
    cls_app_destroy(app);
    return error;
}

void cls_app_destroy(struct cls_app *app) {
    if (!app)
        return;

    cls_ecs_destroy(app->ecs);
    cls_assets_destroy(app->assets);
    cls_window_destroy(app->window);
    cls_arena_destroy(app->arena_perm);
    cls_arena_destroy(app->arena_frame);
    cls_allocator_destroy(app->alloc_perm);
    cls_allocator_destroy(app->alloc_frame);
}

int cls_app_run(struct cls_app *app) {
    if (!app)
        return CLS_NULLPTR;

    bool should_close = false;
    while (!should_close) {
        int error = cls_window_update(&should_close, app->window);
        if (error)
            return error;

        error = cls_ecs_world_update_all(app->ecs, app);
        if (error) {
            CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                                 "Updating all ecs worlds failed");
            return error;
        }

        error = cls_window_renderer_update(app->window, app);
        if (error)
            return error;
    }

    return CLS_SUCCESS;
}
