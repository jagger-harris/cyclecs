#include "core/gfx/renderer.h"
#include "core/ecs/component/renderable/renderable2d.h"
#include "core/ecs/component/renderable/renderable3d.h"
#include "core/ecs/component/renderable/sprite2d.h"
#include "core/ecs/component/transform2d.h"
#include "core/ecs/component/transform3d.h"
#include "core/ecs/world.h"
#include "core/gfx/camera.h"
#include "core/gfx/draw_call2d.h"
#include "core/gfx/draw_call3d.h"
#include "core/util/array.h"
#include "core/util/error.h"
#include <cglm/cam.h>
#include <cglm/vec3.h>

#define RENDERER_DRAW_CALL_START_CAPACITY 128

int renderer_init(struct renderer *out, float aspect_ratio, const api_init init,
                  const api_swap_buffers swap, const api_on_resize resize,
                  const api_draw_frame draw_frame) {
    if (!out || !init || !swap || !resize || !draw_frame)
        return CORE_NULLPTR;

    int status =
        array_init(&out->opaque_draws_2d, RENDERER_DRAW_CALL_START_CAPACITY,
                   sizeof(struct draw_call2d));
    if (status)
        return status;

    status = array_init(&out->transparent_draws_2d,
                        RENDERER_DRAW_CALL_START_CAPACITY,
                        sizeof(struct draw_call2d));
    if (status)
        return status;

    status =
        array_init(&out->opaque_draws_3d, RENDERER_DRAW_CALL_START_CAPACITY,
                   sizeof(struct draw_call3d));
    if (status)
        return status;

    status = array_init(&out->transparent_draws_3d,
                        RENDERER_DRAW_CALL_START_CAPACITY,
                        sizeof(struct draw_call3d));
    if (status)
        return status;

    out->camera = (struct camera){.view = {{0.0F}},
                                  .projection = {{0.0F}},
                                  .pos = {0.0F, 0.0F, -1.0F},
                                  .rot = {0.0F},
                                  .fov = 90.0F,
                                  .near_clip = 0.1F,
                                  .far_clip = 100.0F,
                                  .aspect_ratio = aspect_ratio,
                                  .ortho_size = 1.0F,
                                  .orthographic = true,
                                  .active = true,
                                  .update = true};

    out->init = init;
    out->swap = swap;
    out->resize = resize;
    out->draw_frame = draw_frame;

    return CORE_SUCCESS;
}

void renderer_destroy(struct renderer *in) {
    if (!in)
        return;

    array_destroy(&in->opaque_draws_2d);
    array_destroy(&in->transparent_draws_2d);
    array_destroy(&in->opaque_draws_3d);
    array_destroy(&in->transparent_draws_3d);
}

int renderer_use(const struct renderer *in) {
    if (!in)
        return CORE_INVALID_ARGS;

    int status = CORE_SUCCESS;
    status = in->init();
    if (status)
        return status;

    return status;
}

int renderer_swap_buffers(struct renderer *in, GLFWwindow *window) {
    if (!in)
        return CORE_NULLPTR;

    int status = CORE_SUCCESS;
    status = in->swap(window);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int renderer_resize(struct renderer *in, int width, int height) {
    if (!in)
        return CORE_NULLPTR;

    in->resize(width, height);
    in->camera.aspect_ratio = (float)width / (float)height;
    in->camera.update = true;
    return CORE_SUCCESS;
}

int renderer_camera_update(struct renderer *in) {
    if (!in)
        return CORE_SUCCESS;

    struct camera *cam = &in->camera;
    vec3 forward = {0.0F};
    vec3 target = {0.0F};
    vec3 up = {0.0F, 1.0F, 0.0F};
    float pitch = glm_rad(cam->rot[0]);
    float yaw = glm_rad(cam->rot[1]);

    forward[0] = cosf(pitch) * sinf(yaw);
    forward[1] = sinf(pitch);
    forward[2] = cosf(pitch);

    glm_vec3_add(cam->pos, forward, target);
    glm_lookat(cam->pos, target, up, cam->view);

    if (cam->orthographic) {
        float top = cam->ortho_size;
        float bottom = -top;
        float right = top * cam->aspect_ratio;
        float left = -right;

        glm_ortho(left, right, bottom, top, cam->near_clip, cam->far_clip,
                  cam->projection);
    } else {
        glm_perspective(cam->fov, cam->aspect_ratio, cam->near_clip,
                        cam->far_clip, cam->projection);
    }

    return CORE_SUCCESS;
}

int renderer_camera_set_pos(struct renderer *in, float pos_x, float pos_y,
                            float pos_z) {
    if (!in)
        return CORE_NULLPTR;

    in->camera.pos[0] = pos_x;
    in->camera.pos[1] = pos_y;
    in->camera.pos[2] = pos_z;
    in->camera.update = true;
    return CORE_SUCCESS;
}

int renderer_camera_move(struct renderer *in, float pos_x, float pos_y,
                         float pos_z) {
    if (!in)
        return CORE_NULLPTR;

    renderer_camera_set_pos(in, pos_x + in->camera.pos[0],
                            pos_y + in->camera.pos[1],
                            pos_z + in->camera.pos[2]);

    return CORE_SUCCESS;
}

static int compare_draw_z_index(const void *a, const void *b) {
    const struct transform2d *transform_a = a;
    const struct transform2d *transform_b = b;

    if (transform_a->z_index < transform_b->z_index)
        return 1;
    if (transform_a->z_index > transform_b->z_index)
        return -1;
    return 0;
}

static int compare_draw_camera_distance(const void *a, const void *b) {
    const struct transform3d *transform_a = a;
    const struct transform3d *transform_b = b;

    if (transform_a->camera_distance < transform_b->camera_distance)
        return 1;
    if (transform_a->camera_distance > transform_b->camera_distance)
        return -1;
    return 0;
}

struct component_context {
    struct renderer *renderer;
    struct ecs_world *world;
    struct assets *assets;
};

typedef int (*add_draw_calls_fn)(struct component_context *ctx);

static int add_draw_calls_from_entities(struct component_context *ctx,
                                        struct array *entities,
                                        enum draw_call2d_render_type draw_type,
                                        const char *component_type_name) {
    u64 entity_current = UINT64_MAX;
    void *component_current = NULL;
    struct renderable2d *renderable2d_current = NULL;
    struct transform2d *transform2d_current = NULL;

    for (size_t i = 0; i < entities->length; ++i) {
        array_get_cpy(&entity_current, entities, i);

        int status = ecs_world_query_data(&component_current, ctx->world,
                                          entity_current, component_type_name);
        if (status)
            return status;

        status =
            ecs_world_query_data((void **)&renderable2d_current, ctx->world,
                                 entity_current, "renderable2d");
        if (status)
            return status;

        status = ecs_world_query_data((void **)&transform2d_current, ctx->world,
                                      entity_current, "transform2d");
        if (status)
            return status;

        struct draw_call2d call = {0};
        status = draw_call2d_init(&call, draw_type, component_current,
                                  renderable2d_current, transform2d_current);
        if (status)
            return status;

        if (!call.renderable->visible)
            continue;

        if (call.renderable->opacity < 1.0F) {
            status = array_push(&ctx->renderer->transparent_draws_2d, &call);
        } else {
            status = array_push(&ctx->renderer->opaque_draws_2d, &call);
        }
        if (status)
            return status;
    }

    return CORE_SUCCESS;
}

static int add_draw_calls_from_component(struct component_context *ctx,
                                         enum draw_call2d_render_type draw_type,
                                         const char *component_type_name) {
    struct array *entities = NULL;
    int status = ecs_world_query_all_entities(&entities, ctx->world,
                                              component_type_name);
    if (status)
        return status;

    return add_draw_calls_from_entities(ctx, entities, draw_type,
                                        component_type_name);
}

static int add_draw_calls_sprite2d(struct component_context *ctx) {
    return add_draw_calls_from_component(ctx, SPRITE2D, "sprite2d");
}

static int add_draw_calls_mesh2d(struct component_context *ctx) {
    return add_draw_calls_from_component(ctx, MESH2D, "mesh2d");
}

static add_draw_calls_fn component_add_draw_calls_functions[] = {
    add_draw_calls_sprite2d, add_draw_calls_mesh2d, NULL};

int renderer_draw_frame(struct renderer *in, struct assets *assets,
                        struct ecs_world *world) {
    if (!in || !assets || !world)
        return CORE_NULLPTR;

    if (in->camera.update) {
        renderer_camera_update(in);
        in->camera.update = false;
    }

    array_clear(&in->opaque_draws_2d);
    array_clear(&in->transparent_draws_2d);
    array_clear(&in->opaque_draws_3d);
    array_clear(&in->transparent_draws_3d);

    struct component_context ctx = {
        .renderer = in, .world = world, .assets = assets};

    for (int i = 0; component_add_draw_calls_functions[i] != NULL; ++i) {
        int status = component_add_draw_calls_functions[i](&ctx);
        if (status)
            return status;
    }

    if (in->transparent_draws_2d.length > 0)
        qsort(in->transparent_draws_2d.data, in->transparent_draws_2d.length,
              sizeof(struct draw_call2d), compare_draw_z_index);

    if (in->transparent_draws_3d.length > 0)
        qsort(in->transparent_draws_3d.data, in->transparent_draws_3d.length,
              sizeof(struct draw_call3d), compare_draw_camera_distance);

    int status = in->draw_frame(assets, &in->camera, &in->opaque_draws_2d,
                                &in->transparent_draws_2d, &in->opaque_draws_3d,
                                &in->transparent_draws_3d);
    if (status)
        return status;

    return CORE_SUCCESS;
}
