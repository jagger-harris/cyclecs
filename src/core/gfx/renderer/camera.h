#ifndef GFX_RENDERER_CAMERA_H
#define GFX_RENDERER_CAMERA_H

#include <cglm/types.h>
#include <stdbool.h>

enum renderer_camera_type { CAMERA_PERSP, CAMERA_ORTHO };

struct table;

struct renderer_camera_persp {
    float fov;
    float aspect_ratio;
};

struct renderer_camera_ortho {
    float left;
    float right;
    float bottom;
    float top;
};

struct renderer_camera {
    union {
        struct renderer_camera_persp persp;
        struct renderer_camera_ortho ortho;
    };
    mat4 view;
    mat4 projection;
    vec3 pos;
    vec3 rot_axis;
    enum renderer_camera_type type;
    float zoom;
    float near_clip;
    float far_clip;
    float rot_angle;
    bool active;
    bool update;
};

int renderer_camera_add_ortho(struct table *cameras, const char *camera_id,
                              vec3 pos, float left, float right, float bottom,
                              float top, float zoom, float near_clip,
                              float far_clip);
int renderer_camera_remove(struct table *cameras, const char *camera_id);
int renderer_camera_set_active(struct table *cameras,
                               struct renderer_camera **active_camera,
                               const char *camera_id);
int renderer_camera_update(struct renderer_camera *in);
void renderer_camera_on_resize(struct renderer_camera *camera, int width,
                               int height);
int renderer_camera_set_pos(struct renderer_camera *camera, vec3 pos);
int renderer_camera_move(struct renderer_camera *camera, vec3 offset);
int renderer_camera_screen_to_world(vec2 out, struct renderer_camera *camera,
                                    const vec2 screen_pos,
                                    const ivec2 viewport_size);
#endif // GFX_RENDERER_CAMERA_H
