#ifndef CLS_GFX_API_H
#define CLS_GFX_API_H

#include <cglm/types.h>

typedef struct GLFWwindow GLFWwindow;
struct cls_app;
struct cls_array;
struct cls_renderer_ctx;
struct cls_shader;
struct cls_shader_info;
struct cls_texture2d;
struct cls_texture2d_info;

struct cls_gfx_api {
    int (*init)(ivec4 bg_color);
    int (*swap_buffers)(GLFWwindow *win);
    void (*on_resize)(int width, int height);
    int (*draw_frame)(struct cls_app *app, struct cls_array *transparent_cmds,
                      struct cls_array *batches);
    int (*shader_init)(struct cls_shader *s,
                       const struct cls_shader_info *info);
    void (*shader_destroy)(struct cls_shader *s);
    int (*shader_use)(const struct cls_shader *s);
    int (*texture2d_init)(struct cls_texture2d *tex,
                          struct cls_texture2d_info *info);
    void (*texture2d_destroy)(struct cls_texture2d *tex);
    int (*texture2d_use)(const struct cls_texture2d *tex);
};

#endif // CLS_GFX_API_H
