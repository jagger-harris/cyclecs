#include "core/gfx/gl/texture2d.h"
#include "core/util/error.h"
#include <stddef.h>

int gl_texture2d_init(GLuint *out, unsigned char *data, int width, int height,
                      int channels) {
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

    if (glGetError() != GL_NO_ERROR)
        return CORE_GL;

    return CORE_SUCCESS;
}

void gl_texture2d_destroy(GLuint texture) {
    if (glIsTexture(texture) == GL_FALSE)
        return;

    glDeleteTextures(1, &texture);
}

int gl_texture2d_use(GLuint texture) {
    if (!glIsTexture(texture))
        return CORE_INVALID_ARGS;

    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, texture);
    return CORE_SUCCESS;
}
