#ifndef IO_FTXT_H
#define IO_FTXT_H

#include "core/util/err.h"

err ftxt_init(const char **out, const char *path);
void ftxt_destroy(const char *in);

#endif // IO_FTXT_H
