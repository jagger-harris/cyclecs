#ifndef CLS_PROFILER_H
#define CLS_PROFILER_H

#include <cls/util/error.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static inline int cls_profiler_mem_usage_get(size_t *mb) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f)
        return CLS_FILE_NOT_FOUND;

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), f)) {
        if (strncmp(buffer, "VmRSS:", 6) == 0) {
            fclose(f);
            size_t kb = 0;
            sscanf(buffer, "VmRSS: %zu kB", &kb);
            *mb = kb / 1024;
            return CLS_SUCCESS;
        }
    }

    fclose(f);
    *mb = 0;
    return CLS_SUCCESS;
}

static inline double cls_profiler_get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

#define CLS_PROFILER_TIME_NOW(time)                                            \
    struct timespec time;                                                      \
    clock_gettime(CLOCK_MONOTONIC, &time)

#define CLS_PROFILER_TIME_DIFF_MS(start, end)                                  \
    (((end.tv_sec - start.tv_sec) * 1000.0) +                                  \
     ((end.tv_nsec - start.tv_nsec) / 1e6))

#define CLS_PROFILER_START(name)                                               \
    double _profiler_start_##name = cls_profiler_get_time()

#define CLS_PROFILER_END(name)                                                 \
    do {                                                                       \
        double _profiler_end_##name = cls_profiler_get_time();                 \
        double _profiler_elapsed_##name =                                      \
            (_profiler_end_##name - _profiler_start_##name) * 1000.0;          \
        printf("[PROFILE] %s: %.3f ms\n", #name, _profiler_elapsed_##name);    \
    } while (0)

#define _CLS_PROFILER_CLEANUP_IMPL(name)                                       \
    static inline void _cls_profiler_cleanup_##name(double *start) {           \
        double end = cls_profiler_get_time();                                  \
        double elapsed = (end - *start) * 1000.0;                              \
        printf("[PROFILE] %s: %.3f ms\n", #name, elapsed);                     \
    }

#define CLS_PROFILER_MAX_ENTRIES 128

struct cls_profiler_entry {
    const char *name;
    double total_time;
    size_t call_count;
};

static struct cls_profiler_entry _profiler_entries[CLS_PROFILER_MAX_ENTRIES] = {
    0};
static size_t _cls_profiler_entry_count = 0;

static inline size_t profiler_get_or_create_entry(const char *name) {
    for (size_t i = 0; i < _cls_profiler_entry_count; ++i) {
        if (_profiler_entries[i].name == name)
            return i;
    }

    if (_cls_profiler_entry_count >= CLS_PROFILER_MAX_ENTRIES)
        return 0;

    size_t idx = _cls_profiler_entry_count++;
    _profiler_entries[idx].name = name;
    _profiler_entries[idx].total_time = 0.0;
    _profiler_entries[idx].call_count = 0;
    return idx;
}

#define CLS_PROFILER_FUNC_START(name)                                          \
    size_t _profiler_idx_##name = profiler_get_or_create_entry(#name);         \
    double _profiler_func_start_##name = cls_profiler_get_time()

#define CLS_PROFILER_FUNC_END(name)                                            \
    do {                                                                       \
        double _profiler_func_end_##name = cls_profiler_get_time();            \
        double _profiler_func_elapsed_##name =                                 \
            _profiler_func_end_##name - _profiler_func_start_##name;           \
        _profiler_entries[_profiler_idx_##name].total_time +=                  \
            _profiler_func_elapsed_##name;                                     \
        _profiler_entries[_profiler_idx_##name].call_count++;                  \
    } while (0)

static inline void cls_profiler_print_summary(void) {
    printf("\n=== PROFILE SUMMARY ===\n");
    printf("%-30s %12s %12s %12s\n", "Function", "Total (ms)", "Calls",
           "Avg (ms)");
    printf("-------------------------------------------------------------------"
           "\n");

    for (size_t i = 0; i < _cls_profiler_entry_count; ++i) {
        struct cls_profiler_entry *entry = &_profiler_entries[i];
        double total_ms = entry->total_time * 1000.0;
        double avg_ms = (entry->call_count > 0)
                            ? (total_ms / (double)entry->call_count)
                            : 0.0;

        printf("%-30s %12.3f %12zu %12.3f\n", entry->name, total_ms,
               entry->call_count, avg_ms);
    }
    printf("======================\n\n");
}

static inline void cls_profiler_reset(void) {
    for (size_t i = 0; i < _cls_profiler_entry_count; ++i) {
        _profiler_entries[i].total_time = 0.0;
        _profiler_entries[i].call_count = 0;
    }
}

#endif // CLS_PROFILER_H
