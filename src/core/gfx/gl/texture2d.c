#include "core/gfx/gl/texture2d.h"
#include "core/util/logger.h"
#include <stddef.h>

err gl_texture2d_new(GLuint *out, unsigned char *data, int width, int height,
                     int channels) {
    err err = CORE_SUCCESS;
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
        err = CORE_INVALID_GFX_GL;
        goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new gl texture", err);
    return err;
}

err gl_texture2d_delete(GLuint texture) {
    err err = CORE_SUCCESS;

    if (glIsTexture(texture) == GL_FALSE) {
        err = CORE_INVALID_GFX_GL;
        goto err;
    }

    glDeleteTextures(1, &texture);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete gl texture", err);
    return err;
}

err gl_texture2d_use(GLuint texture) {
    err err = CORE_SUCCESS;

    if (!glIsTexture(texture)) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, texture);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to use gl texture", err);
    return err;
}
