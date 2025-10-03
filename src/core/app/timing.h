#ifndef APP_TIMING_H
#define APP_TIMING_H

struct timing {
    float delta_time;
    float total;
    double last_frame;
    int frame_count;
    float fps;
    float fps_avg;
    float fps_timer;
};

int timing_init(struct timing *out);
int timing_update(struct timing *in);
int timing_get_fps(float *out, const struct timing *in);
int timing_get_fps_avg(float *out, const struct timing *in);

#endif // APP_TIMING_H
