#include "core/gfx/mesh.h"
#include "core/util/logger.h"

err mesh_new(mesh **out, array *vertices, array *indices) {
    err err = CORE_SUCCESS;

    if (!out || !vertices || !indices) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new mesh", err);
    return err;
}

err mesh_delete(mesh *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete mesh", err);
    return err;
}
