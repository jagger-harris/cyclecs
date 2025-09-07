#include "core/gfx/gl/shader.h"
#include "core/util/error.h"
#include <string.h>

#define STR_MAX 512
#define STR_MIN 256

static int check_ints(GLuint shader, GLuint type) {
    if (!glIsShader(shader))
        return CORE_INVALID_ARGS;

    GLint gl_success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &gl_success);

    char msg[STR_MAX];
    if (!gl_success) {
        char log[STR_MIN];
        glGetShaderInfoLog(shader, STR_MIN, NULL, log);

        switch (type) {
        case GL_VERTEX_SHADER:
            snprintf(msg, STR_MIN, "Vertex: ");
            break;
        case GL_FRAGMENT_SHADER:
            snprintf(msg, STR_MIN, "Fragment: ");
            break;
        default:
            snprintf(msg, STR_MIN, "Unknown: ");
            break;
        }

        strncat(msg, log, STR_MIN);
        return CORE_GL;
    }

    return CORE_SUCCESS;
}

int gl_shader_init(GLuint *out, const char *vert_src, const char *frag_src) {

    if (!out || !vert_src || !frag_src)
        return CORE_NULLPTR;

    GLuint vert = 0;
    GLuint frag = 0;
    int status = CORE_SUCCESS;

    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);
    status = check_ints(vert, GL_VERTEX_SHADER);
    if (status)
        return status;

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);
    status = check_ints(frag, GL_FRAGMENT_SHADER);
    if (status)
        return status;

    *out = glCreateProgram();
    glAttachShader(*out, vert);
    glAttachShader(*out, frag);
    glLinkProgram(*out);

    if (glGetError() != GL_NO_ERROR)
        return CORE_GL;

    glDeleteShader(vert);
    glDeleteShader(frag);
    return CORE_SUCCESS;
}

void gl_shader_destroy(GLuint shader) {
    if (glIsProgram(shader) == GL_FALSE)
        return;

    glDeleteProgram(shader);
}

int gl_shader_use(GLuint shader) {
    if (!glIsProgram(shader))
        return CORE_INVALID_ARGS;

    glUseProgram(shader);
    return CORE_SUCCESS;
}

int gl_shader_set_bool(GLuint shader, const char *name, GLboolean data) {
    if (!name)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARGS;

    glUniform1i(glGetUniformLocation(shader, name), data);
    return CORE_SUCCESS;
}

int gl_shader_set_int(GLuint shader, const char *name, GLint data) {
    if (!name)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARGS;

    glUniform1i(glGetUniformLocation(shader, name), data);
    return CORE_SUCCESS;
}

int gl_shader_set_float(GLuint shader, const char *name, GLfloat data) {
    if (!name)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARGS;

    glUniform1f(glGetUniformLocation(shader, name), data);
    return CORE_SUCCESS;
}

int gl_shader_set_two_float(GLuint shader, const char *name, GLfloat data1,
                            GLfloat data2) {
    if (!name)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARGS;

    glUniform2f(glGetUniformLocation(shader, name), data1, data2);
    return CORE_SUCCESS;
}

int gl_shader_set_mat4(GLuint shader, const char *name, mat4 *data) {
    if (!name || !data)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARGS;

    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, GL_FALSE,
                       (const GLfloat *)data);
    return CORE_SUCCESS;
}

int gl_shader_set_vec3(GLuint shader, const char *name, vec3 *data) {
    if (!name || !data)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARGS;

    glUniform3fv(glGetUniformLocation(shader, name), 1, (const GLfloat *)data);
    return CORE_SUCCESS;
}
