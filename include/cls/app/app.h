/**
 * @file cls/app/app.h
 * @brief App state management for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/app/app.c
 */

#ifndef CLS_APP_H
#define CLS_APP_H

#include <cglm/types.h>
#include <cls/util/error.h>
#include <stdbool.h>

/* Forward declarations. */
struct cls_assets;
struct cls_ecs;
struct cls_renderer_api;
struct cls_mem;
struct cls_window;

/**
 * @defgroup app Application
 * @ingroup app
 * @brief Application state and logic.
 * @{
 */

/**
 * @struct cls_app
 * @brief Application state.
 */
struct cls_app {
    struct cls_assets *assets;
    struct cls_ecs *ecs;
    struct cls_renderer_api *api;
    struct cls_arena *arena_perm;
    struct cls_arena *arena_frame;
    struct cls_mem *mem_perm;
    struct cls_mem *mem_frame;
    struct cls_window *window;
};

/**
 * @brief Initializes an application.
 *
 * Creates the application's memory arenas, window, asset store, and ECS.
 * If initialization fails, any allocated resources are released before
 * returning.
 *
 * @param[in,out] app         Application to initialize. Must be allocated by
 *                            the caller.
 * @param[in] api             Graphics API.
 * @param[in] window_size     Initial window size.
 * @param[in] title           Window title.
 * @param[in] vsync           Vertical synchronization.
 * @param[in] bg_color        Initial background color.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `app` or `title` is NULL.
 * @retval (error)     If creating memory, the window, assets, or the ECS
 *                     fails.
 *
 * @code
 * struct cls_app app = {0};
 * cls_app_init(&app, api, (ivec2){500, 500}, "My Game", true, (ivec4){0, 0, 0,
 * 255});
 * cls_app_run(&app); cls_app_destroy(&app);
 * @endcode
 */
cls_error cls_app_init(struct cls_app *app, struct cls_renderer_api *api,
                       ivec2 window_size, const char *title, bool vsync,
                       const ivec4 bg_color);

/**
 * @brief Destroys an application.
 *
 * Releases the application's ECS, assets, window, memory allocators, and
 * arenas.
 *
 * @param[in] app Application to destroy.
 */
void cls_app_destroy(struct cls_app *app);

/**
 * @brief Runs the application's main loop.
 *
 * Updates the window, runs ECS updates, and renders each frame until the
 * window closes or an error occurs.
 *
 * @param[in] app Application to run. Must be initialized.
 *
 * @return CLS_SUCCESS If the window closes normally.
 * @retval CLS_NULLPTR If `app` is NULL.
 * @retval (error)     If updating the window, ECS, or renderer fails.
 */
cls_error cls_app_run(struct cls_app *app);

/** @} */

#endif // CLS_APP_H
