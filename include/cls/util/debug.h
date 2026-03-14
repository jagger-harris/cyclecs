#ifndef UTIL_DEBUG_H
#define UTIL_DEBUG_H

#define DEBUG_TIME_NOW(time)                                                   \
    struct timespec time;                                                      \
    clock_gettime(CLOCK_MONOTONIC, &time)

#define DEBUG_TIME_DIFF_MS(start, end)                                         \
    (((end.tv_sec - start.tv_sec) * 1000.0) +                                  \
     ((end.tv_nsec - start.tv_nsec) / 1e6))

#endif // UTIL_DEBUG_H
