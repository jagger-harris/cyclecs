#ifndef GFX_GL_TEXTURE2D_H
#define GFX_GL_TEXTURE2D_H

#include "core/util/types.h"
#include <glad/gl.h>

struct texture2d;
struct texture2d_info;

struct gl_texture2d {
    GLuint id;
};

int gl_texture2d_init(struct texture2d *out, struct texture2d_info *info);
void gl_texture2d_destroy(struct texture2d *in);
int gl_texture2d_use(struct texture2d *in);

#endif // GFX_GL_TEXTURE2D_H
