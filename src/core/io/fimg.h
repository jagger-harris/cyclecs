#ifndef IO_FIMG_H
#define IO_FIMG_H

#include "core/util/err.h"

typedef struct fimg fimg;

err fimg_new(fimg **out, const char *path);
err fimg_delete(fimg *in);
err fimg_get_dims(int *width, int *height, fimg *in);
err fimg_get_channels(int *out, fimg *in);
err fimg_get_data(unsigned char **out, fimg *in);

#endif /* IO_FIMG_H */
