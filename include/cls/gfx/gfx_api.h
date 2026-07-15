#ifndef CLS_GFX_API_H
#define CLS_GFX_API_H

#include <cglm/types.h>
#include <cls/util/error.h>

typedef struct GLFWwindow GLFWwindow;
struct cls_app;
struct cls_array;
struct cls_renderer_ctx;
struct cls_shader;
struct cls_shader_info;
struct cls_texture2d;
struct cls_texture2d_info;

struct cls_gfx_api {
    cls_error (*init)(ivec4 bg_color);
    cls_error (*swap_buffers)(GLFWwindow *win);
    void (*on_resize)(int width, int height);
    void (*begin_frame)(void);
    cls_error (*draw_batches)(struct cls_app *app, struct cls_array *cmds,
                              struct cls_array *batches,
                              struct cls_array **transparent_batches);
    cls_error (*shader_init)(struct cls_shader *s,
                             const struct cls_shader_info *info);
    void (*shader_destroy)(struct cls_shader *s);
    cls_error (*shader_use)(const struct cls_shader *s);
    cls_error (*texture2d_init)(struct cls_texture2d *tex,
                                struct cls_texture2d_info *info);
    void (*texture2d_destroy)(struct cls_texture2d *tex);
    cls_error (*texture2d_use)(const struct cls_texture2d *tex);
};

#endif // CLS_GFX_API_H
