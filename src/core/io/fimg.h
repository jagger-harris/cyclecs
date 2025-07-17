#ifndef IO_FIMG_H
#define IO_FIMG_H

#include "core/util/err.h"

struct fimg {
    int width;
    int height;
    int channels;
    unsigned char *data;
};

err fimg_init(struct fimg *out, const char *path);
void fimg_destroy(struct fimg *in);

#endif // IO_FIMG_H
