#include "core/io/ffont.h"
#include "core/io/fimage.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/types.h"
#include <freetype/freetype.h>
#include <freetype/ftimage.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FFONT_MIN_ATLAS_SIZE 128
#define FFONT_MAX_ATLAS_SIZE 2048
#define FFONT_GLYPH_PADDING 2 // Prevents texture bleeding

struct ffont_meta {
    unsigned int atlas_width;
    unsigned int atlas_height;
    int pixel_size;
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

static int ffont_save(const struct ffont *in, const char *path) {
    if (!in || !path)
        return CORE_NULLPTR;

    char atlas_image_path[GLOBALS_PATH_MAX] = {0};
    char meta_path[GLOBALS_PATH_MAX] = {0};

    int ret = snprintf(atlas_image_path, GLOBALS_PATH_MAX, "%s.png", path);
    if (ret < 0)
        return CORE_FAILURE;

    size_t size = (size_t)in->atlas_width * (size_t)in->atlas_height;
    u8 *new_atlas = malloc(size);
    if (!new_atlas)
        return CORE_OUT_OF_MEMORY;

    for (size_t i = 0; i < size; ++i)
        new_atlas[i] = in->atlas[i];

    struct fimage atlas_image = {.data = new_atlas,
                                 .width = (int)in->atlas_width,
                                 .height = (int)in->atlas_height,
                                 .channels = 1};
    int error = fimage_save(&atlas_image, atlas_image_path);
    if (error)
        goto cleanup;

    ret = snprintf(meta_path, GLOBALS_PATH_MAX, "%s.meta", path);
    if (ret < 0) {
        error = CORE_FAILURE;
        goto cleanup;
    }

    FILE *file = fopen(meta_path, "wb");
    if (!file) {
        error = CORE_FAILURE;
        goto cleanup;
    }

    struct ffont_meta meta = {.atlas_width = in->atlas_width,
                              .atlas_height = in->atlas_height,
                              .pixel_size = in->pixel_size};
    unsigned long length = fwrite(&meta, sizeof(meta), 1, file);
    if (length < 1) {
        error = CORE_ACCESS_DENIED;
        goto cleanup;
    }

    length = fwrite(in->glyphs, sizeof(struct fglyph), FFONT_CHAR_COUNT, file);
    if (length < FFONT_CHAR_COUNT) {
        error = CORE_ACCESS_DENIED;
        goto cleanup;
    }

    if (fclose(file) != 0) {
        error = CORE_FAILURE;
        goto cleanup;
    }

    return CORE_SUCCESS;

cleanup:
    if (new_atlas) {
        free(new_atlas);
        new_atlas = NULL;
    }

    return error;
}

static int ffont_load(struct ffont *out, const char *path) {
    if (!out || !path)
        return CORE_NULLPTR;

    char atlas_image_path[GLOBALS_PATH_MAX] = {0};
    char meta_path[GLOBALS_PATH_MAX] = {0};

    int ret = snprintf(atlas_image_path, GLOBALS_PATH_MAX, "%s.png", path);
    if (ret < 0)
        return CORE_FAILURE;

    ret = snprintf(meta_path, GLOBALS_PATH_MAX, "%s.meta", path);
    if (ret < 0)
        return CORE_FAILURE;

    struct fimage atlas_image = {0};
    int error = fimage_init(&atlas_image, atlas_image_path);
    if (error)
        return error;

    FILE *file = fopen(meta_path, "rb");
    if (!file) {
        error = CORE_FILE_NOT_FOUND;
        goto cleanup;
    }

    struct ffont_meta meta = {0};

    if (fread(&meta, sizeof(meta), 1, file) != 1) {
        error = CORE_FILE_CORRUPT;
        goto cleanup;
    }

    if (fread(out->glyphs, sizeof(struct fglyph), FFONT_CHAR_COUNT, file) !=
        FFONT_CHAR_COUNT) {
        error = CORE_FILE_CORRUPT;
        goto cleanup;
    }

    (void)fclose(file);
    file = NULL;

    out->atlas_width = meta.atlas_width;
    out->atlas_height = meta.atlas_height;
    out->pixel_size = meta.pixel_size;

    size_t atlas_size = (size_t)atlas_image.width * (size_t)atlas_image.height;
    out->atlas = malloc(atlas_size);
    if (!out->atlas) {
        error = CORE_OUT_OF_MEMORY;
        goto cleanup;
    }

    memcpy(out->atlas, atlas_image.data, atlas_size);
    fimage_destroy(&atlas_image);
    out->face = NULL;
    return CORE_SUCCESS;

cleanup:
    if (file)
        (void)fclose(file);

    fimage_destroy(&atlas_image);
    return error;
}

int ffont_init(struct ffont *out, FT_Library ft, const char *path,
               int pixel_size) {
    if (!out || !ft || !path)
        return CORE_NULLPTR;

    char cache_path[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(cache_path, GLOBALS_PATH_MAX, "%s_%i", path, pixel_size);
    if (ret < 0)
        return CORE_FAILURE;

    int error = ffont_load(out, cache_path);
    if (!error)
        return CORE_SUCCESS;

    if (FT_New_Face(ft, path, 0, &out->face))
        return CORE_FILE_NOT_FOUND;

    FT_Set_Pixel_Sizes(out->face, 0, pixel_size);
    out->pixel_size = pixel_size;

    unsigned int total_area = 0;
    unsigned int max_width = 0;
    unsigned int max_height = 0;
    for (unsigned char c = FFONT_CHAR_START; c <= FFONT_CHAR_END; ++c) {
        if (FT_Load_Char(out->face, c, FT_LOAD_RENDER))
            continue;

        FT_Bitmap *bmp = &out->face->glyph->bitmap;
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
        out->atlas_width = atlas_size;
        out->atlas_height = atlas_size;

        unsigned int test_x = 0;
        unsigned int test_y = 0;
        unsigned int test_shelf_height = 0;
        packing_success = true;

        for (unsigned char c = FFONT_CHAR_START; c <= FFONT_CHAR_END; ++c) {
            if (FT_Load_Char(out->face, c, FT_LOAD_RENDER))
                continue;

            FT_Bitmap *bmp = &out->face->glyph->bitmap;

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
        return CORE_FAILURE;

    out->atlas = calloc((size_t)out->atlas_width * out->atlas_height, 1);
    if (!out->atlas)
        return CORE_OUT_OF_MEMORY;

    unsigned int shelf_x = 0;
    unsigned int shelf_y = 0;
    unsigned int shelf_height = 0;

    for (unsigned char c = FFONT_CHAR_START; c <= FFONT_CHAR_END; ++c) {
        if (FT_Load_Char(out->face, c, FT_LOAD_RENDER))
            continue;

        FT_GlyphSlot slot = out->face->glyph;
        unsigned int glyph_width = slot->bitmap.width;
        unsigned int glyph_height = slot->bitmap.rows;

        // If glyph does not fit on current shelf, move to next shelf
        if (shelf_x + glyph_width + FFONT_GLYPH_PADDING > out->atlas_width) {
            shelf_x = 0;
            shelf_y += shelf_height + FFONT_GLYPH_PADDING;
            shelf_height = 0;
        }

        // Copy glyph data to atlas
        for (unsigned int row = 0; row < glyph_height; ++row) {
            for (unsigned int col = 0; col < glyph_width; ++col) {
                unsigned int x = shelf_x + col;
                unsigned int y = shelf_y + row;
                out->atlas[y * out->atlas_width + x] =
                    slot->bitmap.buffer[row * slot->bitmap.pitch + col];
            }
        }

        // Store glyph metadata
        struct fglyph *glyph = &out->glyphs[c - FFONT_CHAR_START];
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

    error = ffont_save(out, cache_path);
    if (error)
        return error;

    return CORE_SUCCESS;
}

void ffont_destroy(struct ffont *in) {
    if (!in)
        return;

    if (in->atlas) {
        free(in->atlas);
        in->atlas = NULL;
    }

    if (in->face) {
        FT_Done_Face(in->face);
        in->face = NULL;
    }
}
