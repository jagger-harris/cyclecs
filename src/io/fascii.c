#include <assert.h>
#include <cls/io/fascii.h>
#include <cls/util/error.h>
#include <stdio.h>
#include <stdlib.h>

int fascii_init(const char **out, const char *path) {
    if (!out || !path)
        return CLS_FILE_NOT_FOUND;

    FILE *file = fopen(path, "rb");
    if (!file)
        return CLS_FILE_NOT_FOUND;

    int error = CLS_SUCCESS;
    char *file_buffer = NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    long length = ftell(file);
    if (length < 0) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    size_t buffer_size = (size_t)length + 1;
    file_buffer = malloc(buffer_size);
    if (!file_buffer) {
        error = CLS_OUT_OF_MEMORY;
        goto cleanup;
    }

    size_t read_size = fread(file_buffer, 1, buffer_size, file);
    if (read_size != (size_t)length) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    file_buffer[read_size] = '\0';
    *out = file_buffer;

    (void)fclose(file);
    file = NULL;
    return CLS_SUCCESS;

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
