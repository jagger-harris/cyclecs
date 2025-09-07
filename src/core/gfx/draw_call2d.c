#include "core/gfx/draw_call2d.h"
#include "core/util/error.h"

int draw_call2d_init(struct draw_call2d *out, enum draw_call2d_render_type type,
                     void *component, struct renderable2d *renderable,
                     struct transform2d *transform) {
    if (!out || !component || !renderable || !transform)
        return CORE_NULLPTR;

    if (type >= DRAW_CALL2D_RENDER_TYPE_COUNT)
        return CORE_INVALID_ARGS;

    *out = ((struct draw_call2d){.type = type,
                                 .component = component,
                                 .renderable = renderable,
                                 .transform = transform});
    return CORE_SUCCESS;
}
