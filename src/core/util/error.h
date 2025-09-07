#ifndef UTIL_ERROR_H
#define UTIL_ERROR_H

typedef enum {
    CORE_SUCCESS,
    CORE_FAILURE,
    CORE_NULLPTR,
    CORE_OUT_OF_MEMORY,
    CORE_INVALID_ARGS,
    CORE_NOT_FOUND,
    CORE_FILE,
    CORE_GLFW,
    CORE_GL,
} core_error;

#endif // UTIL_ERROR_H
