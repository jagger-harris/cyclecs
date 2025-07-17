#ifndef UTIL_ERR_H
#define UTIL_ERR_H

typedef int err;

enum core_err {
    CORE_SUCCESS,
    CORE_FAILURE,
    CORE_NULLPTR,
    CORE_OUT_OF_MEMORY,
    CORE_FILE,
    CORE_ARGS,
    CORE_GLFW,
    CORE_GL,
};

#endif // UTIL_ERR_H
