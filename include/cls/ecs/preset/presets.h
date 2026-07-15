#ifndef CLS_PRESETS_H
#define CLS_PRESETS_H

#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>

struct cls_preset_ui_image_button {
    struct renderable ren;
    struct transform tf;
    struct ui ui;
    struct button button;
    struct button_group grp;
};

struct cls_preset_ui_label {
    struct renderable ren;
    struct transform tf;
    struct ui ui;
    struct label label;
};

struct cls_assets;
struct cls_ecs_world;

cls_error cls_preset_group_despawn(cls_entity e, struct cls_ecs_world *world);
cls_error cls_preset_camera_ortho_spawn(cls_entity *camera,
                                        struct cls_ecs_world *world, vec3 pos,
                                        float left, float right, float bottom,
                                        float top, float zoom, float near_clip,
                                        float far_clip, bool y_down, int layer,
                                        bool active);
cls_error cls_preset_renderable_spawn(
    cls_entity *ren, struct cls_ecs_world *world, const char *mesh_id,
    const char *shader_id, const char *texture2d_id, vec3 pos, vec3 scale,
    float rot_angle, vec2 uv_offset, vec2 uv_scale, ivec4 tint, bool visible,
    bool depth_test, bool depth_write, bool blending, int blend_src,
    int blend_dest);
cls_error cls_preset_rect_spawn(cls_entity *rect, struct cls_ecs_world *world,
                                const char *texture2d_id, vec2 pos, vec2 scale,
                                float rot_angle, float z_index, ivec4 tint,
                                vec2 uv_offset, vec2 uv_scale, bool visible);
cls_error cls_preset_sprite_spawn(cls_entity *sprite,
                                  struct cls_ecs_world *world,
                                  const char *texture2d_id, vec2 pos,
                                  vec2 scale, float rot_angle, float z_index,
                                  ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                                  bool visible);
cls_error cls_preset_image_button_spawn(cls_entity *button,
                                        struct cls_ecs_world *world,
                                        const char *id, const char *image_id,
                                        vec2 pos, float z_index, vec2 scale,
                                        vec2 uv_offset, vec2 uv_scale,
                                        ivec4 image_tint, bool visible);
cls_error cls_preset_label_spawn(cls_entity *label, struct cls_ecs_world *world,
                                 const char *id, vec2 pos, float z_index,
                                 const char *text, int font_size,
                                 const char *font_id, bool visible, ivec4 tint);

#endif // CLS_PRESETS_H
