#ifndef CLS_PROFILER_H
#define CLS_PROFILER_H

#include <stdio.h>
#include <time.h>

#ifdef ENABLE_PROFILER

#include <sys/time.h>
static inline double profiler_get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

#define PROFILER_START(name) double _profiler_start_##name = profiler_get_time()

#define PROFILER_END(name)                                                     \
    do {                                                                       \
        double _profiler_end_##name = profiler_get_time();                     \
        double _profiler_elapsed_##name =                                      \
            (_profiler_end_##name - _profiler_start_##name) * 1000.0;          \
        printf("[PROFILE] %s: %.3f ms\n", #name, _profiler_elapsed_##name);    \
    } while (0)

#define _PROFILER_CLEANUP_IMPL(name)                                           \
    static inline void _profiler_cleanup_##name(double *start) {               \
        double end = profiler_get_time();                                      \
        double elapsed = (end - *start) * 1000.0;                              \
        printf("[PROFILE] %s: %.3f ms\n", #name, elapsed);                     \
    }

#define PROFILER_MAX_ENTRIES 128

struct profiler_entry {
    const char *name;
    double total_time;
    size_t call_count;
};

static struct profiler_entry _profiler_entries[PROFILER_MAX_ENTRIES] = {0};
static size_t _profiler_entry_count = 0;

static inline size_t profiler_get_or_create_entry(const char *name) {
    for (size_t i = 0; i < _profiler_entry_count; ++i) {
        if (_profiler_entries[i].name == name)
            return i;
    }

    if (_profiler_entry_count >= PROFILER_MAX_ENTRIES)
        return 0;

    size_t idx = _profiler_entry_count++;
    _profiler_entries[idx].name = name;
    _profiler_entries[idx].total_time = 0.0;
    _profiler_entries[idx].call_count = 0;
    return idx;
}

#define PROFILER_FUNC_START(name)                                              \
    size_t _profiler_idx_##name = profiler_get_or_create_entry(#name);         \
    double _profiler_func_start_##name = profiler_get_time()

#define PROFILER_FUNC_END(name)                                                \
    do {                                                                       \
        double _profiler_func_end_##name = profiler_get_time();                \
        double _profiler_func_elapsed_##name =                                 \
            _profiler_func_end_##name - _profiler_func_start_##name;           \
        _profiler_entries[_profiler_idx_##name].total_time +=                  \
            _profiler_func_elapsed_##name;                                     \
        _profiler_entries[_profiler_idx_##name].call_count++;                  \
    } while (0)

static inline void profiler_print_summary(void) {
    printf("\n=== PROFILE SUMMARY ===\n");
    printf("%-30s %12s %12s %12s\n", "Function", "Total (ms)", "Calls",
           "Avg (ms)");
    printf("-------------------------------------------------------------------"
           "\n");

    for (size_t i = 0; i < _profiler_entry_count; ++i) {
        struct profiler_entry *entry = &_profiler_entries[i];
        double total_ms = entry->total_time * 1000.0;
        double avg_ms = (entry->call_count > 0)
                            ? (total_ms / (double)entry->call_count)
                            : 0.0;

        printf("%-30s %12.3f %12zu %12.3f\n", entry->name, total_ms,
               entry->call_count, avg_ms);
    }
    printf("======================\n\n");
}

static inline void profiler_reset(void) {
    for (size_t i = 0; i < _profiler_entry_count; ++i) {
        _profiler_entries[i].total_time = 0.0;
        _profiler_entries[i].call_count = 0;
    }
}

#else

#define PROFILER_START(name) ((void)0)
#define PROFILER_END(name) ((void)0)
#define PROFILER_SCOPE(name) ((void)0)
#define PROFILER_FUNC_START(name) ((void)0)
#define PROFILER_FUNC_END(name) ((void)0)
#define profiler_print_summary() ((void)0)
#define profiler_reset() ((void)0)

#endif // ENABLE_PROFILER

#endif // CLS_PROFILER_H
