#ifndef UTIL_ERR_H
#define UTIL_ERR_H

typedef int err;

enum core_err {
    CORE_SUCCESS,
    CORE_FAILURE,
    CORE_INVALID_NULLPTR,
    CORE_OUT_OF_MEMORY,
    CORE_INVALID_FILE,
    CORE_INVALID_ARGS,
    CORE_GLFW,
    CORE_INVALID_GFX_GL,
};

#endif /* UTIL_ERR_H */
