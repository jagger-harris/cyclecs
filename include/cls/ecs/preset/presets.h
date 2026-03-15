#ifndef CLS_PRESETS_H
#define CLS_PRESETS_H

#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>

struct preset_ui_image_button {
    struct ui_button_group grp;
    struct renderable ren;
    struct transform tf;
    struct ui_base base;
    struct ui_button button;
};

struct preset_ui_label {
    struct ui_label_group grp;
    struct renderable ren;
    struct transform tf;
    struct ui_base base;
    struct ui_label label;
};

struct assets;
struct ecs_world;

int preset_camera_ortho_spawn(entity *camera, struct ecs_world *world, vec3 pos,
                              float left, float right, float bottom, float top,
                              float zoom, float near_clip, float far_clip,
                              bool y_down, bool active);
int preset_renderable_spawn(entity *ren, struct ecs_world *world,
                            const char *mesh_id, const char *shader_id,
                            const char *texture2d_id, vec3 pos, vec3 scale,
                            float rot_angle, vec2 uv_offset, vec2 uv_scale,
                            ivec4 tint, bool visible, bool transparent);
int preset_rect_spawn(entity *rect, struct ecs_world *world,
                      const char *texture2d_id, vec2 pos, vec2 scale,
                      float rot_angle, float z_index, ivec4 tint,
                      vec2 uv_offset, vec2 uv_scale, bool visible);
int preset_sprite_spawn(entity *sprite, struct ecs_world *world,
                        const char *texture2d_id, vec2 pos, vec2 scale,
                        float rot_angle, float z_index, ivec4 tint,
                        vec2 uv_offset, vec2 uv_scale, bool visible);
int preset_ui_image_button_spawn(entity *button, struct ecs_world *world,
                                 const char *id, const char *image_id, vec2 pos,
                                 float z_index, vec2 scale, vec2 uv_offset,
                                 vec2 uv_scale, ivec4 image_tint, bool visible);
int preset_ui_label_spawn(entity *label, struct ecs_world *world,
                          struct assets *assets, const char *id, vec2 pos,
                          float z_index, const char *text, int font_size,
                          const char *font_id, bool visible, ivec4 tint);

#endif // CLS_PRESETS_H
