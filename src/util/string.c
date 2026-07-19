/**
 * @file cls/util/string.c
 * @brief String utils for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/util/string.h
 */

#include <assert.h>
#include <cls/util/string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *cls_str_fmt(const char *fmt, ...) {
    assert(fmt && "fmt is NULL");

    va_list args;
    va_list args_copy;
    va_start(args, fmt);
    va_copy(args_copy, args);

    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (needed < 0) {
        va_end(args);
        return NULL;
    }

    char *buf = malloc((size_t)needed + 1);
    if (buf)
        (void)vsnprintf(buf, (size_t)needed + 1, fmt, args);

    va_end(args);
    return buf;
}
