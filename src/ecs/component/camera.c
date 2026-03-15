#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>

int camera_update(struct camera *cam, struct transform *tf) {
    if (!cam)
        return CLS_NULLPTR;

    cam->update = false;

    switch (cam->type) {
    case CAMERA_PERSP: {
        glm_perspective(cam->persp.fov, cam->persp.aspect_ratio, cam->near_clip,
                        cam->far_clip, cam->projection);

        vec3 forward = {0.0f};
        vec3 target = {0.0f};
        vec3 up = {0.0f, 1.0f, 0.0f};
        float pitch = glm_rad(tf->rot_axis[0]);
        float yaw = glm_rad(tf->rot_axis[1]);

        forward[0] = cosf(pitch) * cosf(yaw);
        forward[1] = sinf(pitch);
        forward[2] = cosf(pitch) * sinf(yaw);

        glm_vec3_norm(forward);
        glm_vec3_add(tf->pos, forward, target);
        glm_lookat(tf->pos, target, up, cam->view);
        break;
    }
    case CAMERA_ORTHO: {
        float zoom = cam->zoom;
        if (zoom <= 0.0f)
            zoom = 0.0001f;

        float width = (cam->ortho.right - cam->ortho.left) / zoom;
        float height = (cam->ortho.bottom - cam->ortho.top) / zoom;

        float center_x = (cam->ortho.right + cam->ortho.left) * 0.5f;
        float center_y = (cam->ortho.top + cam->ortho.bottom) * 0.5f;

        float left = center_x - width * 0.5f;
        float right = center_x + width * 0.5f;
        float bottom = center_y + height * 0.5f;
        float top = center_y - height * 0.5f;

        glm_ortho(left, right, bottom, top, cam->near_clip, cam->far_clip,
                  cam->projection);

        mat4 view = {{0.0f}};
        glm_mat4_identity(view);
        glm_translate(view, (vec3){-tf->pos[0], -tf->pos[1], -tf->pos[2]});
        glm_mat4_copy(view, cam->view);
        break;
    }
    default:
        return CLS_INVALID_ARG;
    }

    return CLS_SUCCESS;
}

void camera_resize(struct camera *cam, ivec2 size) {
    if (!cam)
        return;

    cam->update = true;

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
