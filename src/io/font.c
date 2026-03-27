#include <cls/io/font.h>
#include <cls/io/image.h>
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

#define FONT_MIN_ATLAS_SIZE 128
#define FONT_MAX_ATLAS_SIZE 2048
#define FONT_GLYPH_PADDING 2 // Prevents texture bleeding
#define FONT_SDF_SPREAD 8 // SDF spread in pixels

struct cls_font_meta {
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

static int font_save(const struct cls_font *f, const char *path) {
    if (!f || !path)
        return CLS_NULLPTR;

    char atlas_image_path[CLS_GLOBALS_PATH_MAX] = {0};
    char meta_path[CLS_GLOBALS_PATH_MAX] = {0};

    int ret = snprintf(atlas_image_path, CLS_GLOBALS_PATH_MAX, "%s.png", path);
    if (ret < 0)
        return CLS_FAILURE;

    size_t size = (size_t)f->atlas_width * (size_t)f->atlas_height;
    u8 *atlas = malloc(size);
    if (!atlas)
        return CLS_OUT_OF_MEMORY;

    for (size_t i = 0; i < size; ++i)
        atlas[i] = f->atlas[i];

    struct cls_image atlas_image = {.data = atlas,
                                    .width = (int)f->atlas_width,
                                    .height = (int)f->atlas_height,
                                    .channels = 1};
    int error = cls_image_save(&atlas_image, atlas_image_path);
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

    struct cls_font_meta meta = {.atlas_width = f->atlas_width,
                                 .atlas_height = f->atlas_height,
                                 .pixel_size = f->pixel_size,
                                 .sdf_spread = FONT_SDF_SPREAD};
    unsigned long length = fwrite(&meta, sizeof(meta), 1, file);
    if (length < 1) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    length =
        fwrite(f->glyphs, sizeof(struct cls_glyph), CLS_FONT_CHAR_LENGTH, file);
    if (length < CLS_FONT_CHAR_LENGTH) {
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

static int font_load(struct cls_font *f, const char *path) {
    if (!f || !path)
        return CLS_NULLPTR;

    char atlas_image_path[CLS_GLOBALS_PATH_MAX] = {0};
    char meta_path[CLS_GLOBALS_PATH_MAX] = {0};

    int ret = snprintf(atlas_image_path, CLS_GLOBALS_PATH_MAX, "%s.png", path);
    if (ret < 0)
        return CLS_FAILURE;

    ret = snprintf(meta_path, CLS_GLOBALS_PATH_MAX, "%s.meta", path);
    if (ret < 0)
        return CLS_FAILURE;

    struct cls_image atlas_image = {0};
    int error = cls_image_init(&atlas_image, atlas_image_path);
    if (error)
        return error;

    FILE *file = fopen(meta_path, "rb");
    if (!file) {
        error = CLS_FILE_NOT_FOUND;
        goto cleanup;
    }

    struct cls_font_meta meta = {0};

    if (fread(&meta, sizeof(meta), 1, file) != 1) {
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    if (fread(f->glyphs, sizeof(struct cls_glyph), CLS_FONT_CHAR_LENGTH,
              file) != CLS_FONT_CHAR_LENGTH) {
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    (void)fclose(file);
    file = NULL;

    f->atlas_width = meta.atlas_width;
    f->atlas_height = meta.atlas_height;
    f->pixel_size = meta.pixel_size;

    size_t atlas_size = (size_t)atlas_image.width * (size_t)atlas_image.height;
    f->atlas = malloc(atlas_size);
    if (!f->atlas) {
        error = CLS_OUT_OF_MEMORY;
        goto cleanup;
    }

    memcpy(f->atlas, atlas_image.data, atlas_size);
    cls_image_destroy(&atlas_image);
    f->face = NULL;
    return CLS_SUCCESS;

cleanup:
    if (file)
        (void)fclose(file);

    cls_image_destroy(&atlas_image);
    return error;
}

int cls_font_init(struct cls_font *f, FT_Library ft, const char *path,
                  int pixel_size) {
    if (!f || !ft || !path)
        return CLS_NULLPTR;

    char cache_path[CLS_GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(cache_path, CLS_GLOBALS_PATH_MAX, "%s_%i_sdf", path,
                       pixel_size);
    if (ret < 0)
        return CLS_FAILURE;

    int error = font_load(f, cache_path);
    if (!error)
        return CLS_SUCCESS;

    if (FT_New_Face(ft, path, 0, &f->face))
        return CLS_FILE_NOT_FOUND;

    FT_Set_Pixel_Sizes(f->face, 0, (FT_UInt)pixel_size);
    f->pixel_size = pixel_size;

    // Set SDF property for the face
    FT_Property_Set(ft, "sdf", "spread", &(FT_Int){FONT_SDF_SPREAD});

    unsigned int total_area = 0;
    unsigned int max_width = 0;
    unsigned int max_height = 0;
    for (u8 c = CLS_FONT_CHAR_START; c <= CLS_FONT_CHAR_END; ++c) {
        // Load with SDF render mode
        if (FT_Load_Char(f->face, c, FT_LOAD_DEFAULT))
            continue;

        if (FT_Render_Glyph(f->face->glyph, FT_RENDER_MODE_SDF))
            continue;

        FT_Bitmap *bmp = &f->face->glyph->bitmap;
        total_area += bmp->width * bmp->rows;

        if (bmp->width > max_width)
            max_width = bmp->width;

        if (bmp->rows > max_height)
            max_height = bmp->rows;
    }

    unsigned int atlas_size = next_pow2((unsigned int)sqrt(total_area * 1.5));
    if (atlas_size < FONT_MIN_ATLAS_SIZE)
        atlas_size = FONT_MIN_ATLAS_SIZE;
    if (atlas_size > FONT_MAX_ATLAS_SIZE)
        atlas_size = FONT_MAX_ATLAS_SIZE;

    // Pack font glyphs into texture atlas, keep trying until it fits
    bool packing_success = false;
    while (atlas_size <= FONT_MAX_ATLAS_SIZE && !packing_success) {
        f->atlas_width = atlas_size;
        f->atlas_height = atlas_size;

        unsigned int test_x = 0;
        unsigned int test_y = 0;
        unsigned int test_shelf_height = 0;
        packing_success = true;

        for (u8 c = CLS_FONT_CHAR_START; c <= CLS_FONT_CHAR_END; ++c) {
            if (FT_Load_Char(f->face, c, FT_LOAD_DEFAULT))
                continue;

            if (FT_Render_Glyph(f->face->glyph, FT_RENDER_MODE_SDF))
                continue;

            FT_Bitmap *bmp = &f->face->glyph->bitmap;

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

    // Could not fit in FONT_MAX_ATLAS_SIZE * FONT_MAX_ATLAS_SIZE
    if (!packing_success)
        return CLS_FAILURE;

    f->atlas = calloc((size_t)f->atlas_width * f->atlas_height, 1);
    if (!f->atlas)
        return CLS_OUT_OF_MEMORY;

    unsigned int shelf_x = 0;
    unsigned int shelf_y = 0;
    unsigned int shelf_height = 0;

    for (u8 c = CLS_FONT_CHAR_START; c <= CLS_FONT_CHAR_END; ++c) {
        if (FT_Load_Char(f->face, c, FT_LOAD_DEFAULT))
            continue;

        // Render glyph as SDF
        if (FT_Render_Glyph(f->face->glyph, FT_RENDER_MODE_SDF))
            continue;

        FT_GlyphSlot slot = f->face->glyph;
        unsigned int glyph_width = slot->bitmap.width;
        unsigned int glyph_height = slot->bitmap.rows;

        // If glyph does not fit on current shelf, move to next shelf
        if (shelf_x + glyph_width + FONT_GLYPH_PADDING > f->atlas_width) {
            shelf_x = 0;
            shelf_y += shelf_height + FONT_GLYPH_PADDING;
            shelf_height = 0;
        }

        // Copy SDF glyph data to atlas
        for (unsigned int row = 0; row < glyph_height; ++row) {
            for (unsigned int col = 0; col < glyph_width; ++col) {
                unsigned int x = shelf_x + col;
                unsigned int y = shelf_y + row;
                f->atlas[y * f->atlas_width + x] =
                    slot->bitmap
                        .buffer[row * (unsigned int)slot->bitmap.pitch + col];
            }
        }

        // Store glyph metadata
        struct cls_glyph *glyph = &f->glyphs[c - CLS_FONT_CHAR_START];
        glyph->width = glyph_width;
        glyph->height = glyph_height;
        glyph->bearing_x = slot->bitmap_left;
        glyph->bearing_y = slot->bitmap_top;
        glyph->advance = slot->advance.x >> 6;
        glyph->atlas_x = shelf_x;
        glyph->atlas_y = shelf_y;

        // Update shelf position
        shelf_x += glyph_width + FONT_GLYPH_PADDING;
        if (glyph_height > shelf_height)
            shelf_height = glyph_height;
    }

    error = font_save(f, cache_path);
    if (error)
        return error;

    return CLS_SUCCESS;
}

void cls_font_destroy(struct cls_font *f) {
    if (!f)
        return;

    if (f->atlas) {
        free(f->atlas);
        f->atlas = NULL;
    }

    if (f->face) {
        FT_Done_Face(f->face);
        f->face = NULL;
    }
}
