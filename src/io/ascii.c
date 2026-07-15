#include <assert.h>
#include <cls/io/ascii.h>
#include <stdio.h>
#include <stdlib.h>

cls_error cls_ascii_init(const char **ascii, const char *path) {
    if (!ascii || !path)
        return CLS_FILE_NOT_FOUND;

    FILE *file = fopen(path, "rb");
    if (!file)
        return CLS_FILE_NOT_FOUND;

    cls_error error = CLS_SUCCESS;
    char *file_buffer = NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    long len = ftell(file);
    if (len < 0) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    size_t buffer_size = (size_t)len + 1;
    file_buffer = malloc(buffer_size);
    if (!file_buffer) {
        error = CLS_OUT_OF_MEMORY;
        goto cleanup;
    }

    size_t read_size = fread(file_buffer, 1, (size_t)len, file);
    if (read_size != (size_t)len) {
        error = CLS_ACCESS_DENIED;
        goto cleanup;
    }

    file_buffer[read_size] = '\0';
    *ascii = file_buffer;

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

void cls_ascii_destroy(const char **ascii) {
    if (!ascii)
        return;

    if (*ascii) {
        free((void *)*ascii);
        *ascii = NULL;
    }
}
