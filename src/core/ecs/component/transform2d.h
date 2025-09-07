#ifndef ECS_COMPONENT_TRANSFORM2D_H
#define ECS_COMPONENT_TRANSFORM2D_H

#include <cglm/cglm.h>

struct transform2d {
    vec2 pos;
    vec2 scale;
    float rot_angle;
    float z_index;
};

#endif // ECS_COMPONENT_TRANSFORM2D_H
