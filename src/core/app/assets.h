#ifndef APP_ASSETS_H
#define APP_ASSETS_H

#include "core/util/arena.h"
#include "core/util/err.h"

#define ASSETS_STR_MAX 64

typedef struct assets assets;

struct material {
    char shader_path[ASSETS_STR_MAX];
    char texture_path[ASSETS_STR_MAX];
};

err assets_new(assets **out, arena *mem);
err assets_delete(assets *in);
void assets_material_add(assets *in, int id, const char *shader_path,
                         const char *texture_path);
void assets_material_remove(assets *in, int id);

#endif /* APP_ASSETS_H */
