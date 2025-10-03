#include "core/io/fimage.h"
#include "core/util/error.h"
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

int fimage_init(struct fimage *out, const char *path) {
    if (!out || !path)
        return CORE_NULLPTR;

    out->data = stbi_load(path, &out->width, &out->height, &out->channels, 0);
    if (!out->data)
        return CORE_FILE_NOT_FOUND;

    return CORE_SUCCESS;
}

void fimage_destroy(struct fimage *in) {
    if (!in || !in->data)
        return;

    stbi_image_free(in->data);
    in->data = NULL;
}

int fimage_save(const struct fimage *in, const char *path) {
    if (!in || !in->data || !path)
        return CORE_NULLPTR;

    if (in->width < 1 || in->height < 1 || in->channels < 1)
        return CORE_INVALID_ARG;

    // Ensures stbi_write_png does not malloc(0)
    int stride = in->width * in->channels;
    if (stride < 1)
        return CORE_INVALID_ARG;

    int success = stbi_write_png(path, in->width, in->height, in->channels,
                                 in->data, in->width * in->channels);
    if (!success)
        return CORE_FAILURE;

    return CORE_SUCCESS;
}
