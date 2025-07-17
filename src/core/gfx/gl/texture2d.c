#include "core/gfx/gl/texture2d.h"
#include "core/util/logger.h"
#include <stddef.h>

err gl_texture2d_init(GLuint *out, unsigned char *data, int width, int height,
                      int channels) {
    err status = CORE_SUCCESS;
    GLenum format = GL_RED;

    if (channels == 3)
        format = GL_RGB;

    if (channels == 4)
        format = GL_RGBA;

    glGenTextures(1, out);
    glBindTexture(GL_TEXTURE_2D, *out);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (glGetError() != GL_NO_ERROR) {
        status = CORE_GL;
        goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init gl texture2d failed");
    return status;
}

void gl_texture2d_destroy(GLuint texture) {
    if (glIsTexture(texture) == GL_FALSE)
        return;

    glDeleteTextures(1, &texture);
}

err gl_texture2d_use(GLuint texture) {
    err status = CORE_SUCCESS;

    if (!glIsTexture(texture)) {
        status = CORE_ARGS;
        goto err;
    }

    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, texture);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Using gl texture2d failed");
    return status;
}
