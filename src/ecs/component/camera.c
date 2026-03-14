#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>

int camera_update(struct camera *in, struct transform *tf) {
    if (!in)
        return CLS_NULLPTR;

    in->update = false;

    switch (in->type) {
    case CAMERA_PERSP: {
        glm_perspective(in->persp.fov, in->persp.aspect_ratio, in->near_clip,
                        in->far_clip, in->projection);

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
        glm_lookat(tf->pos, target, up, in->view);
        break;
    }
    case CAMERA_ORTHO: {
        float zoom = in->zoom;
        if (zoom <= 0.0f)
            zoom = 0.0001f;

        float width = (in->ortho.right - in->ortho.left) / zoom;
        float height = (in->ortho.bottom - in->ortho.top) / zoom;

        float center_x = (in->ortho.right + in->ortho.left) * 0.5f;
        float center_y = (in->ortho.top + in->ortho.bottom) * 0.5f;

        float left = center_x - width * 0.5f;
        float right = center_x + width * 0.5f;
        float bottom = center_y + height * 0.5f;
        float top = center_y - height * 0.5f;

        glm_ortho(left, right, bottom, top, in->near_clip, in->far_clip,
                  in->projection);

        mat4 view = {{0.0f}};
        glm_mat4_identity(view);
        glm_translate(view, (vec3){-tf->pos[0], -tf->pos[1], -tf->pos[2]});
        glm_mat4_copy(view, in->view);
        break;
    }
    default:
        return CLS_INVALID_ARG;
    }

    return CLS_SUCCESS;
}

void camera_resize(struct camera *camera, ivec2 size) {
    if (!camera)
        return;

    camera->update = true;

    switch (camera->type) {
    case CAMERA_PERSP:
        camera->persp.aspect_ratio = (float)size[0] / (float)size[1];
        break;
    case CAMERA_ORTHO:
        camera->ortho.left = 0.0f;
        camera->ortho.right = (float)size[0];

        if (camera->ortho.y_down) {
            camera->ortho.top = 0.0f;
            camera->ortho.bottom = (float)size[1];
        } else {
            camera->ortho.top = (float)size[1];
            camera->ortho.bottom = 0.0f;
        }
        break;
    default:
        break;
    }
}

int camera_screen_to_world(vec2 out, struct camera *camera,
                           const vec2 cursor_pos, const ivec2 viewport_size) {
    if (!camera)
        return CLS_NULLPTR;

    if (viewport_size[0] <= 0 || viewport_size[1] <= 0)
        return CLS_INVALID_ARG;

    float ndc_x = (2.0f * cursor_pos[0]) / (float)viewport_size[0] - 1.0f;
    float ndc_y = 1.0f - (2.0f * cursor_pos[1]) / (float)viewport_size[1];
    vec4 ndc = {ndc_x, ndc_y, -1.0f, 1.0f};

    mat4 view_projection = {{0}};
    mat4 inverse_view_projection = {{0}};
    vec4 world = {0};
    glm_mat4_mul(camera->projection, camera->view, view_projection);
    glm_mat4_inv(view_projection, inverse_view_projection);
    glm_mat4_mulv(inverse_view_projection, ndc, world);

    out[0] = world[0] / world[3];
    out[1] = world[1] / world[3];
    return CLS_SUCCESS;
}
