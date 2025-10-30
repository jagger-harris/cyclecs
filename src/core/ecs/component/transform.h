#ifndef ECS_COMPONENT_TRANSFORM_H
#define ECS_COMPONENT_TRANSFORM_H

#include <cglm/cglm.h>

struct transform {
    vec3 origin;
    vec3 pos;
    vec3 scale;
    vec3 rot_axis;
    float rot_angle;
};

#endif // ECS_COMPONENT_TRANSFORM_H
