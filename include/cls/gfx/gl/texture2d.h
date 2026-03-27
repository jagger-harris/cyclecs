#ifndef CLS_GL_TEXTURE2D_H
#define CLS_GL_TEXTURE2D_H

#include <cls/util/types.h>
#include <glad/gl.h>

struct cls_texture2d;
struct cls_texture2d_info;

struct cls_gl_texture2d {
    GLuint id;
};

int cls_gl_texture2d_init(struct cls_texture2d *tex,
                          struct cls_texture2d_info *info);
void cls_gl_texture2d_destroy(struct cls_texture2d *tex);
int cls_gl_texture2d_use(const struct cls_texture2d *tex);

#endif // CLS_GL_TEXTURE2D_H
