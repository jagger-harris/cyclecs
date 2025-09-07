#include "core/gfx/gl/renderer.h"
#include "cglm/affine-pre.h"
#include "core/app/assets.h"
#include "core/ecs/component/renderable/sprite2d.h"
#include "core/ecs/component/transform2d.h"
#include "core/gfx/draw_call2d.h"
#include "core/gfx/gl/mesh.h"
#include "core/gfx/gl/shader.h"
#include "core/gfx/gl/texture2d.h"
#include "core/util/error.h"
#include <cglm/cglm.h>
#include <glad/gl.h>
#include <string.h>

struct render_context {
    struct assets *assets;
    struct camera *camera;
    GLuint shader;
    GLuint texture;
    mat4 mvp;
    mat4 model;
};

typedef int (*draw_fn)(struct render_context *ctx,
                       struct draw_call2d *draw_call);

int gl_renderer_init(void) {
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
        return CORE_GL;

    return CORE_SUCCESS;
}

int gl_renderer_swap_buffers(GLFWwindow *window) {
    if (!window)
        return CORE_NULLPTR;

    glfwSwapBuffers(window);
    return CORE_SUCCESS;
}

void gl_renderer_on_resize(int width, int height) {
    glViewport(0, 0, width, height);
}

static int render_mesh2d(struct render_context *ctx,
                         struct draw_call2d *draw_call) {
    if (!ctx || !draw_call)
        return CORE_NULLPTR;

    return CORE_SUCCESS;
}

static int render_sprite2d(struct render_context *ctx,
                           struct draw_call2d *draw_call) {
    if (!ctx || !draw_call)
        return CORE_NULLPTR;

    struct sprite2d *sprite = draw_call->component;

    assets_shader_get(&ctx->shader, ctx->assets, "sprite2d");
    assets_texture_get(&ctx->texture, ctx->assets, sprite->texture_path);
    gl_shader_use(ctx->shader);
    gl_texture2d_use(ctx->texture);

    vec3 new_scale = {draw_call->transform->scale[0],
                      draw_call->transform->scale[1], 1};
    vec3 new_pos = {draw_call->transform->pos[0], draw_call->transform->pos[1],
                    draw_call->transform->z_index};
    vec3 rotation_pivot = {0.0F, 0.0F, 0.0F};
    vec3 rotation_axis_2d = {0.0F, 0.0F, 1.0F};

    glm_mat4_identity(ctx->model);
    glm_scale(ctx->model, new_scale);
    glm_rotate_at(ctx->model, rotation_pivot, draw_call->transform->rot_angle,
                  rotation_axis_2d);
    glm_rotate(ctx->model, draw_call->transform->rot_angle, rotation_axis_2d);
    glm_translate(ctx->model, new_pos);

    glm_mat4_identity(ctx->mvp);
    glm_mat4_mul(ctx->mvp, ctx->camera->projection, ctx->mvp);
    glm_mat4_mul(ctx->mvp, ctx->camera->view, ctx->mvp);
    glm_mat4_mul(ctx->mvp, ctx->model, ctx->mvp);

    gl_shader_set_mat4(ctx->shader, "mvp", &ctx->mvp);

    struct gl_mesh *mesh = NULL;
    assets_mesh_get(&mesh, ctx->assets, "sprite2d");
    gl_mesh_draw(mesh);
    return CORE_SUCCESS;
}

static draw_fn draw_functions[] = {
    [MESH2D] = render_mesh2d, [SPRITE2D] = render_sprite2d};

static int render_draw_calls(struct render_context *ctx,
                             struct array *draw_calls) {
    for (size_t i = 0; i < draw_calls->length; ++i) {
        struct draw_call2d *draw_call = NULL;
        array_get((void **)&draw_call, draw_calls, i);
        if (!draw_call || draw_call->type >= DRAW_CALL2D_RENDER_TYPE_COUNT)
            continue;

        draw_fn draw = draw_functions[draw_call->type];
        if (draw)
            draw(ctx, draw_call);
    }

    return CORE_SUCCESS;
}

int gl_renderer_draw_frame(struct assets *assets, struct camera *camera,
                           struct array *opaque_draws_2d,
                           struct array *transparent_draws_2d,
                           struct array *opaque_draws_3d,
                           struct array *transparent_draws_3d) {
    glClearColor(0.2F, 0.3F, 0.4F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    struct render_context ctx = {
        .assets = assets, .camera = camera, .shader = 0, .texture = 0};

    glm_mat4_identity(ctx.mvp);
    glm_mat4_identity(ctx.model);

    render_draw_calls(&ctx, opaque_draws_3d);
    render_draw_calls(&ctx, transparent_draws_3d);
    render_draw_calls(&ctx, opaque_draws_2d);
    render_draw_calls(&ctx, transparent_draws_2d);
    return CORE_SUCCESS;
}
