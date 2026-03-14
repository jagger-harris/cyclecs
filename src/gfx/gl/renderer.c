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
        enum logger_level level =
            severity == GL_DEBUG_SEVERITY_HIGH ? LOGGER_ERROR : LOGGER_WARN;
        LOGGER_LOG(level, "GL 0x%x: %s\n", type, message);
    }
}
#endif

int gl_renderer_init(ivec4 bg_color) {
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

int gl_renderer_swap_buffers(GLFWwindow *window) {
    if (!window)
        return CLS_NULLPTR;

    glfwSwapBuffers(window);
    return CLS_SUCCESS;
}

void gl_renderer_on_resize(int width, int height) {
    glViewport(0, 0, width, height);
}

static int render_batch(struct app *app, struct renderer_batch *batch) {
    const struct gl_mesh *mesh = NULL;
    int error = assets_mesh_get(&mesh, app->assets, batch->data.mesh_id);
    if (error)
        return error;

    const struct shader *shader = NULL;
    error = assets_shader_get(&shader, app->assets, batch->data.shader_id);
    if (error)
        return error;

    const struct texture2d *texture = NULL;
    error = assets_texture2d_get(&texture, app->assets, batch->data.texture_id);
    if (error)
        return error;

    error = gl_shader_use(shader);
    if (error)
        return error;

    error = gl_texture2d_use(texture);
    if (error)
        return error;

    batch->data.transparent ? glEnable(GL_BLEND) : glDisable(GL_BLEND);

    size_t cmds_length = 0;
    error = array_length_get(&cmds_length, batch->cmds);
    if (error)
        return error;

    if (cmds_length > 1) {
        size_t instance_count = cmds_length;
        if (instance_count == 0)
            return CLS_SUCCESS;

        struct gl_mesh_instance_data *instances = NULL;
        error =
            allocator_alloc((void **)&instances, app->alloc_frame,
                            sizeof(struct gl_mesh_instance_data) * cmds_length,
                            alignof(struct gl_mesh_instance_data));
        if (error)
            return error;

        for (size_t i = 0; i < instance_count; ++i) {
            void *cmd_ptr = NULL;
            error = array_elem_get(&cmd_ptr, batch->cmds, i);
            if (error)
                continue;

            if (!cmd_ptr)
                continue;

            struct renderer_cmd *cmd = *(struct renderer_cmd **)cmd_ptr;
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

        gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_TRUE);
        gl_mesh_draw_instanced((struct gl_mesh *)mesh, instances,
                               (GLsizei)instance_count);
    } else {
        for (size_t i = 0; i < cmds_length; ++i) {
            void *cmd_ptr = NULL;
            error = array_elem_get(&cmd_ptr, batch->cmds, i);
            if (error)
                continue;

            if (!cmd_ptr)
                continue;

            struct renderer_cmd *cmd = *(struct renderer_cmd **)cmd_ptr;
            if (!cmd)
                continue;

            vec4 tint = {(float)cmd->ren->tint[0] / 255.0f,
                         (float)cmd->ren->tint[1] / 255.0f,
                         (float)cmd->ren->tint[2] / 255.0f,
                         (float)cmd->ren->tint[3] / 255.0f};

            gl_shader_set_mat4(shader->gl.id, "u_mvp", &cmd->mvp);
            gl_shader_set_vec4(shader->gl.id, "u_tint", &tint);
            gl_shader_set_vec2(shader->gl.id, "u_uv_offset",
                               &cmd->ren->uv_offset);
            gl_shader_set_vec2(shader->gl.id, "u_uv_scale",
                               &cmd->ren->uv_scale);
            gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_FALSE);
            gl_mesh_draw(mesh);
        }
    }

    return CLS_SUCCESS;
}

int gl_renderer_draw_frame(struct app *app, struct array *batches) {
    if (!app || !batches)
        return CLS_NULLPTR;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    size_t batches_length = 0;
    int error = array_length_get(&batches_length, batches);
    if (error)
        return error;

    for (size_t i = 0; i < batches_length; ++i) {
        struct renderer_batch *batch = NULL;
        error = array_elem_get((void **)&batch, batches, i);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Getting render batch failed");
            continue;
        }

        error = render_batch(app, batch);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "GL rendering render batch failed");
            continue;
        }
    }

    return CLS_SUCCESS;
}
