#ifndef ECS_COMP_TRANSFORM_H
#define ECS_COMP_TRANSFORM_H

#include <cglm/cglm.h>

struct transform_comp {
    vec3 pos;
    vec3 rot;
    vec3 scale;
    float rot_deg;
};

#endif // ECS_COMP_TRANSFORM_H
