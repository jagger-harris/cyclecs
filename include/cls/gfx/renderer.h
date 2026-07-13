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
struct cls_ecs_world;
struct renderable;
struct transform;

/**
 * @struct cls_renderer
 * @brief Renderer.
 */
struct cls_renderer;

/**
 * @struct cls_renderer_cmd
 * @brief Renderer command.
 */
struct cls_renderer_cmd {
    struct renderable ren;
    struct transform tf;
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
int cls_renderer_create(struct cls_renderer **rend, struct cls_mem *mem_perm,
                        struct cls_mem *mem_frame, struct cls_gfx_api *api,
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
int cls_renderer_swap_buffers(struct cls_renderer *rend, GLFWwindow *window);

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
int cls_renderer_on_resize(struct cls_renderer *rend, int width, int height);

/**
 * @brief Pushes a render command.
 *
 * Adds a render command to the renderer's command list for the current
 * frame.
 *
 * @param[in] rend  Renderer.
 * @param[in] ren   Renderable.
 * @param[in] tf    Transform.
 * @param[in] depth Depth value.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `rend`, `ren`, or `tf` is NULL.
 * @retval (error)     If adding the command fails.
 */
int cls_renderer_cmd_push(struct cls_renderer *rend, struct renderable *ren,
                          struct transform *tf, float depth);

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
int cls_renderer_frame_create(struct cls_renderer *rend, struct cls_app *app);

#endif // CLS_RENDERER_H
