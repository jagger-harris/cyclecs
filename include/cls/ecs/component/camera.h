#ifndef ECS_COMPONENT_CAMERA_H
#define ECS_COMPONENT_CAMERA_H

#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/types.h>
#include <cls/util/error.h>
#include <cls/util/types.h>
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
    enum camera_type type;
    float zoom;
    float near_clip;
    float far_clip;
    float rot_angle;
    bool update;
};

struct camera_active {
    u8 _;
};

struct transform;

int camera_update(struct camera *in, struct transform *tf);
void camera_resize(struct camera *camera, ivec2 size);
int camera_screen_to_world(vec2 out, struct camera *camera,
                           const vec2 screen_pos, const ivec2 viewport_size);

#endif // ECS_COMPONENT_CAMERA_H
