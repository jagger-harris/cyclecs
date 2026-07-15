#ifndef CLS_ECS_COMPONENT_CAMERA_H
#define CLS_ECS_COMPONENT_CAMERA_H

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
    vec3 forward;
    enum camera_type type;
    float zoom;
    float near_clip;
    float far_clip;
    float rot_angle;
    int layer;
    bool dirty;
};

struct transform;

cls_error camera_update(struct camera *cam, struct transform *tf);
void camera_resize(struct camera *cam, ivec2 size);
cls_error camera_screen_to_world(vec2 pos, struct camera *cam,
                                 const vec2 cursor_pos,
                                 const ivec2 viewport_size);

#endif // CLS_ECS_COMPONENT_CAMERA_H
