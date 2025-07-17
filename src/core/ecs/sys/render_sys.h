#ifndef ECS_SYS_RENDER_H
#define ECS_SYS_RENDER_H

#include "base/game/constants.h"
#include "core/ecs/ecs.h"
#include "core/gfx/draw_call.h"
#include "core/gfx/renderer.h"
#include "core/util/err.h"
#include "core/util/logger.h"
#include <string.h>

err render_sys(const struct ecs *in, void *ctx) {
    err status = CORE_SUCCESS;
    struct ecs_handles *handles = (struct ecs_handles *)in->handles;
    struct renderer *renderer_ctx = (struct renderer *)ctx;

    if (!ctx) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct array *tfs = NULL;
    status = ecs_comps_get(&tfs, in, handles->transformable);
    if (status)
        goto err;

    struct array *draws = NULL;
    status = ecs_comps_get(&draws, in, handles->drawable);
    if (status)
        goto err;

    for (size_t i = 0; i < draws->length; ++i) {
        struct draw_call *draw = NULL;
        struct transform_comp *tf = NULL;
        array_get_ptr((void **)&draw, draws, i);
        array_get_ptr((void **)&tf, tfs, i);
        memcpy(&draw->tf, tf, sizeof(struct transform_comp));

        status = renderer_draw_call_add(renderer_ctx, draw);
        if (status)
            goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Running render sys failed");
    return status;
}

#endif // ECS_SYS_RENDER_H
