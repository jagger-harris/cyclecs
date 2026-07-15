#ifndef CLS_PROFILER_H
#define CLS_PROFILER_H

#include <cls/util/error.h>
#include <stddef.h>

struct cls_profiler_timer {
    size_t entry_idx;
    double start_time;
};

/**
 * @brief Starts a profiler timer.
 *
 * Starts a timer for the named profiling section.
 *
 * @param[in] name Timer name.
 *
 * @return Profiler timer.
 */
struct cls_profiler_timer cls_profiler_start(const char *name);

/**
 * @brief Ends a profiler timer.
 *
 * Records the elapsed time for the timer.
 *
 * @param[in] timer Profiler timer.
 */
void cls_profiler_end(struct cls_profiler_timer timer);

/**
 * @brief Prints the elapsed time.
 *
 * Prints the elapsed time for a profiler timer.
 *
 * @param[in] name  Timer name.
 * @param[in] timer Profiler timer.
 */
void cls_profiler_print_elapsed(const char *name,
                                struct cls_profiler_timer timer);

/**
 * @brief Prints the profiler summary.
 *
 * Prints the recorded profiler data.
 */
void cls_profiler_print_summary(void);

/**
 * @brief Resets the profiler.
 *
 * Clears all recorded profiler data.
 */
void cls_profiler_reset(void);

/**
 * @brief Retrieves memory usage.
 *
 * Retrieves the current process memory usage in megabytes.
 *
 * @param[out] mb Memory usage.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `mb` is NULL.
 * @retval CLS_FILE_NOT_FOUND If reading the process status fails.
 */
cls_error cls_profiler_mem_usage_get(size_t *mb);

#endif // CLS_PROFILER_H
