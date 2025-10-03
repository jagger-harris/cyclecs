#include "core/gfx/gl/renderer.h"
#include "core/app/app.h"
#include "core/app/assets.h"
#include "core/app/window.h"
#include "core/gfx/gl/mesh.h"
#include "core/gfx/gl/shader.h"
#include "core/gfx/gl/texture2d.h"
#include "core/gfx/renderer/batch.h"
#include "core/gfx/renderer/camera.h"
#include "core/gfx/renderer/cmd.h"
#include "core/gfx/renderer/renderer.h"
#include "core/gfx/shader.h"
#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include <GLFW/glfw3.h>
#include <cglm/affine-pre.h>
#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/mat4.h>
#include <cglm/types.h>
#include <cglm/vec2.h>
#include <cglm/vec4.h>
#include <glad/gl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct gl_renderer_ctx {
    mat4 mvp;
    struct gl_mesh *mesh;
};

typedef int (*draw_fn)(struct gl_renderer_ctx *ctx, struct renderer_cmd *cmd);

#if DEBUG
void GLAPIENTRY msg_callback(GLenum source, GLenum type, GLuint id,
                             GLenum severity, GLsizei length,
                             const GLchar *message, const void *user) {
    (void)source;
    (void)id;
    (void)length;
    (void)user;

    if (severity == GL_DEBUG_SEVERITY_MEDIUM ||
        severity == GL_DEBUG_SEVERITY_HIGH) {
        enum logger_level level =
            severity == GL_DEBUG_SEVERITY_HIGH ? LOGGER_ERROR : LOGGER_WARN;
        LOGGER_LOG(level, "GL 0x%x: %s\n", type, message);
    }
}
#endif

int gl_renderer_init(void) {
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
        return CORE_GL;

#if DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(msg_callback, 0);
#endif

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

static int render_batch(struct gl_renderer_ctx *gl_ctx, struct app *app,
                        struct renderer_batch *batch) {
    struct gl_mesh *mesh = NULL;
    int error = assets_mesh_get(&mesh, &app->assets, batch->mesh_id);
    if (error)
        return error;

    struct shader *shader = NULL;
    error = assets_shader_get(&shader, &app->assets, batch->shader_id);
    if (error)
        return error;

    struct texture2d *texture = NULL;
    error = assets_texture2d_get(&texture, &app->assets, batch->texture_id);
    if (error)
        return error;

    error = gl_shader_use(shader);
    if (error)
        return error;

    error = gl_texture2d_use(texture);
    if (error)
        return error;

    gl_ctx->mesh = mesh;

    struct renderer_camera *active_camera = NULL;
    error = renderer_active_camera_get(&active_camera, &app->window.renderer);
    if (error)
        return error;

    // gl_shader_set_bool(shader->gl.id, "sdf", GL_TRUE);
    // gl_shader_set_float(shader->gl.id, "sdf_edge", 0.001f);
    batch->transparent ? glEnable(GL_BLEND) : glDisable(GL_BLEND);

    size_t cmds_length = batch->cmds.length;
    if (batch->instanced) {
        GLsizei instance_count = (GLsizei)cmds_length;
        if (instance_count == 0)
            return CORE_SUCCESS;

        // TODO: Use memory arena
        struct gl_mesh_instance_data *instances =
            malloc(sizeof(struct gl_mesh_instance_data) * instance_count);
        if (!instances)
            return CORE_OUT_OF_MEMORY;

        for (GLsizei i = 0; i < instance_count; ++i) {
            struct renderer_cmd *cmd = NULL;
            array_get((void **)&cmd, &batch->cmds, i);

            glm_mat4_identity(cmd->model);
            glm_translate(cmd->model, cmd->transform.pos);
            glm_rotate_at(cmd->model, cmd->transform.origin,
                          cmd->transform.rot_angle, cmd->transform.rot_axis);
            glm_scale(cmd->model, cmd->transform.scale);

            glm_mat4_identity(gl_ctx->mvp);
            if (cmd->is_in_world) {
                glm_mat4_mul(gl_ctx->mvp, active_camera->projection,
                             gl_ctx->mvp);
                glm_mat4_mul(gl_ctx->mvp, active_camera->view, gl_ctx->mvp);
                glm_mat4_mul(gl_ctx->mvp, cmd->model, gl_ctx->mvp);
            } else {
                mat4 ortho = {{0.0f}};
                glm_ortho(0.0f, (float)app->window.size[0],
                          (float)app->window.size[1], 0.0f, -1.0f, 1.0f, ortho);
                glm_mat4_mul(gl_ctx->mvp, ortho, gl_ctx->mvp);
                glm_mat4_mul(gl_ctx->mvp, cmd->model, gl_ctx->mvp);
            }

            glm_mat4_copy(gl_ctx->mvp, instances[i].mvp);
            glm_vec4_copy((vec4){(float)cmd->tint.r / 255.0f,
                                 (float)cmd->tint.g / 255.0f,
                                 (float)cmd->tint.b / 255.0f,
                                 (float)cmd->tint.a / 255.0f},
                          instances[i].tint);
            glm_vec2_copy(cmd->uv_offset, instances[i].uv_offset);
            glm_vec2_copy(cmd->uv_scale, instances[i].uv_scale);
        }

        gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_TRUE);
        gl_mesh_draw_instanced(gl_ctx->mesh, instances, instance_count);
        free(instances);
    } else {
        for (size_t i = 0; i < cmds_length; ++i) {
            struct renderer_cmd *cmd = NULL;
            error = array_get((void **)&cmd, &batch->cmds, i);
            if (error)
                continue;

            glm_mat4_identity(cmd->model);
            glm_translate(cmd->model, cmd->transform.pos);
            glm_rotate_at(cmd->model, cmd->transform.origin,
                          cmd->transform.rot_angle, cmd->transform.rot_axis);
            glm_scale(cmd->model, cmd->transform.scale);

            glm_mat4_identity(gl_ctx->mvp);
            if (cmd->is_in_world) {
                glm_mat4_mul(gl_ctx->mvp, active_camera->projection,
                             gl_ctx->mvp);
                glm_mat4_mul(gl_ctx->mvp, active_camera->view, gl_ctx->mvp);
                glm_mat4_mul(gl_ctx->mvp, cmd->model, gl_ctx->mvp);
            } else {
                mat4 ortho = {{0.0f}};
                glm_ortho(0.0f, (float)app->window.size[0],
                          (float)app->window.size[1], 0.0f, -1.0f, 1.0f, ortho);
                glm_mat4_mul(gl_ctx->mvp, ortho, gl_ctx->mvp);
                glm_mat4_mul(gl_ctx->mvp, cmd->model, gl_ctx->mvp);
            }

            vec4 tint = {
                (float)cmd->tint.r / 255.0f, (float)cmd->tint.g / 255.0f,
                (float)cmd->tint.b / 255.0f, (float)cmd->tint.a / 255.0f};

            gl_shader_set_mat4(shader->gl.id, "u_mvp", &gl_ctx->mvp);
            gl_shader_set_vec4(shader->gl.id, "u_tint", &tint);
            gl_shader_set_vec2(shader->gl.id, "u_uv_offset", &cmd->uv_offset);
            gl_shader_set_vec2(shader->gl.id, "u_uv_scale", &cmd->uv_scale);
            gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_FALSE);
            gl_mesh_draw(mesh);
        }
    }

    return CORE_SUCCESS;
}

int gl_renderer_draw_frame(struct app *app, struct color bg_color,
                           struct array *batches) {
    glClearColor((float)bg_color.r / 255.0f, (float)bg_color.g / 255.0f,
                 (float)bg_color.b / 255.0f, (float)bg_color.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    struct gl_renderer_ctx gl_ctx = {0};

    for (size_t i = 0; i < batches->length; ++i) {
        struct renderer_batch *batch = NULL;
        int error = array_get((void **)&batch, batches, i);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Getting render batch failed");
            continue;
        }

        error = render_batch(&gl_ctx, app, batch);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "GL rendering render batch failed");
            continue;
        }
    }

    return CORE_SUCCESS;
}
