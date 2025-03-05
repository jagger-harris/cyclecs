#ifndef CORE_DEBUG_H
#define CORE_DEBUG_H

#define DEBUG_TIME_NOW(var)                                                    \
    struct timespec var;                                                       \
    clock_gettime(CLOCK_MONOTONIC, &var)
#define DEBUG_TIME_DIFF_MS(start, end)                                         \
    (((end.tv_sec - start.tv_sec) * 1000.0) +                                  \
     ((end.tv_nsec - start.tv_nsec) / 1e6))

#endif
