#include "core/app/app.h"
#include "core/gfx/gl/renderer.h"
#include "core/gfx/gl/shader.h"
#include "core/util/error.h"
#include "core/util/logger.h"

int app_init(struct app *out, void *game_state, ivec2 window_size,
             const char *title, ivec4 bg_color) {
    if (!out || !title)
        return CORE_NULLPTR;

    *out = (struct app){
        // TODO: Add renderer api type via an options file
        .api = (struct gfx_api){.type = GL,
                                .init = gl_renderer_init,
                                .swap_buffers = gl_renderer_swap_buffers,
                                .on_resize = gl_renderer_on_resize,
                                .draw_frame = gl_renderer_draw_frame,
                                .shader_init = gl_shader_init,
                                .shader_destroy = gl_shader_destroy,
                                .shader_use = gl_shader_use,
                                .texture2d_init = gl_texture2d_init,
                                .texture2d_destroy = gl_texture2d_destroy,
                                .texture2d_use = gl_texture2d_use},
        .game_state = game_state};

    // TODO: Add checking vsync via an options file
    bool vsync = false;
    int error = window_init(&out->window, &out->api, window_size, title, vsync,
                            bg_color);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init app window failed");
        goto cleanup;
    }

    error = assets_init(&out->assets, &out->api);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init app assets failed");
        goto cleanup;
    }

    error = ecs_init(&out->ecs);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init app ecs failed");
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

    assets_destroy(&in->assets);
    ecs_destroy(&in->ecs);
    window_destroy(&in->window);
}

int app_run(struct app *in, const game_update_fn game_update) {
    if (!in || !game_update)
        return CORE_NULLPTR;

    bool close = false;
    int error = window_should_close(&close, &in->window);
    if (error)
        return error;

    while (!close) {
        error = window_should_close(&close, &in->window);
        if (error)
            break;

        error = timing_update(&in->window.timing);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Updating window time failed");
            break;
        }

        error = input_update(&in->window.input);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Updating window input failed");
            break;
        }

        error = game_update(in);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Updating game failed");
            break;
        }

        error = ecs_update_all_worlds(&in->ecs, in);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Updating all ecs worlds failed");
            break;
        }

        // PROFILING_START(renderer_draw_frame);
        error = renderer_draw_frame(&in->window.renderer, in);
        // PROFILING_END(renderer_draw_frame);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Renderer drawing frame failed");
            break;
        }

        error =
            renderer_swap_buffers(&in->window.renderer, in->window.glfw_window);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Renderer swapping buffers failed");
            break;
        }
    }

    return error;
}
