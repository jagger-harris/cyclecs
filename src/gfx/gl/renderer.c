/**
 * @file cls/gfx/gl/renderer.c
 * @brief OpenGL renderer for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/gfx/gl/renderer.h
 */

#include <GLFW/glfw3.h>
#include <assert.h>
#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/ecs/component/components.h>
#include <cls/gfx/gl/mesh.h>
#include <cls/gfx/renderer.h>
#include <cls/gfx/shader.h>
#include <cls/util/array.h>
#include <cls/util/mem.h>

#if NDEBUG
#include <cls/util/logger.h>

void GLAPIENTRY msg_callback(GLenum source, GLenum type, GLuint id,
                             GLenum severity, GLsizei length,
                             const GLchar *message, const void *user) {
    (void)source;
    (void)id;
    (void)length;
    (void)user;

    if (severity == GL_DEBUG_SEVERITY_MEDIUM ||
        severity == GL_DEBUG_SEVERITY_HIGH) {
        enum cls_logger_level level = severity == GL_DEBUG_SEVERITY_HIGH
                                          ? CLS_LOGGER_ERROR
                                          : CLS_LOGGER_WARN;
        CLS_LOGGER_LOG(level, "GL 0x%x: %s\n", type, message);
    }
}
#endif

cls_error cls_gl_renderer_init(ivec4 bg_color) {
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
        return CLS_GL;

#if NDEBUG
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
    return CLS_SUCCESS;
}

cls_error cls_gl_renderer_swap_buffers(GLFWwindow *win) {
    if (!win)
        return CLS_NULLPTR;

    glfwSwapBuffers(win);
    return CLS_SUCCESS;
}

void cls_gl_renderer_on_resize(int width, int height) {
    glViewport(0, 0, width, height);
}

static void apply_render_state(const struct cls_render_state *state) {
    assert(state && "state is NULL");

    if (state->depth_test) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask((GLboolean)state->depth_write);

    if (state->blending) {
        glEnable(GL_BLEND);
        glBlendFunc((GLenum)state->blend_src, (GLenum)state->blend_dest);
    } else {
        glDisable(GL_BLEND);
    }
}

static cls_error render_batch(struct cls_app *app, struct cls_array *cmds,
                              struct cls_renderer_batch *batch) {
    assert(app && cmds && batch && "app or cmds or batch is NULL");

    struct cls_gl_mesh *mesh = NULL;
    cls_error error =
        cls_assets_mesh_get(&mesh, app->assets, batch->data.mesh_id);
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
        void *instances_ptr = NULL;
        error = cls_mem_alloc(&instances_ptr, app->mem_frame,
                              sizeof(struct cls_gl_mesh_instance_data) *
                                  cmds_length,
                              alignof(struct cls_gl_mesh_instance_data));
        if (error)
            return error;

        struct cls_gl_mesh_instance_data *instances = instances_ptr;
        if (!instances)
            return error;

        for (size_t i = 0; i < cmds_length; ++i) {
            size_t cmd_idx = 0;
            error = cls_array_elem_get_cpy(&cmd_idx, batch->cmds, i);
            if (error)
                continue;

            void *cmd_ptr = NULL;
            error = cls_array_elem_get(&cmd_ptr, cmds, cmd_idx);
            if (error)
                continue;

            struct cls_renderer_cmd *cmd = cmd_ptr;
            if (!cmd)
                continue;

            glm_mat4_copy(cmd->mvp, instances[i].mvp);
            glm_vec4_copy((vec4){(float)cmd->ren.tint[0] / 255.0f,
                                 (float)cmd->ren.tint[1] / 255.0f,
                                 (float)cmd->ren.tint[2] / 255.0f,
                                 (float)cmd->ren.tint[3] / 255.0f},
                          instances[i].tint);
            glm_vec2_copy(cmd->ren.uv_offset, instances[i].uv_offset);
            glm_vec2_copy(cmd->ren.uv_scale, instances[i].uv_scale);
        }

        cls_gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_TRUE);
        cls_gl_mesh_draw_instanced(mesh, instances, (GLsizei)cmds_length);
    } else {
        for (size_t i = 0; i < cmds_length; ++i) {
            size_t cmd_idx = 0;
            error = cls_array_elem_get_cpy(&cmd_idx, batch->cmds, i);
            if (error)
                continue;

            void *cmd_ptr = NULL;
            error = cls_array_elem_get(&cmd_ptr, cmds, cmd_idx);
            if (error)
                continue;

            struct cls_renderer_cmd *cmd = cmd_ptr;
            if (!cmd)
                continue;

            vec4 tint = {(float)cmd->ren.tint[0] / 255.0f,
                         (float)cmd->ren.tint[1] / 255.0f,
                         (float)cmd->ren.tint[2] / 255.0f,
                         (float)cmd->ren.tint[3] / 255.0f};

            cls_gl_shader_set_mat4(shader->gl.id, "u_mvp", &cmd->mvp);
            cls_gl_shader_set_vec4(shader->gl.id, "u_tint", &tint);
            cls_gl_shader_set_vec2(shader->gl.id, "u_uv_offset",
                                   &cmd->ren.uv_offset);
            cls_gl_shader_set_vec2(shader->gl.id, "u_uv_scale",
                                   &cmd->ren.uv_scale);
            cls_gl_shader_set_bool(shader->gl.id, "u_use_instancing", GL_FALSE);
            cls_gl_mesh_draw(mesh);
        }
    }

    return CLS_SUCCESS;
}

static int batch_depth_compare(const void *a, const void *b) {
    assert(a && b && "a or b is NULL");

    const struct cls_renderer_batch *ba =
        *(const struct cls_renderer_batch **)a;
    const struct cls_renderer_batch *bb =
        *(const struct cls_renderer_batch **)b;

    return (ba->depth < bb->depth) ? -1 : (ba->depth > bb->depth) ? 1 : 0;
}

void cls_gl_renderer_begin_frame(void) {
    // Reset masks
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Default pipeline state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

cls_error cls_gl_renderer_draw_batches(struct cls_app *app,
                                       struct cls_array *cmds,
                                       struct cls_array *batches,
                                       struct cls_array **transparent_batches) {
    if (!app || !batches || !transparent_batches)
        return CLS_NULLPTR;

    size_t batches_len = 0;
    cls_error error = cls_array_length_get(&batches_len, batches);
    if (error)
        return error;

    void *batches_data = NULL;
    error = cls_array_data_get(&batches_data, batches);
    if (error)
        return error;

    for (size_t i = 0; i < batches_len; ++i) {
        void *batch_ptr = NULL;
        error = cls_array_elem_get(&batch_ptr, batches, i);
        if (error)
            continue;

        struct cls_renderer_batch *batch = batch_ptr;
        if (!batch || batch->data.state.blending)
            continue; // Skip transparent batches

        apply_render_state(&batch->data.state);
        error = render_batch(app, cmds, batch);
        if (error)
            continue;
    }

    cls_array_clear(*transparent_batches);

    for (size_t i = 0; i < batches_len; ++i) {
        void *batch_ptr = NULL;
        error = cls_array_elem_get(&batch_ptr, batches, i);
        if (error)
            continue;

        struct cls_renderer_batch *batch = batch_ptr;
        if (!batch || !batch->data.state.blending)
            continue;

        error = cls_array_push(transparent_batches, (const void *)&batch_ptr);
        if (error)
            continue;
    }

    size_t transparent_batch_len = 0;
    error = cls_array_length_get(&transparent_batch_len, *transparent_batches);
    if (error)
        return error;

    void *transparent_data = NULL;
    error = cls_array_data_get(&transparent_data, *transparent_batches);
    if (error)
        return error;

    qsort(transparent_data, transparent_batch_len,
          sizeof(struct cls_renderer_batch *), batch_depth_compare);

    for (size_t i = 0; i < transparent_batch_len; ++i) {
        void *batch_ptr = NULL;
        error = cls_array_elem_get(&batch_ptr, *transparent_batches, i);
        if (error)
            continue;

        struct cls_renderer_batch *batch =
            *(struct cls_renderer_batch **)batch_ptr;

        apply_render_state(&batch->data.state);
        error = render_batch(app, cmds, batch);
        if (error)
            continue;
    }

    return CLS_SUCCESS;
}
