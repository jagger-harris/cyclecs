#include "core/io/fascii.h"
#include "core/util/error.h"
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>

int fascii_init(const char **out, const char *path) {
    if (!out || !path)
        return CORE_FILE_NOT_FOUND;

    FILE *file = fopen(path, "rb");
    if (!file)
        return CORE_FILE_NOT_FOUND;

    int error = CORE_SUCCESS;
    char *file_buffer = NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        error = CORE_ACCESS_DENIED;
        goto cleanup;
    }

    long length = ftell(file);
    if (length < 0) {
        error = CORE_ACCESS_DENIED;
        goto cleanup;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        error = CORE_ACCESS_DENIED;
        goto cleanup;
    }

    file_buffer = malloc((size_t)length + 1);
    if (!file_buffer) {
        error = CORE_OUT_OF_MEMORY;
        goto cleanup;
    }

    size_t read_size = fread(file_buffer, 1, (size_t)length, file);
    if (read_size != (size_t)length) {
        error = CORE_ACCESS_DENIED;
        goto cleanup;
    }

    file_buffer[length] = '\0';
    *out = file_buffer;

    (void)fclose(file);
    file = NULL;
    return CORE_SUCCESS;

cleanup:
    if (file_buffer) {
        free(file_buffer);
        file_buffer = NULL;
    }

    if (file)
        (void)fclose(file);

    return error;
}

void fascii_destroy(const char **in) {
    if (!in)
        return;

    if (*in) {
        free((void *)*in);
        *in = NULL;
    }
}
