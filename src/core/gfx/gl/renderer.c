#include "core/gfx/gl/renderer.h"
#include "core/gfx/draw_call.h"
#include "core/gfx/gl/mesh.h"
#include "core/gfx/gl/shader.h"
#include "core/gfx/gl/texture2d.h"
#include "core/util/logger.h"
#include <cglm/cglm.h>
#include <glad/gl.h>
#include <string.h>

err gl_renderer_init(void) {
    err status = CORE_SUCCESS;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        status = CORE_FAILURE;
        goto err;
    }

    return status;

err:
    logger_log_err(
        LOGGER_ERR, status,
        "Initializing OpenGL failed (old graphics drivers or unsupported GPU)");
    return status;
}

err gl_renderer_swap_buffers(GLFWwindow *window) {
    err status = CORE_SUCCESS;

    if (!window) {
        status = CORE_NULLPTR;
        goto err;
    }

    glfwSwapBuffers(window);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Swapping buffers failed");
    return status;
}

void gl_renderer_on_resize(int width, int height) {
    glViewport(0, 0, width, height);
}

static int compare_draw_camera_dist(const void *a, const void *b) {
    struct draw_call *draw_a = *(struct draw_call **)a;
    struct draw_call *draw_b = *(struct draw_call **)b;
    return (draw_b->camera_dist > draw_a->camera_dist) -
           (draw_b->camera_dist < draw_a->camera_dist);
}

err gl_renderer_render_frame(struct assets *assets, struct camera *camera,
                             struct array *opaque_draws,
                             struct array *transparent_draws) {
    glClearColor(0.2F, 0.3F, 0.4F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO: Add batching draw calls to avoid frequent opengl state changes

    GLuint shader = 0;
    GLuint texture = 0;

    for (size_t i = 0; i < opaque_draws->length; ++i) {
        struct draw_call *draw = NULL;
        array_get_ptr((void **)&draw, opaque_draws, i);

        if (!draw || !draw->visible)
            continue;

        assets_shader_get(&shader, assets, draw->mat->shader_path);
        assets_texture_get(&texture, assets, draw->mat->texture_path);

        gl_shader_use(shader);
        gl_texture2d_use(texture);

        mat4 mvp = {0};
        mat4 model = {0};
        glm_mat4_identity(mvp);
        glm_mat4_identity(model);

        glm_scale(model, draw->tf.scale);
        glm_rotate(model, draw->tf.rot_deg, draw->tf.rot);
        glm_translate(model, draw->tf.pos);

        glm_mat4_mul(mvp, camera->projection, mvp);
        glm_mat4_mul(mvp, camera->view, mvp);
        glm_mat4_mul(mvp, model, mvp);

        gl_shader_set_mat4(shader, "mvp", &mvp);
        gl_mesh_draw(draw->mesh);
    }

    if (0 < transparent_draws->length) {
        struct draw_call **sorted = transparent_draws->data;
        qsort(sorted, transparent_draws->length, sizeof(struct draw_call *),
              compare_draw_camera_dist);
    }

    for (size_t i = 0; i < transparent_draws->length; ++i) {
        struct draw_call *draw = NULL;
        array_get_ptr((void **)&draw, transparent_draws, i);

        if (!draw || !draw->visible)
            continue;

        assets_shader_get(&shader, assets, draw->mat->shader_path);
        assets_texture_get(&texture, assets, draw->mat->shader_path);

        gl_shader_use(shader);
        gl_texture2d_use(texture);
        gl_mesh_draw(draw->mesh);
    }

    return 0;
}
