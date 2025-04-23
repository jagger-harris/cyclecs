#ifndef IO_FTXT_H
#define IO_FTXT_H

#include "core/util/err.h"

err ftxt_new(const char **out, const char *path);
err ftxt_delete(const char *in);

#endif /* IO_FTXT_H */
