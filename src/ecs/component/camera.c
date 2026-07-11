#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>

int camera_update(struct camera *cam, struct transform *tf) {
    if (!cam || !tf)
        return CLS_NULLPTR;

    vec3 forward = {0.0f, 0.0f, -1.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};

    mat4 rot = {{0.0f}};
    glm_mat4_identity(rot);
    glm_rotate(rot, tf->rot_angle, tf->rot_axis);

    vec4 fwd4 = {0.0f, 0.0f, -1.0f, 0.0f};
    vec4 up4 = {0.0f, 1.0f, 0.0f, 0.0f};
    glm_mat4_mulv(rot, fwd4, fwd4);
    glm_mat4_mulv(rot, up4, up4);

    glm_vec3(fwd4, forward);
    glm_vec3(up4, up);

    vec3 target;
    glm_vec3_add(tf->pos, forward, target);
    glm_lookat(tf->pos, target, up, cam->view);

    switch (cam->type) {
    case CAMERA_PERSP:
        glm_perspective(glm_rad(cam->persp.fov / cam->zoom),
                        cam->persp.aspect_ratio, cam->near_clip, cam->far_clip,
                        cam->projection);
        break;
    case CAMERA_ORTHO: {
        float z = cam->zoom;
        glm_ortho(cam->ortho.left / z, cam->ortho.right / z,
                  cam->ortho.bottom / z, cam->ortho.top / z, cam->near_clip,
                  cam->far_clip, cam->projection);
        break;
    }
    }

    return CLS_SUCCESS;
}

void camera_resize(struct camera *cam, ivec2 size) {
    if (!cam)
        return;

    cam->dirty = true;

    switch (cam->type) {
    case CAMERA_PERSP:
        cam->persp.aspect_ratio = (float)size[0] / (float)size[1];
        break;
    case CAMERA_ORTHO:
        cam->ortho.left = 0.0f;
        cam->ortho.right = (float)size[0];

        if (cam->ortho.y_down) {
            cam->ortho.top = 0.0f;
            cam->ortho.bottom = (float)size[1];
        } else {
            cam->ortho.top = (float)size[1];
            cam->ortho.bottom = 0.0f;
        }
        break;
    default:
        break;
    }
}

int camera_screen_to_world(vec2 pos, struct camera *cam, const vec2 cursor_pos,
                           const ivec2 viewport_size) {
    if (!cam)
        return CLS_NULLPTR;

    if (viewport_size[0] <= 0 || viewport_size[1] <= 0)
        return CLS_INVALID_ARG;

    float ndc_x = (2.0f * cursor_pos[0]) / (float)viewport_size[0] - 1.0f;
    float ndc_y = 1.0f - (2.0f * cursor_pos[1]) / (float)viewport_size[1];
    vec4 ndc = {ndc_x, ndc_y, -1.0f, 1.0f};

    mat4 view_projection = {{0}};
    mat4 inverse_view_projection = {{0}};
    vec4 world = {0};
    glm_mat4_mul(cam->projection, cam->view, view_projection);
    glm_mat4_inv(view_projection, inverse_view_projection);
    glm_mat4_mulv(inverse_view_projection, ndc, world);

    pos[0] = world[0] / world[3];
    pos[1] = world[1] / world[3];
    return CLS_SUCCESS;
}
