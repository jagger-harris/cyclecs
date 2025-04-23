#ifndef GFX_GL_TEXTURE2D_H
#define GFX_GL_TEXTURE2D_H

#include "core/util/err.h"
#include <glad/gl.h>

err gl_texture2d_new(GLuint *out, unsigned char *data, int width, int height,
                     int channels);
err gl_texture2d_delete(GLuint texture);
err gl_texture2d_use(GLuint texture);

#endif /* GFX_GL_TEXTURE2D_H */
