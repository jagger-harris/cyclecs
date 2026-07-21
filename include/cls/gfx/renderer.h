/**
 * @file cls/gfx/renderer.h
 * @brief Renderer for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/gfx/renderer.c
 */

#ifndef CLS_RENDERER_H
#define CLS_RENDERER_H

#include <cglm/types.h>
#include <cls/ecs/component/components.h>
#include <cls/util/array.h>
#include <cls/util/mem.h>
#include <cls/util/table.h>

/* Forward declarations. */
typedef struct GLFWwindow GLFWwindow;
struct cls_app;
struct cls_gfx_api;
struct cls_mem;
struct cls_shader_info;
struct cls_ecs_world;

/**
 * @defgroup renderer Renderer.
 * @ingroup gfx
 * @brief Universal renderer.
 * @{
 */

/**
 * @struct cls_renderer
 * @brief Renderer.
 */
struct cls_renderer;

/**
 * @struct cls_renderer_api
 * @brief Renderer API.
 */
struct cls_renderer_api {
    cls_error (*init)(ivec4 bg_color);
    cls_error (*swap_buffers)(GLFWwindow *win);
    void (*on_resize)(int width, int height);
    void (*begin_frame)(void);
    cls_error (*draw_batches)(struct cls_app *app, struct cls_array *cmds,
                              struct cls_array *batches,
                              struct cls_array **transparent_batches);
    cls_error (*shader_init)(struct cls_shader *s,
                             const struct cls_shader_info *info);
    void (*shader_destroy)(struct cls_shader *s);
    cls_error (*shader_use)(const struct cls_shader *s);
    cls_error (*texture2d_init)(struct cls_texture2d *tex,
                                struct cls_texture2d_info *info);
    void (*texture2d_destroy)(struct cls_texture2d *tex);
    cls_error (*texture2d_use)(const struct cls_texture2d *tex);
};

/**
 * @struct cls_renderer_cmd
 * @brief Renderer command.
 */
struct cls_renderer_cmd {
    struct cls_renderable ren;
    struct cls_transform tf;
    mat4 mvp;
    float depth;
};

/**
 * @struct cls_renderer_batch_data
 * @brief Renderer batch data.
 */
struct cls_renderer_batch_data {
    struct cls_render_state state;
    u32 mesh_id;
    u32 shader_id;
    u32 texture_id;
    bool transparent;
};

/**
 * @struct cls_renderer_batch
 * @brief Renderer batch.
 */
struct cls_renderer_batch {
    struct cls_renderer_batch_data data;
    struct cls_array *cmds;
    float depth;
};

/**
 * @brief Creates a renderer.
 *
 * Initializes the renderer, creates its command and batch storage, and
 * initializes the graphics API with the background color. Destroy the
 * returned renderer with cls_renderer_destroy().
 *
 * @param[out] rend      Renderer.
 * @param[in]  mem_perm  Memory allocator used for persistent renderer state.
 * @param[in]  mem_frame Memory allocator used for per frame renderer state.
 * @param[in]  api       Graphics API.
 * @param[in]  bg_color  Background color.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `rend`, `mem_perm`, `mem_frame`, or `api` is NULL.
 * @retval (error)     If allocation, storage creation, or graphics API
 *                     initialization fails.
 *
 * @code
 * struct cls_renderer *rend;
 * ivec4 bg = {0, 0, 0, 255};
 * cls_renderer_create(&rend, mem_perm, mem_frame, api, bg);
 * // Use rend.
 * cls_renderer_destroy(rend);
 * @endcode
 */
cls_error cls_renderer_create(struct cls_renderer **rend,
                              struct cls_mem *mem_perm,
                              struct cls_mem *mem_frame,
                              struct cls_renderer_api *api,
                              const ivec4 bg_color);

/**
 * @brief Destroys a renderer.
 *
 * Destroys the renderer and releases its command and batch storage.
 *
 * @param[in] rend Renderer to destroy.
 */
void cls_renderer_destroy(struct cls_renderer *rend);

/**
 * @brief Swaps the window buffers.
 *
 * @param[in] rend   Renderer.
 * @param[in] window Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `rend` or `window` is NULL.
 * @retval (error)     If swapping the buffers fails.
 */
cls_error cls_renderer_swap_buffers(struct cls_renderer *rend,
                                    GLFWwindow *window);

/**
 * @brief Handles a renderer resize.
 *
 * Updates the graphics API with the new window size.
 *
 * @param[in] rend   Renderer.
 * @param[in] width  New width.
 * @param[in] height New height.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `rend` is NULL.
 */
cls_error cls_renderer_on_resize(struct cls_renderer *rend, int width,
                                 int height);

/**
 * @brief Pushes a render command.
 *
 * Adds a render command to the renderer's command list for the current
 * frame.
 *
 * @param[in] rend Renderer.
 * @param[in] cmd  Renderer cmd.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `rend` or `cmd` is NULL.
 * @retval (error)     If adding the command fails.
 */
cls_error cls_renderer_cmd_push(struct cls_renderer *rend,
                                struct cls_renderer_cmd *cmd);

/**
 * @brief Creates a renderer frame.
 *
 * Begins the frame, batches and draws queued commands, and resets the
 * renderer state for the next frame.
 *
 * @param[in] rend Renderer.
 * @param[in] app  Application state.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `rend` or `app` is NULL.
 * @retval (error)     If batching, drawing, or resetting the frame fails.
 */
cls_error cls_renderer_frame_create(struct cls_renderer *rend,
                                    struct cls_app *app);

/** @} */

#endif // CLS_RENDERER_H
