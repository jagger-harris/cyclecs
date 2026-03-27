#ifndef CLS_FONT_H
#define CLS_FONT_H

#include <cls/util/types.h>
#include <ft2build.h>
#include <stdbool.h>
#include FT_FREETYPE_H

#define CLS_FONT_CHAR_START 32
#define CLS_FONT_CHAR_END 126
#define CLS_FONT_CHAR_LENGTH (CLS_FONT_CHAR_END - CLS_FONT_CHAR_START + 1)

struct cls_glyph {
    unsigned int width;
    unsigned int height;
    int bearing_x;
    int bearing_y;
    long advance;
    unsigned int atlas_x;
    unsigned int atlas_y;
};

struct cls_font {
    struct cls_glyph glyphs[CLS_FONT_CHAR_LENGTH];
    FT_Face face;
    int pixel_size;
    unsigned int atlas_width;
    unsigned int atlas_height;
    u8 *atlas;
};

int cls_font_init(struct cls_font *f, FT_Library ft, const char *path,
                  int pixel_size);
void cls_font_destroy(struct cls_font *f);

#endif // CLS_FONT_H
