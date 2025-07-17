#ifndef GFX_CAMERA_H
#define GFX_CAMERA_H

#include "cglm/types.h"
#include <stdbool.h>

struct camera {
    mat4 view;
    mat4 projection;
    vec3 pos;
    vec3 rot;
    float fov;
    float near_clip;
    float far_clip;
    float aspect_ratio;
    float ortho_size;
    bool orthographic;
    bool active;
    bool update;
};

#endif // GFX_CAMERA_H
