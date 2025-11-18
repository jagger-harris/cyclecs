#include "base/game/game.h"
#include "core/app/app.h"
#include "core/gfx/api.h"
#include "core/gfx/gl/renderer.h"
#include "core/gfx/gl/shader.h"
#include "core/gfx/gl/texture2d.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include "core/util/mem.h"

#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_TITLE "C ECS OpenGL Game Engine"

int main(void) {
    struct gfx_api api =
        (struct gfx_api){.type = GL,
                         .init = gl_renderer_init,
                         .swap_buffers = gl_renderer_swap_buffers,
                         .on_resize = gl_renderer_on_resize,
                         .draw_frame = gl_renderer_draw_frame,
                         .shader_init = gl_shader_init,
                         .shader_destroy = gl_shader_destroy,
                         .shader_use = gl_shader_use,
                         .texture2d_init = gl_texture2d_init,
                         .texture2d_destroy = gl_texture2d_destroy,
                         .texture2d_use = gl_texture2d_use};
    struct game_state state = {0};
    struct app app = {0};
    int error = app_init(&app, &api, &state, (ivec2){WIN_WIDTH, WIN_HEIGHT},
                         WIN_TITLE, (ivec4){5, 10, 20, 255});
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init app failed");
        goto cleanup;
    }

    error = game_init(&state, &app);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init game failed");
        goto cleanup;
    }

    error = app_run(&app, game_update);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Running app failed");
        goto cleanup;
    }

    app_destroy(&app);
    return CORE_SUCCESS;

cleanup:
    LOGGER_LOG_ERROR(LOGGER_FATAL, error, "%s", "Fatal error");
    app_destroy(&app);
    return error;
}
