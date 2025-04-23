#include "core/io/ftxt.h"
#include "core/util/logger.h"
#include <stdio.h>
#include <stdlib.h>

#define STR_MAX 512

err ftxt_new(const char **out, const char *path) {
    err err = CORE_SUCCESS;
    char msg[STR_MAX] = {0};
    FILE *file = fopen(path, "r");
    long length = 0;
    char *file_buffer = NULL;

    if (!file) {
        err = CORE_INVALID_FILE;
        goto err;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* TODO: Use arena/better allocator if possible */
    file_buffer = (char *)malloc(length + 1);
    if (!file_buffer) {
        fclose(file);
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    fread(file_buffer, 1, length, file);
    file_buffer[length] = '\0';
    fclose(file);

    *out = file_buffer;

    return err;

err:
    snprintf(msg, STR_MAX, "Failed to read text file: %s", path);
    logger_log(LOGGER_ERR, msg, err);
    return err;
}

/* TODO: Use arena if possible */
err ftxt_delete(const char *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    free((void *)in);

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete text file data", err);
    return err;
}
