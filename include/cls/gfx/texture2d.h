#ifndef CLS_TEXTURE2D_H
#define CLS_TEXTURE2D_H

#include <cls/gfx/gl/texture2d.h>
#include <cls/util/types.h>
#include <stdbool.h>

enum texture2d_filter { TEXTURE_FILTER_LINEAR, TEXTURE_FILTER_NEAREST };
enum texture2d_wrap { TEXTURE_WRAP_CLAMP, TEXTURE_WRAP_REPEAT };

struct texture2d {
    union {
        struct gl_texture2d gl;
    };
};

struct texture2d_info {
    u8 *data;
    int width;
    int height;
    int channels;
    enum texture2d_filter filter;
    enum texture2d_wrap wrap;
};

#endif // CLS_TEXTURE2D_H
