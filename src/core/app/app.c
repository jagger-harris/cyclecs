#include "core/app/app.h"
#include "core/ecs/ecs.h"
#include "core/gfx/gl/renderer.h"
#include "core/gfx/renderer.h"
#include "core/util/logger.h"
#include "window.h"
#include <stdio.h>

struct app {
    assets *assets;
    ecs *ecs;
    renderer *renderer;
    window *window;
};

struct comp_test {
    int a;
    int b;
    int c;
};

int system_test_fn(ecs *in, ecs_ctx *ctx) {
    err err = CORE_SUCCESS;

    array *comps = NULL;
    err = ecs_comps_get(&comps, in, 0);
    if (err)
        goto err;

    if (!ctx) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    size_t length = 0;
    array_length(&length, comps);

    for (size_t i = 0; i < length; ++i) {
        struct comp_test *element = NULL;
        err = array_at((void **)&element, comps, i);

        printf("Entity:\n");
        printf("    a: %i\n", element->a);
        printf("    b: %i\n", element->b);
        printf("    c: %i\n", element->c);

        element->a = element->a + 100;
        element->b = element->b + 5;
        element->c = element->c - 5;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to perform this system", err);
    return err;
}

err app_new(app **out, arena *mem, int width, int height, const char *title) {
    err err = CORE_SUCCESS;

    if (!out || !mem || !title) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = arena_alloc((void **)out, mem, sizeof(app), _Alignof(app));
    if (err)
        goto err;

    /* TODO: Add checking vsync through an options file */
    int vsync = 1;

    err = ecs_new(&(*out)->ecs, mem);
    if (err)
        goto err;

    ecs_comp_type comp_test = 0;
    err = ecs_comp_type_add(&comp_test, (*out)->ecs, sizeof(struct comp_test));

    ecs_system_type system_test = 0;
    err = ecs_system_add(&system_test, (*out)->ecs, system_test_fn);
    if (err)
        goto err;

    ecs_entity entity = 0;
    err = ecs_entity_add(&entity, (*out)->ecs);
    if (err)
        goto err;

    struct comp_test comp = {
        .a = 1,
        .b = 5,
        .c = -20,
    };

    err = ecs_comp_add((*out)->ecs, entity, comp_test, &comp);

    err = assets_new(&(*out)->assets, mem);
    if (err)
        goto err;

    err = window_new(&(*out)->window, mem, width, height, title, vsync);
    if (err)
        goto err;

    /* TODO: Add support for multiple apis through an options file */
    err = renderer_new(&(*out)->renderer, mem, gl_renderer_init,
                       gl_renderer_swap_buffers, gl_renderer_on_resize,
                       gl_renderer_render_frame);
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new app", err);
    return err;
}

err app_delete(app *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    ecs_delete(in->ecs);
    assets_delete(in->assets);
    window_delete(in->window);

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete app", err);
    return err;
}

err app_run(app *in, game_new game, game_update update) {
    err err = CORE_SUCCESS;

    if (!in || !game || !update) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    GLFWwindow *current_native_window = NULL;
    if (!current_native_window) {
        err = window_get_native_window(&current_native_window, in->window);
        if (err)
            goto err;
    }

    err = renderer_use(in->renderer);
    if (err)
        goto err;

    err = window_set_renderer(in->window, in->renderer);
    if (err)
        goto err;

    err = game(in);
    if (err)
        goto err;

    int should_close = 0;
    while (!should_close) {
        err = window_should_close(&should_close, in->window);
        if (err)
            goto err;

        glfwPollEvents();

        ecs_ctx ctx = {.assets = in->assets,
                       .renderer = in->renderer,
                       .window = in->window};

        err = ecs_update(in->ecs, &ctx);
        if (err)
            goto err;

        err = update();
        if (err)
            goto err;

        err = renderer_render_frame(in->renderer);
        if (err)
            goto err;

        err = renderer_swap_buffers(in->renderer, current_native_window);
        if (err)
            goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to run app", err);
    return err;
}

err app_get_assets(assets **out, app *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = in->assets;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get assets", err);
    return err;
}
