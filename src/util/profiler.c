/**
 * @file cls/util/profiler.c
 * @brief Profiler utils for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/profiler.h
 */

#include <assert.h>
#include <cls/util/profiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { CLS_PROFILER_MAX_ENTRIES = 128 };

struct profiler_entry {
    const char *name;
    double total_time;
    size_t call_count;
};

static struct profiler_entry g_entries[CLS_PROFILER_MAX_ENTRIES];
static size_t g_entry_count = 0;

static double cls_profiler_get_time(void) {
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static size_t cls_profiler_get_or_create_entry(const char *name) {
    assert(name && "name is NULL");

    for (size_t i = 0; i < g_entry_count; ++i) {
        if (strcmp(g_entries[i].name, name) == 0)
            return i;
    }

    if (g_entry_count >= CLS_PROFILER_MAX_ENTRIES) {
        (void)fprintf(
            stderr, "cls_profiler: max entries (%d) exceeded, dropping '%s'\n",
            CLS_PROFILER_MAX_ENTRIES, name);
        return CLS_PROFILER_MAX_ENTRIES - 1;
    }

    size_t idx = g_entry_count++;
    g_entries[idx].name = name;
    g_entries[idx].total_time = 0.0;
    g_entries[idx].call_count = 0;
    return idx;
}

struct cls_profiler_timer cls_profiler_start(const char *name) {
    struct cls_profiler_timer t = {0};
    t.entry_idx = cls_profiler_get_or_create_entry(name);
    t.start_time = cls_profiler_get_time();
    return t;
}

void cls_profiler_end(struct cls_profiler_timer timer) {
    double elapsed = cls_profiler_get_time() - timer.start_time;
    g_entries[timer.entry_idx].total_time += elapsed;
    g_entries[timer.entry_idx].call_count++;
}

void cls_profiler_print_elapsed(const char *name,
                                struct cls_profiler_timer timer) {
    double elapsed_ms = (cls_profiler_get_time() - timer.start_time) * 1000.0;
    printf("[PROFILE] %s: %.3f ms\n", name, elapsed_ms);
}

void cls_profiler_print_summary(void) {
    printf("\n=== PROFILE SUMMARY ===\n");
    printf("%-30s %12s %12s %12s\n", "Function", "Total (ms)", "Calls",
           "Avg (ms)");
    printf("-------------------------------------------------------------------"
           "\n");

    for (size_t i = 0; i < g_entry_count; ++i) {
        struct profiler_entry *e = &g_entries[i];
        double total_ms = e->total_time * 1000.0;
        double avg_ms = e->call_count ? total_ms / (double)e->call_count : 0.0;
        printf("%-30s %12.3f %12zu %12.3f\n", e->name, total_ms, e->call_count,
               avg_ms);
    }
}

void cls_profiler_reset(void) {
    for (size_t i = 0; i < g_entry_count; ++i) {
        g_entries[i].total_time = 0.0;
        g_entries[i].call_count = 0;
    }
}

cls_error cls_profiler_mem_usage_get(size_t *mb) {
    if (!mb)
        return CLS_NULLPTR;

    FILE *f = fopen("/proc/self/status", "r");
    if (!f)
        return CLS_FILE_NOT_FOUND;

    char buffer[128] = {0};
    *mb = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        if (strncmp(buffer, "VmRSS:", 6) == 0) {
            char *p = buffer + 6;

            while (*p == ' ' || *p == '\t') {
                ++p;
            }

            char *endptr = NULL;
            int errno = 0;
            unsigned long kb = strtoul(p, &endptr, 10);

            if (endptr != p && errno == 0) {
                *mb = (size_t)kb / 1024;
            }

            break;
        }
    }

    (void)fclose(f);
    return CLS_SUCCESS;
}
