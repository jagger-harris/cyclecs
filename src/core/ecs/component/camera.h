#ifndef ECS_COMPONENT_CAMERA_H
#define ECS_COMPONENT_CAMERA_H

#include "cglm/affine.h"
#include "cglm/cam.h"
#include "cglm/types.h"
#include "core/util/error.h"
#include <stdbool.h>

enum camera_type { CAMERA_PERSP, CAMERA_ORTHO };

struct camera_persp {
    float fov;
    float aspect_ratio;
};

struct camera_ortho {
    float left;
    float right;
    float bottom;
    float top;
    bool y_down;
};

struct camera {
    union {
        struct camera_persp persp;
        struct camera_ortho ortho;
    };
    mat4 view;
    mat4 projection;
    vec3 pos;
    vec3 rot_axis;
    enum camera_type type;
    float zoom;
    float near_clip;
    float far_clip;
    float rot_angle;
    bool active;
    bool update;
};

int camera_update(struct camera *in);
void camera_resize(struct camera *camera, ivec2 size);
int camera_set_pos(struct camera *camera, vec3 pos);
int camera_move(struct camera *camera, vec3 offset);
int camera_screen_to_world(vec2 out, struct camera *camera,
                           const vec2 screen_pos, const ivec2 viewport_size);

#endif // ECS_COMPONENT_CAMERA_H
