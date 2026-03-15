#ifndef IO_FFONT_H
#define IO_FFONT_H

#include <cls/util/types.h>
#include <ft2build.h>
#include <stdbool.h>
#include FT_FREETYPE_H

#define FFONT_CHAR_START 32
#define FFONT_CHAR_END 126
#define FFONT_CHAR_LENGTH (FFONT_CHAR_END - FFONT_CHAR_START + 1)

struct fglyph {
    unsigned int width;
    unsigned int height;
    int bearing_x;
    int bearing_y;
    long advance;
    unsigned int atlas_x;
    unsigned int atlas_y;
};

struct ffont {
    struct fglyph glyphs[FFONT_CHAR_LENGTH];
    FT_Face face;
    int pixel_size;
    unsigned int atlas_width;
    unsigned int atlas_height;
    u8 *atlas;
};

int ffont_init(struct ffont *font, FT_Library ft, const char *path,
               int pixel_size);
void ffont_destroy(struct ffont *font);

#endif // IO_FFONT_H
