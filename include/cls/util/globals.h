#ifndef CLS_GLOBALS_H
#define CLS_GLOBALS_H

#include <cls/util/error.h>
#include <string.h>

enum {
    CLS_GLOBALS_PATH_MAX = 512, /**< Maximum length in bytes of a
                                   path. */
    CLS_GLOBALS_STR_ID_MAX = 64, /**< Maximum length in bytes of a string
                                    id before hashing. */
};

#endif // CLS_GLOBALS_H
