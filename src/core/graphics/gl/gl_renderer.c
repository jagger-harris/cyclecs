#include "gl_renderer.h"
#include "../../util/error.h"
#include "../../util/logger.h"
#include <glad/gl.h>

int gl_renderer_init(void) {
    int error = CORE_SUCCESS;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        error = CORE_FAILURE;
        goto error;
    }

    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to initialize OpenGL", error);
    return error;
}

int gl_renderer_swap_buffers(GLFWwindow *window) {
    int error = CORE_SUCCESS;

    if (!window) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    glfwSwapBuffers(window);
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to swap buffers", error);
    return error;
}

void gl_renderer_on_resize(int width, int height) {
    glViewport(0, 0, width, height);
}

void gl_renderer_clear(void) {
    glClearColor(0.2F, 0.3F, 0.4F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
