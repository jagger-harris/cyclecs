#include "core/io/fimg.h"
#include "core/util/logger.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STR_MAX 512

struct fimg {
    int width;
    int height;
    int channels;
    stbi_uc *data;
};

err fimg_new(fimg **out, const char *path) {
    err err = CORE_SUCCESS;
    char msg[STR_MAX] = {0};

    if (!path) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    /* TODO: Use arena/other allocator if possible */
    *out = malloc(sizeof(fimg));
    if (!(*out)) {
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    (*out)->data = NULL;
    (*out)->data =
        stbi_load(path, &(*out)->width, &(*out)->height, &(*out)->channels, 0);

    if (!(*out)->data) {
        err = CORE_FAILURE;
        goto err;
    }

    return err;

err:
    snprintf(msg, STR_MAX, "Failed to read image: %s", path);
    logger_log(LOGGER_ERR, msg, err);

    if (*out)
        free(*out);

    return err;
}

err fimg_delete(fimg *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    stbi_image_free(in->data);
    free(in);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete img", err);
    return err;
}

err fimg_get_dims(int *width, int *height, fimg *in) {
    err err = CORE_SUCCESS;

    if (!in || (!width && !height)) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (width)
        *width = in->width;

    if (height)
        *height = in->height;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get img dimensions", err);
    return err;
}

err fimg_get_channels(int *out, fimg *in) {
    err err = CORE_SUCCESS;

    if (!out || !in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = in->channels;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get img channels", err);
    return err;
}

err fimg_get_data(unsigned char **out, fimg *in) {
    err err = CORE_SUCCESS;

    if (!out || !in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = in->data;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get img channels", err);
    return err;
}
