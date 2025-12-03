#include "core/gfx/gl/texture2d.h"
#include "core/gfx/texture2d.h"
#include "core/util/error.h"
#include <stddef.h>

int gl_texture2d_init(struct texture2d *out, struct texture2d_info *info) {
    if (!out || !info)
        return CORE_NULLPTR;

    GLint format = GL_RED;
    GLint filter = GL_LINEAR;
    GLint wrap = GL_CLAMP_TO_EDGE;

    if (info->channels == 3)
        format = GL_RGB;

    if (info->channels == 4)
        format = GL_RGBA;

    if (info->filter == TEXTURE_FILTER_NEAREST)
        filter = GL_NEAREST;

    if (info->wrap == TEXTURE_WRAP_REPEAT)
        wrap = GL_REPEAT;

    glGenTextures(1, &out->gl.id);
    glBindTexture(GL_TEXTURE_2D, out->gl.id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, info->width, info->height, 0,
                 (GLenum)format, GL_UNSIGNED_BYTE, info->data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (glGetError() != GL_NO_ERROR)
        return CORE_GL;

    return CORE_SUCCESS;
}

void gl_texture2d_destroy(struct texture2d *in) {
    if (!glIsTexture(in->gl.id))
        return;

    glDeleteTextures(1, &in->gl.id);
}

int gl_texture2d_use(const struct texture2d *in) {
    if (!glIsTexture(in->gl.id))
        return CORE_INVALID_ARG;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, in->gl.id);
    return CORE_SUCCESS;
}
