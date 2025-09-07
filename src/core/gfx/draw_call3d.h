#ifndef GFX_DRAW_CALL3D_H
#define GFX_DRAW_CALL3D_H

enum draw_call3d_render_type {
    MESH3D,
    SPRITE3D,
    DRAW_CALL3D_RENDER_TYPE_COUNT
};

struct draw_call3d {
    struct renderable3d *renderable;
    struct transform3d *transform;
    enum draw_call3d_render_type type;
};

int draw_call3d_init(struct draw_call3d *out, enum draw_call3d_render_type type,
                     struct renderable3d *renderable,
                     struct transform3d *transform);

#endif // GFX_DRAW_CALL3D_H
