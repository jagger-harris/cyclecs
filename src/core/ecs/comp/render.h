#ifndef COMP_RENDER_H
#define COMP_RENDER_H

#include "core/app/assets.h"
#include "core/ecs/comp/transform.h"

struct render {
    struct material material;
    struct transform *transform;
};

#endif /* COMP_RENDER_H */
