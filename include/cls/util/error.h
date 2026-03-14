#ifndef UTIL_ERROR_H
#define UTIL_ERROR_H

typedef enum {
    CLS_SUCCESS = 0,
    CLS_FAILURE,
    CLS_NULLPTR,
    CLS_OUT_OF_MEMORY,
    CLS_ACCESS_DENIED,
    CLS_INVALID_ARG,
    CLS_FILE_NOT_FOUND,
    CLS_FILE_CORRUPT,
    CLS_GLFW,
    CLS_GL,
} cls_error;

#endif // UTIL_ERROR_H
