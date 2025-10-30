#include "core/gfx/gl/shader.h"
#include "core/gfx/shader.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include <cglm/types.h>
#include <string.h>

#define STR_MAX 512

static int check_shader(GLuint shader, GLuint type) {
    if (!glIsShader(shader))
        return CORE_INVALID_ARG;

    GLint gl_success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &gl_success);

    if (!gl_success) {
        char log[STR_MAX];
        glGetShaderInfoLog(shader, STR_MAX, NULL, log);

        const char *type_name;
        switch (type) {
        case GL_VERTEX_SHADER:
            type_name = "vertex";
            break;
        case GL_FRAGMENT_SHADER:
            type_name = "fragment";
            break;
        default:
            type_name = "unknown";
            break;
        }

        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GL,
                         "Compiling (%s) shader failed: %s", type_name, log);
        return CORE_GL;
    }

    return CORE_SUCCESS;
}

int gl_shader_init(struct shader *out, const struct shader_info *info) {
    if (!out || !info || !info->vert_src || !info->frag_src)
        return CORE_NULLPTR;

    GLuint vert = 0;
    GLuint frag = 0;

    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &info->vert_src, NULL);
    glCompileShader(vert);
    int error = check_shader(vert, GL_VERTEX_SHADER);
    if (error)
        return error;

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &info->frag_src, NULL);
    glCompileShader(frag);
    error = check_shader(frag, GL_FRAGMENT_SHADER);
    if (error)
        return error;

    out->gl.id = glCreateProgram();
    glAttachShader(out->gl.id, vert);
    glAttachShader(out->gl.id, frag);
    glLinkProgram(out->gl.id);

    int gl_error = (int)glGetError();
    if (gl_error != GL_NO_ERROR) {
        LOGGER_LOG_ERROR(LOGGER_WARN, error, "%s", "Adding gl shader failed");
        return CORE_GL;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return CORE_SUCCESS;
}

void gl_shader_destroy(struct shader *in) {
    if (!in)
        return;

    if (!glIsProgram(in->gl.id))
        return;

    glDeleteProgram(in->gl.id);
}

int gl_shader_use(struct shader *in) {
    if (!in)
        return CORE_NULLPTR;

    if (!glIsProgram(in->gl.id))
        return CORE_INVALID_ARG;

    glUseProgram(in->gl.id);
    return CORE_SUCCESS;
}

int gl_shader_set_bool(GLuint shader, const char *name, GLboolean data) {
    if (!name)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARG;

    glUniform1i(glGetUniformLocation(shader, name), data);
    return CORE_SUCCESS;
}

int gl_shader_set_int(GLuint shader, const char *name, GLint data) {
    if (!name)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARG;

    glUniform1i(glGetUniformLocation(shader, name), data);
    return CORE_SUCCESS;
}

int gl_shader_set_float(GLuint shader, const char *name, GLfloat data) {
    if (!name)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARG;

    glUniform1f(glGetUniformLocation(shader, name), data);
    return CORE_SUCCESS;
}

int gl_shader_set_mat4(GLuint shader, const char *name, mat4 *data) {
    if (!name || !data)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARG;

    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, GL_FALSE,
                       (const GLfloat *)data);
    return CORE_SUCCESS;
}

int gl_shader_set_vec2(GLuint shader, const char *name, vec2 *data) {
    if (!name || !data)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARG;

    glUniform2fv(glGetUniformLocation(shader, name), 1, (const GLfloat *)data);
    return CORE_SUCCESS;
}

int gl_shader_set_vec3(GLuint shader, const char *name, vec3 *data) {
    if (!name || !data)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARG;

    glUniform3fv(glGetUniformLocation(shader, name), 1, (const GLfloat *)data);
    return CORE_SUCCESS;
}

int gl_shader_set_vec4(GLuint shader, const char *name, vec4 *data) {
    if (!name || !data)
        return CORE_NULLPTR;

    if (!glIsProgram(shader))
        return CORE_INVALID_ARG;

    glUniform4fv(glGetUniformLocation(shader, name), 1, (const GLfloat *)data);
    return CORE_SUCCESS;
}
