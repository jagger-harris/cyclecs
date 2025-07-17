#include "core/io/fimg.h"
#include "core/util/logger.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STR_MAX 512

err fimg_init(struct fimg *out, const char *path) {
    err status = CORE_SUCCESS;

    if (!path) {
        status = CORE_NULLPTR;
        goto err;
    }

    out->data = NULL;
    out->data = stbi_load(path, &out->width, &out->height, &out->channels, 0);

    if (!out->data) {
        status = CORE_FAILURE;
        goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Reading image failed: %s", path);
    return status;
}

void fimg_destroy(struct fimg *in) {
    if (!in)
        return;

    stbi_image_free(in->data);
}
