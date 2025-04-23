#include "core/gfx/gl/shader.h"
#include "core/util/logger.h"
#include <string.h>

#define STR_MAX 512
#define STR_MIN 256

static int check_errs(GLuint shader, GLuint type) {
    err err = CORE_SUCCESS;
    GLint gl_success;
    char msg[STR_MAX];

    if (!glIsShader(shader)) {
        err = CORE_INVALID_ARGS;
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
        err = CORE_INVALID_GFX_GL;
        goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, msg, err);
    return err;
}

err gl_shader_new(GLuint *out, const char *vert_src, const char *frag_src) {
    err err = CORE_SUCCESS;

    if (!out || !vert_src || !frag_src) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    GLuint vert = 0;
    GLuint frag = 0;

    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);
    err = check_errs(vert, GL_VERTEX_SHADER);
    if (err)
        goto err;

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);
    err = check_errs(frag, GL_FRAGMENT_SHADER);
    if (err)
        goto err;

    *out = glCreateProgram();
    glAttachShader(*out, vert);
    glAttachShader(*out, frag);
    glLinkProgram(*out);

    if (glGetError() != GL_NO_ERROR) {
        err = CORE_INVALID_GFX_GL;
        goto err;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new gl shader", err);
    return err;
}

err gl_shader_delete(GLuint shader) {
    err err = CORE_SUCCESS;

    if (glIsShader(shader) == GL_FALSE) {
        err = CORE_INVALID_GFX_GL;
        goto err;
    }

    glDeleteProgram(shader);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete gl shader", err);
    return err;
}

err gl_shader_use(GLuint shader) {
    err err = CORE_SUCCESS;

    if (!glIsShader(shader)) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to use gl shader", err);
    return err;
}

err gl_shader_set_bool(GLuint shader, const char *name, GLboolean data) {
    err err = CORE_SUCCESS;

    if (!name) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (!glIsShader(shader)) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    glUniform1i(glGetUniformLocation(shader, name), data);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to set gl shader bool", err);
    return err;
}

err gl_shader_set_int(GLuint shader, const char *name, GLint data) {
    err err = CORE_SUCCESS;

    if (!name) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (!glIsShader(shader)) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    glUniform1i(glGetUniformLocation(shader, name), data);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to set gl shader int", err);
    return err;
}

err gl_shader_set_float(GLuint shader, const char *name, GLfloat data) {
    err err = CORE_SUCCESS;

    if (!name) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (!glIsShader(shader)) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    glUniform1f(glGetUniformLocation(shader, name), data);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to set gl shader float", err);
    return err;
}

err gl_shader_set_two_float(GLuint shader, const char *name, GLfloat data1,
                            GLfloat data2) {
    err err = CORE_SUCCESS;

    if (!name) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (!glIsShader(shader)) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    glUniform2f(glGetUniformLocation(shader, name), data1, data2);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to set gl shader two floats", err);
    return err;
}

err gl_shader_set_mat4(GLuint shader, const char *name, mat4 *data) {
    err err = CORE_SUCCESS;

    if (!name) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (!glIsShader(shader)) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, GL_FALSE,
                       (const GLfloat *)data);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to set gl shader mat4", err);
    return err;
}

err gl_shader_set_vec3(GLuint shader, const char *name, vec3 *data) {
    err err = CORE_SUCCESS;

    if (!name) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (!glIsShader(shader)) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    glUniform3fv(glGetUniformLocation(shader, name), 1, (const GLfloat *)data);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to set gl shader vec3", err);
    return err;
}
