#include <cls/io/ffont.h>
#include <cls/io/fimage.h>
#include <cls/util/error.h>
#include <cls/util/globals.h>
#include <cls/util/types.h>
#include <freetype/freetype.h>
#include <freetype/ftimage.h>
#include <freetype/ftmodapi.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FFONT_MIN_ATLAS_SIZE 128
#define FFONT_MAX_ATLAS_SIZE 2048
#define FFONT_GLYPH_PADDING 2 // Prevents texture bleeding
#define FFONT_SDF_SPREAD 8 // SDF spread in pixels

struct ffont_meta {
    unsigned int atlas_width;
    unsigned int atlas_height;
    int pixel_size;
    u8 sdf_spread;
};

static unsigned int next_pow2(unsigned int i) {
    --i;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    ++i;
    return i;
}

static int ffont_save(const struct ffont *font, const char *path) {
    if (!font || !path)
        return CLS_NULLPTR;

    char atlas_image_path[CLS_GLOBALS_PATH_MAX] = {0};
    char meta_path[CLS_GLOBALS_PATH_MAX] = {0};

    int ret = snprintf(atlas_image_path, CLS_GLOBALS_PATH_MAX, "%s.png", path);
    if (ret < 0)
        return CLS_FAILURE;

    size_t size = (size_t)font->atlas_width * (size_t)font->atlas_height;
    u8 *atlas = malloc(size);
    if (!atlas)
        return CLS_OUT_OF_MEMORY;

    for (size_t i = 0; i < size; ++i)
        atlas[i] = font->atlas[i];

    struct fimage atlas_image = {.data = atlas,
                                 .width = (int)font->atlas_width,
                                 .height = (int)font->atlas_height,
                                 .channels = 1};
    int error = fimage_save(&atlas_image, atlas_image_path);
    if (error)
        goto cleanup;

    ret = snprintf(meta_path, CLS_GLOBALS_PATH_MAX, "%s.meta", path);
    if (ret < 0) {
        error = CLS_FAILURE;
        goto cleanup;
    }

    FILE *file = fopen(meta_path, "wb");
    if (!file) {
        error = CLS_FAILURE;
        goto cleanup;
    }

    struct ffont_meta meta = {.atlas_width = font->atlas_width,
                              .atlas_height = font->atlas_height,
                              .pixel_size = font->pixel_size,
                              .sdf_spread = FFONT_SDF_SPREAD};
    unsigned long length = fwrite(&meta, sizeof(meta), 1, file);
    if (length < 1) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    length =
        fwrite(font->glyphs, sizeof(struct fglyph), FFONT_CHAR_LENGTH, file);
    if (length < FFONT_CHAR_LENGTH) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    if (fclose(file) != 0) {
        error = CLS_FAILURE;
        goto cleanup;
    }

    return CLS_SUCCESS;

cleanup:
    if (atlas) {
        free(atlas);
        atlas = NULL;
    }

    return error;
}

static int ffont_load(struct ffont *font, const char *path) {
    if (!font || !path)
        return CLS_NULLPTR;

    char atlas_image_path[CLS_GLOBALS_PATH_MAX] = {0};
    char meta_path[CLS_GLOBALS_PATH_MAX] = {0};

    int ret = snprintf(atlas_image_path, CLS_GLOBALS_PATH_MAX, "%s.png", path);
    if (ret < 0)
        return CLS_FAILURE;

    ret = snprintf(meta_path, CLS_GLOBALS_PATH_MAX, "%s.meta", path);
    if (ret < 0)
        return CLS_FAILURE;

    struct fimage atlas_image = {0};
    int error = fimage_init(&atlas_image, atlas_image_path);
    if (error)
        return error;

    FILE *file = fopen(meta_path, "rb");
    if (!file) {
        error = CLS_FILE_NOT_FOUND;
        goto cleanup;
    }

    struct ffont_meta meta = {0};

    if (fread(&meta, sizeof(meta), 1, file) != 1) {
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    if (fread(font->glyphs, sizeof(struct fglyph), FFONT_CHAR_LENGTH, file) !=
        FFONT_CHAR_LENGTH) {
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    (void)fclose(file);
    file = NULL;

    font->atlas_width = meta.atlas_width;
    font->atlas_height = meta.atlas_height;
    font->pixel_size = meta.pixel_size;

    size_t atlas_size = (size_t)atlas_image.width * (size_t)atlas_image.height;
    font->atlas = malloc(atlas_size);
    if (!font->atlas) {
        error = CLS_OUT_OF_MEMORY;
        goto cleanup;
    }

    memcpy(font->atlas, atlas_image.data, atlas_size);
    fimage_destroy(&atlas_image);
    font->face = NULL;
    return CLS_SUCCESS;

cleanup:
    if (file)
        (void)fclose(file);

    fimage_destroy(&atlas_image);
    return error;
}

int ffont_init(struct ffont *font, FT_Library ft, const char *path,
               int pixel_size) {
    if (!font || !ft || !path)
        return CLS_NULLPTR;

    char cache_path[CLS_GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(cache_path, CLS_GLOBALS_PATH_MAX, "%s_%i_sdf", path,
                       pixel_size);
    if (ret < 0)
        return CLS_FAILURE;

    int error = ffont_load(font, cache_path);
    if (!error)
        return CLS_SUCCESS;

    if (FT_New_Face(ft, path, 0, &font->face))
        return CLS_FILE_NOT_FOUND;

    FT_Set_Pixel_Sizes(font->face, 0, (FT_UInt)pixel_size);
    font->pixel_size = pixel_size;

    // Set SDF property for the face
    FT_Property_Set(ft, "sdf", "spread", &(FT_Int){FFONT_SDF_SPREAD});

    unsigned int total_area = 0;
    unsigned int max_width = 0;
    unsigned int max_height = 0;
    for (u8 c = FFONT_CHAR_START; c <= FFONT_CHAR_END; ++c) {
        // Load with SDF render mode
        if (FT_Load_Char(font->face, c, FT_LOAD_DEFAULT))
            continue;

        if (FT_Render_Glyph(font->face->glyph, FT_RENDER_MODE_SDF))
            continue;

        FT_Bitmap *bmp = &font->face->glyph->bitmap;
        total_area += bmp->width * bmp->rows;

        if (bmp->width > max_width)
            max_width = bmp->width;

        if (bmp->rows > max_height)
            max_height = bmp->rows;
    }

    unsigned int atlas_size = next_pow2((unsigned int)sqrt(total_area * 1.5));
    if (atlas_size < FFONT_MIN_ATLAS_SIZE)
        atlas_size = FFONT_MIN_ATLAS_SIZE;
    if (atlas_size > FFONT_MAX_ATLAS_SIZE)
        atlas_size = FFONT_MAX_ATLAS_SIZE;

    // Pack font glyphs into texture atlas, keep trying until it fits
    bool packing_success = false;
    while (atlas_size <= FFONT_MAX_ATLAS_SIZE && !packing_success) {
        font->atlas_width = atlas_size;
        font->atlas_height = atlas_size;

        unsigned int test_x = 0;
        unsigned int test_y = 0;
        unsigned int test_shelf_height = 0;
        packing_success = true;

        for (u8 c = FFONT_CHAR_START; c <= FFONT_CHAR_END; ++c) {
            if (FT_Load_Char(font->face, c, FT_LOAD_DEFAULT))
                continue;

            if (FT_Render_Glyph(font->face->glyph, FT_RENDER_MODE_SDF))
                continue;

            FT_Bitmap *bmp = &font->face->glyph->bitmap;

            if (test_x + bmp->width > atlas_size) {
                test_x = 0;
                test_y += test_shelf_height;
                test_shelf_height = 0;
            }

            if (test_y + bmp->rows > atlas_size) {
                packing_success = false;
                break;
            }

            test_x += bmp->width;
            if (bmp->rows > test_shelf_height)
                test_shelf_height = bmp->rows;
        }

        if (!packing_success)
            atlas_size *= 2;
    }

    // Could not fit in FFONT_MAX_ATLAS_SIZE * FFONT_MAX_ATLAS_SIZE
    if (!packing_success)
        return CLS_FAILURE;

    font->atlas = calloc((size_t)font->atlas_width * font->atlas_height, 1);
    if (!font->atlas)
        return CLS_OUT_OF_MEMORY;

    unsigned int shelf_x = 0;
    unsigned int shelf_y = 0;
    unsigned int shelf_height = 0;

    for (u8 c = FFONT_CHAR_START; c <= FFONT_CHAR_END; ++c) {
        if (FT_Load_Char(font->face, c, FT_LOAD_DEFAULT))
            continue;

        // Render glyph as SDF
        if (FT_Render_Glyph(font->face->glyph, FT_RENDER_MODE_SDF))
            continue;

        FT_GlyphSlot slot = font->face->glyph;
        unsigned int glyph_width = slot->bitmap.width;
        unsigned int glyph_height = slot->bitmap.rows;

        // If glyph does not fit on current shelf, move to next shelf
        if (shelf_x + glyph_width + FFONT_GLYPH_PADDING > font->atlas_width) {
            shelf_x = 0;
            shelf_y += shelf_height + FFONT_GLYPH_PADDING;
            shelf_height = 0;
        }

        // Copy SDF glyph data to atlas
        for (unsigned int row = 0; row < glyph_height; ++row) {
            for (unsigned int col = 0; col < glyph_width; ++col) {
                unsigned int x = shelf_x + col;
                unsigned int y = shelf_y + row;
                font->atlas[y * font->atlas_width + x] =
                    slot->bitmap
                        .buffer[row * (unsigned int)slot->bitmap.pitch + col];
            }
        }

        // Store glyph metadata
        struct fglyph *glyph = &font->glyphs[c - FFONT_CHAR_START];
        glyph->width = glyph_width;
        glyph->height = glyph_height;
        glyph->bearing_x = slot->bitmap_left;
        glyph->bearing_y = slot->bitmap_top;
        glyph->advance = slot->advance.x >> 6;
        glyph->atlas_x = shelf_x;
        glyph->atlas_y = shelf_y;

        // Update shelf position
        shelf_x += glyph_width + FFONT_GLYPH_PADDING;
        if (glyph_height > shelf_height)
            shelf_height = glyph_height;
    }

    error = ffont_save(font, cache_path);
    if (error)
        return error;

    return CLS_SUCCESS;
}

void ffont_destroy(struct ffont *font) {
    if (!font)
        return;

    if (font->atlas) {
        free(font->atlas);
        font->atlas = NULL;
    }

    if (font->face) {
        FT_Done_Face(font->face);
        font->face = NULL;
    }
}
