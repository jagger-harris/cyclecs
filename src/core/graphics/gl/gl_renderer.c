#include "gl_renderer.h"
#include "../../util/error.h"
#include <glad/gl.h>

int gl_renderer_swap_buffers(GLFWwindow *window) {
    int error = CORE_ERROR_SUCCESS;

    if (!window) {
        error = CORE_ERROR_INVALID_NULLPTR;
        return error;
    }

    glfwSwapBuffers(window);

    return error;
}

void gl_renderer_on_resize(int width, int height) {
    glViewport(0, 0, width, height);
}

void gl_renderer_clear(void) {
    glClearColor(0.2F, 0.3F, 0.4F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
