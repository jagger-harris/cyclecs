#ifndef APP_TIME_H
#define APP_TIME_H

#include "core/util/err.h"

struct time {
    float delta;
    float total;
    double last_frame;
};

err time_init(struct time *out);
err time_update(struct time *in);

#endif // APP_TIME_H
