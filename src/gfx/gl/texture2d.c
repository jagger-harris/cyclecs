#include <cls/gfx/gl/texture2d.h>
#include <cls/gfx/texture2d.h>
#include <cls/util/error.h>
#include <stddef.h>

int cls_gl_texture2d_init(struct cls_texture2d *tex,
                          struct cls_texture2d_info *info) {
    if (!tex || !info)
        return CLS_NULLPTR;

    GLint format = GL_RED;
    GLint filter = GL_LINEAR;
    GLint wrap = GL_CLAMP_TO_EDGE;

    if (info->channels == 3)
        format = GL_RGB;

    if (info->channels == 4)
        format = GL_RGBA;

    if (info->filter == CLS_TEXTURE_FILTER_NEAREST)
        filter = GL_NEAREST;

    if (info->wrap == CLS_TEXTURE_WRAP_REPEAT)
        wrap = GL_REPEAT;

    glGenTextures(1, &tex->gl.id);
    glBindTexture(GL_TEXTURE_2D, tex->gl.id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, info->width, info->height, 0,
                 (GLenum)format, GL_UNSIGNED_BYTE, info->data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (glGetError() != GL_NO_ERROR)
        return CLS_GL;

    return CLS_SUCCESS;
}

void cls_gl_texture2d_destroy(struct cls_texture2d *in) {
    if (!glIsTexture(in->gl.id))
        return;

    glDeleteTextures(1, &in->gl.id);
}

int cls_gl_texture2d_use(const struct cls_texture2d *in) {
    if (!glIsTexture(in->gl.id))
        return CLS_INVALID_ARG;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, in->gl.id);
    return CLS_SUCCESS;
}
