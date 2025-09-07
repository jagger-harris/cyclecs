#ifndef ECS_COMPONENT_TRANSFORM3D_H
#define ECS_COMPONENT_TRANSFORM3D_H

#include <cglm/cglm.h>

struct transform3d {
    vec3 pos;
    vec3 rot_axis;
    vec3 scale;
    float rot_angle;
    float camera_distance;
};

#endif // ECS_COMPONENT_TRANSFORM3D_H
