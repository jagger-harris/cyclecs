#include "core/io/ftxt.h"
#include "core/util/logger.h"
#include <stdio.h>
#include <stdlib.h>

#define STR_MAX 512

err ftxt_init(const char **out, const char *path) {
    err status = CORE_SUCCESS;
    FILE *file = fopen(path, "r");
    long length = 0;
    char *file_buffer = NULL;

    if (!file) {
        status = CORE_FILE;
        goto err;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // TODO: Use arena/better allocator if possible
    file_buffer = (char *)malloc(length + 1);
    if (!file_buffer) {
        fclose(file);
        status = CORE_OUT_OF_MEMORY;
        goto err;
    }

    fread(file_buffer, 1, length, file);
    file_buffer[length] = '\0';
    fclose(file);

    *out = file_buffer;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Reading text file failed: %s", path);
    return status;
}

void ftxt_destroy(const char *in) {
    if (!in)
        return;

    free((void *)in);
}
