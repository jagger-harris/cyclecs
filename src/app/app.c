/**
 * @file cls/app/app.c
 * @brief App state management for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/app/app.h
 */

#include <assert.h>
#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/ecs.h>
#include <cls/gfx/gl/shader.h>
#include <cls/gfx/renderer.h>
#include <cls/util/arena.h>
#include <cls/util/logger.h>
#include <cls/util/mem.h>

static const size_t APP_PERM_ARENA_SIZE = 1024L * 1024L;
static const size_t APP_FRAME_ARENA_SIZE = 1024L * 1024L * 50L;

static cls_error allocator_arena_alloc(void **dest, void *ctx, size_t size,
                                       size_t align) {
    assert(dest && ctx && "dest or ctx is NULL");

    return cls_arena_alloc(dest, (struct cls_arena *)ctx, size, align);
}

cls_error cls_app_init(struct cls_app *app, struct cls_renderer_api *api,
                       ivec2 window_size, const char *title, bool vsync,
                       const ivec4 bg_color) {
    if (!app || !title)
        return CLS_NULLPTR;

    app->api = api;

    cls_error error = cls_arena_create(&app->arena_perm, APP_PERM_ARENA_SIZE);
    if (error)
        goto cleanup;

    error = cls_arena_create(&app->arena_frame, APP_FRAME_ARENA_SIZE);
    if (error)
        goto cleanup;

    error = cls_mem_create(&app->mem_perm, allocator_arena_alloc, NULL,
                           app->arena_perm);
    if (error)
        goto cleanup;

    error = cls_mem_create(&app->mem_frame, allocator_arena_alloc, NULL,
                           app->arena_frame);
    if (error)
        goto cleanup;

    error = cls_window_create(&app->window, app->mem_perm, app->mem_frame,
                              app->api, window_size, title, vsync, bg_color);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Creating app window failed");
        goto cleanup;
    }

    error = cls_assets_create(&app->assets, app->mem_perm, app->api);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Creating app assets failed");
        goto cleanup;
    }

    error = cls_ecs_create(&app->ecs, app->mem_perm);
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
    cls_mem_destroy(app->mem_perm);
    cls_mem_destroy(app->mem_frame);
}

cls_error cls_app_run(struct cls_app *app) {
    if (!app)
        return CLS_NULLPTR;

    bool should_close = false;
    while (!should_close) {
        cls_error error = cls_window_update(&should_close, app->window);
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
