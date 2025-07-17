#ifndef GFX_MATERIAL_H
#define GFX_MATERIAL_H

#include "core/util/globals.h"
#include <stdbool.h>

struct material {
    char shader_path[ASSETS_STR_MAX];
    char texture_path[ASSETS_STR_MAX];
    bool transparent;
};

#endif // GFX_MATERIAL_H
