#include "core/gfx/renderer.h"
#include "core/util/logger.h"
#include <cglm/cglm.h>
#include <glad/gl.h>

struct renderable_object {
    GLuint vao;
    GLuint shader;
    GLuint texture;
    mat4 transform;
};

err gl_renderer_init(void) {
    err err = CORE_SUCCESS;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        err = CORE_FAILURE;
        goto err;
    }

    return err;

err:
    logger_log(
        LOGGER_ERR,
        "Failed to initialize OpenGL (old graphics drivers or unsupported GPU)",
        err);
    return err;
}

err gl_renderer_swap_buffers(GLFWwindow *window) {
    err err = CORE_SUCCESS;

    if (!window) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    glfwSwapBuffers(window);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to swap buffers", err);
    return err;
}

void gl_renderer_on_resize(int width, int height) {
    glViewport(0, 0, width, height);
}

err gl_renderer_render_frame(void) {
    glClearColor(0.2F, 0.3F, 0.4F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return 0;
}
