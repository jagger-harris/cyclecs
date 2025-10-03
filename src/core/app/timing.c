#include "core/app/timing.h"
#include "core/util/error.h"
#include <GLFW/glfw3.h>
#include <stdalign.h>

int timing_init(struct timing *out) {
    if (!out)
        return CORE_NULLPTR;

    *out = (struct timing){.last_frame = glfwGetTime()};
    return CORE_SUCCESS;
}

int timing_update(struct timing *in) {
    if (!in)
        return CORE_NULLPTR;

    double now = glfwGetTime();
    in->delta_time = (float)(now - in->last_frame);
    in->total += in->delta_time;
    in->last_frame = now;

    if (in->delta_time > 0.0f)
        in->fps = 1.0f / in->delta_time;

    in->frame_count++;
    in->fps_timer += in->delta_time;

    if (in->fps_timer >= 1.0f) {
        in->fps_avg = (float)in->frame_count / in->fps_timer;
        in->frame_count = 0;
        in->fps_timer = 0.0f;
    }

    return CORE_SUCCESS;
}
