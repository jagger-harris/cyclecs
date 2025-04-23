#include "core/ecs/comp/render.h"
#include "core/ecs/ecs.h"
#include "core/ecs/ecs_ctx.h"
#include "core/gfx/renderer.h"
#include "core/util/err.h"
#include "core/util/logger.h"

err render_system(ecs *in, ecs_ctx *ctx) {
    err err = CORE_SUCCESS;
    array *renders = NULL;

    if (!ctx) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = ecs_comps_get(&renders, in, 0);
    if (err)
        goto err;

    size_t count = 0;
    array_length(&count, renders);
    for (size_t i = 0; i < count; ++i) {
        struct render *render = NULL;
        array_at((void **)&render, renders, i);
        renderer_draw_enqueue(ctx->renderer, &render->material,
                              render->transform);
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to run render system", err);
    return err;
}
