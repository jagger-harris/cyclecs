#include <cglm/types.h>
#include <cls/gfx/gl/shader.h>
#include <cls/gfx/shader.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <string.h>

#define LOG_BUFFER 512

static int check_shader(GLuint s, GLuint type) {
    if (!glIsShader(s))
        return CLS_INVALID_ARG;

    GLint gl_success = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &gl_success);

    if (!gl_success) {
        char log[LOG_BUFFER];
        glGetShaderInfoLog(s, LOG_BUFFER, NULL, log);

        const char *type_id;
        switch (type) {
        case GL_VERTEX_SHADER:
            type_id = "vertex";
            break;
        case GL_FRAGMENT_SHADER:
            type_id = "fragment";
            break;
        default:
            type_id = "unknown";
            break;
        }

        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GL,
                         "Compiling (%s) shader failed: %s", type_id, log);
        return CLS_GL;
    }

    return CLS_SUCCESS;
}

int gl_shader_init(struct shader *s, const struct shader_info *info) {
    if (!s || !info || !info->vert_src || !info->frag_src)
        return CLS_NULLPTR;

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

    s->gl.id = glCreateProgram();
    glAttachShader(s->gl.id, vert);
    glAttachShader(s->gl.id, frag);
    glLinkProgram(s->gl.id);

    int gl_error = (int)glGetError();
    if (gl_error != GL_NO_ERROR) {
        LOGGER_LOG_ERROR(LOGGER_WARN, error, "%s", "Adding gl shader failed");
        return CLS_GL;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return CLS_SUCCESS;
}

void gl_shader_destroy(struct shader *s) {
    if (!s)
        return;

    if (!glIsProgram(s->gl.id))
        return;

    glDeleteProgram(s->gl.id);
}

int gl_shader_use(const struct shader *s) {
    if (!s)
        return CLS_NULLPTR;

    if (!glIsProgram(s->gl.id))
        return CLS_INVALID_ARG;

    glUseProgram(s->gl.id);
    return CLS_SUCCESS;
}

int gl_shader_set_bool(GLuint s, const char *id, GLboolean value) {
    if (!id)
        return CLS_NULLPTR;

    if (!glIsProgram(s))
        return CLS_INVALID_ARG;

    glUniform1i(glGetUniformLocation(s, id), value);
    return CLS_SUCCESS;
}

int gl_shader_set_int(GLuint s, const char *id, GLint value) {
    if (!id)
        return CLS_NULLPTR;

    if (!glIsProgram(s))
        return CLS_INVALID_ARG;

    glUniform1i(glGetUniformLocation(s, id), value);
    return CLS_SUCCESS;
}

int gl_shader_set_float(GLuint s, const char *id, GLfloat value) {
    if (!id)
        return CLS_NULLPTR;

    if (!glIsProgram(s))
        return CLS_INVALID_ARG;

    glUniform1f(glGetUniformLocation(s, id), value);
    return CLS_SUCCESS;
}

int gl_shader_set_mat4(GLuint s, const char *id, mat4 *value) {
    if (!id || !value)
        return CLS_NULLPTR;

    if (!glIsProgram(s))
        return CLS_INVALID_ARG;

    glUniformMatrix4fv(glGetUniformLocation(s, id), 1, GL_FALSE,
                       (const GLfloat *)value);
    return CLS_SUCCESS;
}

int gl_shader_set_vec2(GLuint s, const char *id, vec2 *value) {
    if (!id || !value)
        return CLS_NULLPTR;

    if (!glIsProgram(s))
        return CLS_INVALID_ARG;

    glUniform2fv(glGetUniformLocation(s, id), 1, (const GLfloat *)value);
    return CLS_SUCCESS;
}

int gl_shader_set_vec3(GLuint s, const char *id, vec3 *value) {
    if (!id || !value)
        return CLS_NULLPTR;

    if (!glIsProgram(s))
        return CLS_INVALID_ARG;

    glUniform3fv(glGetUniformLocation(s, id), 1, (const GLfloat *)value);
    return CLS_SUCCESS;
}

int gl_shader_set_vec4(GLuint s, const char *id, vec4 *value) {
    if (!id || !value)
        return CLS_NULLPTR;

    if (!glIsProgram(s))
        return CLS_INVALID_ARG;

    glUniform4fv(glGetUniformLocation(s, id), 1, (const GLfloat *)value);
    return CLS_SUCCESS;
}
