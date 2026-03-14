#ifndef GFX_API_H
#define GFX_API_H

#include <cglm/types.h>

typedef struct GLFWwindow GLFWwindow;
struct app;
struct array;
struct renderer_ctx;
struct shader;
struct shader_info;
struct texture2d;
struct texture2d_info;

struct gfx_api {
    int (*init)(ivec4 bg_color);
    int (*swap_buffers)(GLFWwindow *window);
    void (*on_resize)(int width, int height);
    int (*draw_frame)(struct app *app, struct array *render_batches);
    int (*shader_init)(struct shader *out, const struct shader_info *info);
    void (*shader_destroy)(struct shader *in);
    int (*shader_use)(const struct shader *in);
    int (*texture2d_init)(struct texture2d *out, struct texture2d_info *info);
    void (*texture2d_destroy)(struct texture2d *in);
    int (*texture2d_use)(const struct texture2d *in);
};

#endif // GFX_API_H
