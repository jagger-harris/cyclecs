#include "core/io/fascii.h"
#include "core/util/error.h"
#include <stdio.h>
#include <stdlib.h>

int fascii_init(const char **out, const char *path) {
    FILE *file = fopen(path, "r");
    long length = 0;
    char *file_buffer = NULL;

    if (!file)
        return CORE_FILE;

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    file_buffer = (char *)malloc(length + 1);
    if (!file_buffer) {
        fclose(file);
        return CORE_OUT_OF_MEMORY;
    }

    fread(file_buffer, 1, length, file);
    file_buffer[length] = '\0';
    fclose(file);

    *out = file_buffer;
    return CORE_SUCCESS;
}

void fascii_destroy(const char *in) {
    if (!in)
        return;

    free((void *)in);
}
