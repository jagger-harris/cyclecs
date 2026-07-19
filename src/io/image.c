/**
 * @file cls/io/image.c
 * @brief Image management for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/io/image.h
 */

#include <cls/io/image.h>
#include <cls/util/error.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

cls_error cls_image_init(struct cls_image *img, const char *path) {
    if (!img || !path)
        return CLS_NULLPTR;

    img->data = stbi_load(path, &img->width, &img->height, &img->channels, 0);
    if (!img->data)
        return CLS_FILE_NOT_FOUND;

    return CLS_SUCCESS;
}

void cls_image_destroy(struct cls_image *img) {
    if (!img || !img->data)
        return;

    stbi_image_free(img->data);
    img->data = NULL;
}

cls_error cls_image_save(const struct cls_image *img, const char *path) {
    if (!img || !img->data || !path)
        return CLS_NULLPTR;

    if (img->width < 1 || img->height < 1 || img->channels < 1)
        return CLS_INVALID_ARG;

    // Ensures stbi_write_png does not malloc(0)
    int stride = img->width * img->channels;
    if (stride < 1)
        return CLS_INVALID_ARG;

    int success = stbi_write_png(path, img->width, img->height, img->channels,
                                 img->data, stride);
    if (!success)
        return CLS_FAILURE;

    return CLS_SUCCESS;
}
