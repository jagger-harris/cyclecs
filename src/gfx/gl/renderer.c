#include <GLFW/glfw3.h>
#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/gfx/batch.h>
#include <cls/gfx/cmd.h>
#include <cls/gfx/gl/mesh.h>
#include <cls/gfx/shader.h>
#include <cls/util/allocator.h>
#include <cls/util/array.h>
#include <cls/util/logger.h>

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
        enum logger_level level = severity == GL_DEBUG_SEVERITY_HIGH
                                      ? CLS_LOGGER_ERROR
                                      : CLS_LOGGER_WARN;
        CLS_LOGGER_LOG(level, "GL 0x%x: %s\n", type, message);
    }
}
#endif

int cls_gl_renderer_init(ivec4 bg_color) {
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
        return CLS_GL;

#if DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(msg_callback, 0);
#endif
    glClearColor((float)bg_color[0] / 255.0f, (float)bg_color[1] / 255.0f,
                 (float)bg_color[2] / 255.0f, (float)bg_color[3] / 255.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return CLS_SUCCESS;
}

int cls_gl_renderer_swap_buffers(GLFWwindow *win) {
    if (!win)
        return CLS_NULLPTR;

    glfwSwapBuffers(win);
    return CLS_SUCCESS;
}

void cls_gl_renderer_on_resize(int width, int height) {
    glViewport(0, 0, width, height);
}

static int render_cmd(struct cls_app *app, struct cls_renderer_cmd *cmd) {
    struct cls_gl_mesh *mesh = NULL;
    int error = cls_assets_mesh_get(&mesh, app->assets, cmd->ren->mesh_id);
    if (error)
        return error;

    struct cls_shader *shader = NULL;
    error = cls_assets_shader_get(&shader, app->assets, cmd->ren->shader_id);
    if (error)
        return error;

    struct cls_texture2d *texture = NULL;
    error =
        cls_assets_texture2d_get(&texture, app->assets, cmd->ren->texture_id);
    if (error)
        return error;

    error = cls_gl_shader_use(shader);
    if (error)
        return error;

    error = cls_gl_texture2d_use(texture);
    if (error)
        return error;

    vec4 tint = {
        (float)cmd->ren->tint[0] / 255.0f, (float)cmd->ren->tint[1] / 255.0f,
        (float)cmd->ren->tint[2] / 255.0f, (float)cmd->ren->tint[3] / 255.0f};

    cls_gl_shader_set_mat4(shader->gl.id, "u_mvp", &cmd->mvp);
    cls_gl_shader_set_vec4(shader->gl.id, "u_tint", &tint);
    cls_gl_shader_set_vec2(shader->gl.id, "u_uv_offset", &cmd->ren->uv_offset);
    cls_gl_shader_set_vec2(shader->gl.id, "u_uv_scale", &cmd->ren->uv_scale);
    cls_gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_FALSE);
    cls_gl_mesh_draw(mesh);

    return CLS_SUCCESS;
}

static int render_batch(struct cls_app *app, struct cls_renderer_batch *batch) {
    struct cls_gl_mesh *mesh = NULL;
    int error = cls_assets_mesh_get(&mesh, app->assets, batch->data.mesh_id);
    if (error)
        return error;

    struct cls_shader *shader = NULL;
    error = cls_assets_shader_get(&shader, app->assets, batch->data.shader_id);
    if (error)
        return error;

    struct cls_texture2d *texture = NULL;
    error =
        cls_assets_texture2d_get(&texture, app->assets, batch->data.texture_id);
    if (error)
        return error;

    error = cls_gl_shader_use(shader);
    if (error)
        return error;

    error = cls_gl_texture2d_use(texture);
    if (error)
        return error;

    size_t cmds_length = 0;
    error = cls_array_length_get(&cmds_length, batch->cmds);
    if (error)
        return error;

    if (cmds_length > 1) {
        size_t instance_count = cmds_length;
        if (instance_count == 0)
            return CLS_SUCCESS;

        struct cls_gl_mesh_instance_data *instances = NULL;
        error = cls_allocator_alloc((void **)&instances, app->alloc_frame,
                                    sizeof(struct cls_gl_mesh_instance_data) *
                                        cmds_length,
                                    alignof(struct cls_gl_mesh_instance_data));
        if (error)
            return error;

        for (size_t i = 0; i < instance_count; ++i) {
            void *cmd_ptr = NULL;
            error = cls_array_elem_get(&cmd_ptr, batch->cmds, i);
            if (error)
                continue;

            if (!cmd_ptr)
                continue;

            struct cls_renderer_cmd *cmd = *(struct cls_renderer_cmd **)cmd_ptr;
            if (!cmd)
                continue;

            glm_mat4_copy(cmd->mvp, instances[i].mvp);
            glm_vec4_copy((vec4){(float)cmd->ren->tint[0] / 255.0f,
                                 (float)cmd->ren->tint[1] / 255.0f,
                                 (float)cmd->ren->tint[2] / 255.0f,
                                 (float)cmd->ren->tint[3] / 255.0f},
                          instances[i].tint);
            glm_vec2_copy(cmd->ren->uv_offset, instances[i].uv_offset);
            glm_vec2_copy(cmd->ren->uv_scale, instances[i].uv_scale);
        }

        cls_gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_TRUE);
        cls_gl_mesh_draw_instanced((struct cls_gl_mesh *)mesh, instances,
                                   (GLsizei)instance_count);
    } else {
        for (size_t i = 0; i < cmds_length; ++i) {
            void *cmd_ptr = NULL;
            error = cls_array_elem_get(&cmd_ptr, batch->cmds, i);
            if (error)
                continue;

            if (!cmd_ptr)
                continue;

            struct cls_renderer_cmd *cmd = *(struct cls_renderer_cmd **)cmd_ptr;
            if (!cmd)
                continue;

            vec4 tint = {(float)cmd->ren->tint[0] / 255.0f,
                         (float)cmd->ren->tint[1] / 255.0f,
                         (float)cmd->ren->tint[2] / 255.0f,
                         (float)cmd->ren->tint[3] / 255.0f};

            cls_gl_shader_set_mat4(shader->gl.id, "u_mvp", &cmd->mvp);
            cls_gl_shader_set_vec4(shader->gl.id, "u_tint", &tint);
            cls_gl_shader_set_vec2(shader->gl.id, "u_uv_offset",
                                   &cmd->ren->uv_offset);
            cls_gl_shader_set_vec2(shader->gl.id, "u_uv_scale",
                                   &cmd->ren->uv_scale);
            cls_gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_FALSE);
            cls_gl_mesh_draw(mesh);
        }
    }

    return CLS_SUCCESS;
}

int cls_gl_renderer_draw_frame(struct cls_app *app,
                               struct cls_array *transparent_cmds,
                               struct cls_array *batches) {
    if (!app || !batches)
        return CLS_NULLPTR;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    size_t batches_length = 0;
    int error = cls_array_length_get(&batches_length, batches);
    if (error)
        return error;

    for (size_t i = 0; i < batches_length; ++i) {
        struct cls_renderer_batch *batch = NULL;
        error = cls_array_elem_get((void **)&batch, batches, i);
        if (error) {
            CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                                 "Getting render batch failed");
            continue;
        }

        error = render_batch(app, batch);
        if (error) {
            CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                                 "GL rendering render batch failed");
            continue;
        }
    }

    size_t transparent_length = 0;
    error = cls_array_length_get(&transparent_length, transparent_cmds);
    if (error)
        return error;

    if (transparent_length > 0) {
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        for (size_t i = 0; i < transparent_length; ++i) {
            void *cmd_ptr = NULL;
            error = cls_array_elem_get(&cmd_ptr, transparent_cmds, i);
            if (error)
                continue;

            if (!cmd_ptr)
                continue;

            struct cls_renderer_cmd *cmd = *(struct cls_renderer_cmd **)cmd_ptr;
            if (!cmd)
                continue;

            error = render_cmd(app, cmd);
            if (error) {
                CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                                     "GL rendering transparent cmd failed");
                continue;
            }
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    return CLS_SUCCESS;
}
