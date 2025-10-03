#ifndef GFX_RENDERER_H
#define GFX_RENDERER_H

#include "cglm/types.h"
#include "core/gfx/color.h"
#include "core/util/array.h"
#include "core/util/table.h"

#define RENDERER_START_CMD_CAPACITY 128

struct app;
typedef struct GLFWwindow GLFWwindow;

struct renderer {
    struct gfx_api *api;
    struct color bg_color;
    struct array cmds;
    struct array batches;
    struct table cameras;
    struct renderer_camera *active_camera;
    size_t plugin_count;
};

struct renderer_plugin {
    const char *name;
    int (*create_cmds)(struct renderer *renderer, struct app *app);
};

int renderer_init(struct renderer *out, struct gfx_api *api,
                  struct color bg_color);
void renderer_destroy(struct renderer *in);

int renderer_draw_frame(struct renderer *in, struct app *app);
int renderer_swap_buffers(struct renderer *in, GLFWwindow *window);
int renderer_on_resize(struct renderer *in, int width, int height);

int renderer_add_camera_ortho(struct renderer *in, const char *camera_id,
                              vec3 pos, float left, float right, float bottom,
                              float top, float zoom, float near_clip,
                              float far_clip);
int renderer_remove_camera(struct renderer *in, const char *camera_id);
int renderer_active_camera_get(struct renderer_camera **out,
                               struct renderer *in);
int renderer_active_camera_set(struct renderer *in, const char *camera_id);
int renderer_active_camera_pos_set(struct renderer *in, vec3 pos);
int renderer_active_camera_move(struct renderer *in, vec3 offset);
int renderer_active_camera_update(struct renderer *in);

int renderer_screen_to_world(vec2 out, const struct renderer *in,
                             const vec2 screen_pos, const ivec2 viewport_size);

#endif // GFX_RENDERER_H
