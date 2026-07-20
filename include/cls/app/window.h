/**
 * @file cls/app/window.h
 * @brief Assets management for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/app/window.c
 */

#ifndef CLS_WINDOW_H
#define CLS_WINDOW_H

#include <cglm/types.h>
#include <cls/util/error.h>
#include <stdbool.h>
#include <stddef.h>

/* Forward declarations. */
struct cls_app;
struct cls_gfx_api;
struct cls_mem;
struct cls_renderer;
struct cls_renderer_api;

/**
 * @defgroup window Window
 * @ingroup app
 * @brief Window state and logic for the application.
 * @{
 */

/**
 * @struct cls_window
 * @brief Window.
 */
struct cls_window;

/**
 * @brief Creates a window.
 *
 * Initializes GLFW, creates a window, and initializes its input, renderer,
 * and timing state. Destroy the returned window with cls_window_destroy().
 *
 * @param[out] win            Window.
 * @param[in]  mem_persistant Memory allocator used for the window's persistent
 *                            state.
 * @param[in]  mem_frame      Memory allocator used for per-frame renderer data.
 * @param[in]  api            Graphics API.
 * @param[in]  size           Initial window size.
 * @param[in]  title          Window title.
 * @param[in]  vsync          Whether to enable vertical sync.
 * @param[in]  bg_color       Initial background color.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win`, `mem_persistant`, `mem_frame`, or `title` is
 *                     NULL.
 * @retval CLS_GLFW    If GLFW initialization or window creation fails.
 * @retval (error)     If creating the input, renderer, or timing state fails.
 *
 * @code
 * struct cls_window *win;
 * ivec4 bg = {0, 0, 0, 255};
 * cls_window_create(&win, mem_persistant, mem_frame, api,
 *                   (ivec2){1280, 720}, "My Game", true, bg);
 * // Use win.
 * cls_window_destroy(win);
 * @endcode
 */
cls_error cls_window_create(struct cls_window **win,
                            struct cls_mem *mem_persistant,
                            struct cls_mem *mem_frame,
                            struct cls_renderer_api *api, ivec2 size,
                            const char *title, bool vsync,
                            const ivec4 bg_color);

/**
 * @brief Destroys a window.
 *
 * Destroys the window and its renderer.
 *
 * @note Input and timing state are released with the window's memory
 * allocator.
 *
 * @param[in] win Window to destroy.
 */
void cls_window_destroy(struct cls_window *win);

/**
 * @brief Retrieves a window renderer.
 *
 * @param[out] rend Renderer.
 * @param[in]  win  Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `rend` or `win` is NULL.
 */
cls_error cls_window_renderer_get(struct cls_renderer **rend,
                                  struct cls_window *win);

/**
 * @brief Updates a window.
 *
 * Updates the window's timing and input state.
 *
 * @param[out] should_close Whether the window should close.
 * @param[in]  win          Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `should_close` or `win` is NULL.
 * @retval (error)     If updating the timing or input state fails.
 */
cls_error cls_window_update(bool *should_close, struct cls_window *win);

/**
 * @brief Renders a frame.
 *
 * Renders a frame and presents it to the window.
 *
 * @param[in] win Window.
 * @param[in] app Application state.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` or `app` is NULL.
 * @retval (error)     If rendering or presenting the frame fails.
 */
cls_error cls_window_renderer_update(struct cls_window *win,
                                     struct cls_app *app);

/**
 * @brief Checks whether a window should close.
 *
 * @param[out] should_close Whether the window should close.
 * @param[in]  win          Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `should_close` or `win` is NULL.
 */
cls_error cls_window_should_close(bool *should_close,
                                  const struct cls_window *win);

/**
 * @brief Retrieves the window size.
 *
 * @param[out] size Window size.
 * @param[in]  win  Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `size` or `win` is NULL.
 */
cls_error cls_window_size_get(ivec2 size, struct cls_window *win);

/**
 * @brief Retrieves the framebuffer size.
 *
 * @param[out] fb_size Framebuffer size.
 * @param[in]  win     Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `fb_size` or `win` is NULL.
 */
cls_error cls_window_fb_size_get(ivec2 fb_size, struct cls_window *win);

/**
 * @brief Updates the input state.
 *
 * Polls for window events and updates the input state.
 *
 * @param[in] win Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 */
cls_error cls_window_input_update(struct cls_window *win);

/**
 * @brief Records a key event.
 *
 * @param[in] win    Window.
 * @param[in] key    GLFW key code.
 * @param[in] action GLFW_PRESS or GLFW_RELEASE. Other values are ignored.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 * @retval CLS_INVALID_ARG If `key` is out of range.
 */
cls_error cls_window_input_key(struct cls_window *win, int key, int action);

/**
 * @brief Records a mouse button event.
 *
 * @param[in] win    Window.
 * @param[in] button Mouse button index.
 * @param[in] action GLFW_PRESS or GLFW_RELEASE. Other values are ignored.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 * @retval CLS_INVALID_ARG If `button` is out of range.
 */
cls_error cls_window_input_mouse_button_set(struct cls_window *win, int button,
                                            int action);

/**
 * @brief Retrieves the cursor position.
 *
 * @param[out] pos Cursor position.
 * @param[in]  win Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 */
cls_error cls_window_input_cursor_pos_get(vec2 pos, struct cls_window *win);

/**
 * @brief Sets the cursor position.
 *
 * @param[in] win Window.
 * @param[in] pos Cursor position.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 */
cls_error cls_window_input_cursor_pos_set(struct cls_window *win, vec2 pos);

/**
 * @brief Retrieves the scroll offset.
 *
 * Returns the accumulated scroll offset since the previous call.
 *
 * @param[out] offset Scroll offset.
 * @param[in]  win    Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 */
cls_error cls_window_input_scroll_offset_get(vec2 offset,
                                             struct cls_window *win);

/**
 * @brief Sets the scroll offset.
 *
 * @param[in] win    Window.
 * @param[in] offset Scroll offset.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 */
cls_error cls_window_input_scroll_offset_set(struct cls_window *win,
                                             vec2 offset);

/**
 * @brief Checks whether a key was pressed this frame.
 *
 * @param[out] pressed Whether the key was pressed this frame.
 * @param[in]  win     Window.
 * @param[in]  key     GLFW key code.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `pressed` or `win` is NULL.
 */
cls_error cls_window_input_key_pressed(bool *pressed,
                                       const struct cls_window *win, int key);

/**
 * @brief Checks whether a key was released this frame.
 *
 * @param[out] released Whether the key was released this frame.
 * @param[in]  win      Window.
 * @param[in]  key      GLFW key code.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `released` or `win` is NULL.
 */
cls_error cls_window_input_key_released(bool *released,
                                        const struct cls_window *win, int key);

/**
 * @brief Checks whether a key is held down.
 *
 * @param[out] down Whether the key is held down.
 * @param[in]  win  Window.
 * @param[in]  key  GLFW key code.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `down` or `win` is NULL.
 */
cls_error cls_window_input_key_down(bool *down, const struct cls_window *win,
                                    int key);

/**
 * @brief Checks whether a mouse button was pressed this frame.
 *
 * @param[out] pressed Whether the button was pressed this frame.
 * @param[in]  win     Window.
 * @param[in]  button  Mouse button index.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `pressed` or `win` is NULL.
 */
cls_error cls_window_input_mouse_pressed(bool *pressed,
                                         const struct cls_window *win,
                                         int button);

/**
 * @brief Checks whether a mouse button was released this frame.
 *
 * @param[out] released Whether the button was released this frame.
 * @param[in]  win      Window.
 * @param[in]  button   Mouse button index.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `released` or `win` is NULL.
 */
cls_error cls_window_input_mouse_released(bool *released,
                                          const struct cls_window *win,
                                          int button);

/**
 * @brief Checks whether a mouse button is held down.
 *
 * @param[out] down   Whether the button is held down.
 * @param[in]  win    Window.
 * @param[in]  button Mouse button index.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `down` or `win` is NULL.
 */
cls_error cls_window_input_mouse_down(bool *down, const struct cls_window *win,
                                      int button);

/**
 * @brief Updates the timing state.
 *
 * Updates the window's timing information.
 *
 * @param[in] win Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `win` is NULL.
 */
cls_error cls_window_timing_update(struct cls_window *win);

/**
 * @brief Retrieves the frame delta time.
 *
 * @param[out] dt  Delta time in seconds.
 * @param[in]  win Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `dt` or `win` is NULL.
 */
cls_error cls_window_timing_dt_get(float *dt, const struct cls_window *win);

/**
 * @brief Retrieves the current FPS.
 *
 * @param[out] fps Current frames per second.
 * @param[in]  win Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `fps` or `win` is NULL.
 */
cls_error cls_window_timing_fps_get(float *fps, const struct cls_window *win);

/**
 * @brief Retrieves the average FPS.
 *
 * @param[out] fps_avg Average frames per second.
 * @param[in]  win     Window.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `fps_avg` or `win` is NULL.
 */
cls_error cls_window_timing_fps_avg_get(float *fps_avg,
                                        const struct cls_window *win);

/** @} */

#endif // CLS_WINDOW_H
