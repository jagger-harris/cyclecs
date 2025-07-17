#include "core/gfx/gl/shader.h"
#include "core/util/logger.h"
#include <string.h>

#define STR_MAX 512
#define STR_MIN 256

static int check_errs(GLuint shader, GLuint type) {
    err status = CORE_SUCCESS;
    GLint gl_success;
    char msg[STR_MAX];

    if (!glIsShader(shader)) {
        status = CORE_ARGS;
        goto err;
    }

    glGetShaderiv(shader, GL_COMPILE_STATUS, &gl_success);

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
        status = CORE_GL;
        goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, msg);
    return status;
}

err gl_shader_init(GLuint *out, const char *vert_src, const char *frag_src) {
    err status = CORE_SUCCESS;

    if (!out || !vert_src || !frag_src) {
        status = CORE_NULLPTR;
        goto err;
    }

    GLuint vert = 0;
    GLuint frag = 0;

    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);
    status = check_errs(vert, GL_VERTEX_SHADER);
    if (status)
        goto err;

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);
    status = check_errs(frag, GL_FRAGMENT_SHADER);
    if (status)
        goto err;

    *out = glCreateProgram();
    glAttachShader(*out, vert);
    glAttachShader(*out, frag);
    glLinkProgram(*out);

    if (glGetError() != GL_NO_ERROR) {
        status = CORE_GL;
        goto err;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init gl shader failed");
    return status;
}

void gl_shader_destroy(GLuint shader) {
    if (glIsProgram(shader) == GL_FALSE)
        return;

    glDeleteProgram(shader);
}

err gl_shader_use(GLuint shader) {
    err status = CORE_SUCCESS;

    if (!glIsProgram(shader)) {
        status = CORE_ARGS;
        goto err;
    }

    glUseProgram(shader);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Using gl shader failed");
    return status;
}

err gl_shader_set_bool(GLuint shader, const char *name, GLboolean data) {
    err status = CORE_SUCCESS;

    if (!name) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (!glIsProgram(shader)) {
        status = CORE_ARGS;
        goto err;
    }

    glUniform1i(glGetUniformLocation(shader, name), data);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Setting gl shader bool failed");
    return status;
}

err gl_shader_set_int(GLuint shader, const char *name, GLint data) {
    err status = CORE_SUCCESS;

    if (!name) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (!glIsProgram(shader)) {
        status = CORE_ARGS;
        goto err;
    }

    glUniform1i(glGetUniformLocation(shader, name), data);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Setting gl shader int failed");
    return status;
}

err gl_shader_set_float(GLuint shader, const char *name, GLfloat data) {
    err status = CORE_SUCCESS;

    if (!name) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (!glIsProgram(shader)) {
        status = CORE_ARGS;
        goto err;
    }

    glUniform1f(glGetUniformLocation(shader, name), data);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Setting gl shader float failed");
    return status;
}

err gl_shader_set_two_float(GLuint shader, const char *name, GLfloat data1,
                            GLfloat data2) {
    err status = CORE_SUCCESS;

    if (!name) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (!glIsProgram(shader)) {
        status = CORE_ARGS;
        goto err;
    }

    glUniform2f(glGetUniformLocation(shader, name), data1, data2);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Setting gl shader two floats failed");
    return status;
}

err gl_shader_set_mat4(GLuint shader, const char *name, mat4 *data) {
    err status = CORE_SUCCESS;

    if (!name) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (!glIsProgram(shader)) {
        status = CORE_ARGS;
        goto err;
    }

    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, GL_FALSE,
                       (const GLfloat *)data);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Setting gl shader mat4 f ailed");
    return status;
}

err gl_shader_set_vec3(GLuint shader, const char *name, vec3 *data) {
    err status = CORE_SUCCESS;

    if (!name) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (!glIsProgram(shader)) {
        status = CORE_ARGS;
        goto err;
    }

    glUniform3fv(glGetUniformLocation(shader, name), 1, (const GLfloat *)data);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Setting gl shader vec3 failed");
    return status;
}
