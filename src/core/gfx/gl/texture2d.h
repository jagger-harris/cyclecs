#ifndef GFX_GL_TEXTURE2D_H
#define GFX_GL_TEXTURE2D_H

#include <glad/gl.h>

int gl_texture2d_init(GLuint *out, unsigned char *data, int width, int height,
                      int channels);
void gl_texture2d_destroy(GLuint texture);
int gl_texture2d_use(GLuint texture);

#endif // GFX_GL_TEXTURE2D_H
