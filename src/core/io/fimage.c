#include "core/io/fimage.h"
#include "core/util/error.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int fimage_init(struct fimage *out, const char *path) {
    if (!path)
        return CORE_NULLPTR;

    out->data = NULL;
    out->data = stbi_load(path, &out->width, &out->height, &out->channels, 0);
    if (!out->data)
        return CORE_FAILURE;

    return CORE_SUCCESS;
}

void fimage_destroy(struct fimage *in) {
    if (!in)
        return;

    stbi_image_free(in->data);
}
