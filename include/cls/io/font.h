#ifndef CLS_FONT_H
#define CLS_FONT_H

#include <cls/util/error.h>
#include <cls/util/types.h>
#include <ft2build.h>
#include <stdbool.h>
#include FT_FREETYPE_H

/**
 * @enum
 * @brief Font character range.
 */
enum {
    CLS_FONT_CHAR_START = 32, /**< First character in the font atlas. */
    CLS_FONT_CHAR_END = 126, /**< Last character in the font atlas. */
    CLS_FONT_CHAR_LENGTH = CLS_FONT_CHAR_END - CLS_FONT_CHAR_START +
                           1, /**< Number of characters in the font atlas. */
};

/**
 * @struct cls_glyph
 * @brief Glyph.
 */
struct cls_glyph {
    unsigned int width;
    unsigned int height;
    int bearing_x;
    int bearing_y;
    long advance;
    unsigned int atlas_x;
    unsigned int atlas_y;
};

/**
 * @struct cls_font
 * @brief Font.
 */
struct cls_font {
    struct cls_glyph glyphs[CLS_FONT_CHAR_LENGTH];
    FT_Face face;
    int pixel_size;
    unsigned int atlas_width;
    unsigned int atlas_height;
    u8 *atlas;
};

/**
 * @brief Initializes a font.
 *
 * Loads a cached glyph atlas if available. Otherwise rasterizes the font
 * glyphs into an SDF atlas and stores the glyph data for later use.
 *
 * @param[out] f          Font.
 * @param[in]  ft         FreeType library.
 * @param[in]  path       Font file path.
 * @param[in]  pixel_size Font pixel size.
 *
 * @return CLS_SUCCESS        On success.
 * @retval CLS_NULLPTR        If `f`, `ft`, or `path` is NULL.
 * @retval CLS_FAILURE        If creating the cache path fails or the glyphs
 *                            cannot fit in the atlas.
 * @retval CLS_FILE_NOT_FOUND If loading the font face fails.
 * @retval CLS_OUT_OF_MEMORY  If allocating the atlas fails.
 * @retval (error)            If saving the atlas cache fails.
 */
cls_error cls_font_init(struct cls_font *f, FT_Library ft, const char *path,
                        int pixel_size);

/**
 * @brief Destroys a font.
 *
 * Releases the font atlas.
 *
 * @param[in] f Font to destroy.
 */
void cls_font_destroy(struct cls_font *f);

#endif // CLS_FONT_H
