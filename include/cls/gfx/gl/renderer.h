#ifndef CLS_GL_RENDERER_H
#define CLS_GL_RENDERER_H

#include <GLFW/glfw3.h>
#include <cglm/types.h>

/* Forward declarations. */
struct cls_app;
struct cls_array;
struct cls_renderer_ctx;

/**
 * @brief Initializes the OpenGL renderer.
 *
 * Loads OpenGL functions, initializes the debug state, sets the clear color,
 * and enables the default rendering state.
 *
 * @param[in] bg_color Background color.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_GL If loading OpenGL functions fails.
 */
int cls_gl_renderer_init(ivec4 bg_color);

/**
 * @brief Swaps window buffers.
 *
 * @param[in] win GLFW window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 */
int cls_gl_renderer_swap_buffers(GLFWwindow *win);

/**
 * @brief Updates the viewport.
 *
 * @param[in] width  Viewport width.
 * @param[in] height Viewport height.
 */
void cls_gl_renderer_on_resize(int width, int height);

/**
 * @brief Begins a frame.
 *
 * Resets the OpenGL state and clears the framebuffer.
 */
void cls_gl_renderer_begin_frame(void);

/**
 * @brief Draws renderer batches.
 *
 * Draws opaque batches first, then sorts and draws transparent batches.
 *
 * @param[in] app                     Application state.
 * @param[in] cmds                    Render commands.
 * @param[in] batches                 Batches to draw.
 * @param[in,out] transparent_batches Transparent batch storage.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `app` or `batches` is NULL.
 * @retval (error)     If reading or sorting batch data fails.
 */
int cls_gl_renderer_draw_batches(struct cls_app *app, struct cls_array *cmds,
                                 struct cls_array *batches,
                                 struct cls_array **transparent_batches);

#endif // CLS_GL_RENDERER_H
