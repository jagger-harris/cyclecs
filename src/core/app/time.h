#ifndef APP_TIME_H
#define APP_TIME_H

#include "core/util/arena.h"
#include "core/util/err.h"

typedef struct time time;

err time_new(time **out, arena *mem);
err time_get_dt(float *out, time *in);
err time_update(time *in);

#endif /* APP_TIME_H */
