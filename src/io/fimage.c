#include <cls/io/fimage.h>
#include <cls/util/error.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

int fimage_init(struct fimage *img, const char *path) {
    if (!img || !path)
        return CLS_NULLPTR;

    img->data = stbi_load(path, &img->width, &img->height, &img->channels, 0);
    if (!img->data)
        return CLS_FILE_NOT_FOUND;

    return CLS_SUCCESS;
}

void fimage_destroy(struct fimage *img) {
    if (!img || !img->data)
        return;

    stbi_image_free(img->data);
    img->data = NULL;
}

int fimage_save(const struct fimage *img, const char *path) {
    if (!img || !img->data || !path)
        return CLS_NULLPTR;

    if (img->width < 1 || img->height < 1 || img->channels < 1)
        return CLS_INVALID_ARG;

    // Ensures stbi_write_png does not malloc(0)
    int stride = img->width * img->channels;
    if (stride < 1)
        return CLS_INVALID_ARG;

    int success = stbi_write_png(path, img->width, img->height, img->channels,
                                 img->data, img->width * img->channels);
    if (!success)
        return CLS_FAILURE;

    return CLS_SUCCESS;
}
