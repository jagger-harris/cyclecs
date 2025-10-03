#ifndef GFX_MATERIAL_H
#define GFX_MATERIAL_H

#include "core/util/globals.h"
#include <stdbool.h>

struct material {
    char shader_path[GLOBALS_PATH_MAX];
};

#endif // GFX_MATERIAL_H
