#ifndef CLS_FONT_H
#define CLS_FONT_H

#include <cls/util/types.h>
#include <ft2build.h>
#include <stdbool.h>
#include FT_FREETYPE_H

#define FONT_CHAR_START 32
#define FONT_CHAR_END 126
#define FONT_CHAR_LENGTH (FONT_CHAR_END - FONT_CHAR_START + 1)

struct glyph {
    unsigned int width;
    unsigned int height;
    int bearing_x;
    int bearing_y;
    long advance;
    unsigned int atlas_x;
    unsigned int atlas_y;
};

struct font {
    struct glyph glyphs[FONT_CHAR_LENGTH];
    FT_Face face;
    int pixel_size;
    unsigned int atlas_width;
    unsigned int atlas_height;
    u8 *atlas;
};

int font_init(struct font *f, FT_Library ft, const char *path, int pixel_size);
void font_destroy(struct font *f);

#endif // CLS_FONT_H
