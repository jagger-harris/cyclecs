#ifndef IO_FIMAGE_H
#define IO_FIMAGE_H

struct fimage {
    int width;
    int height;
    int channels;
    unsigned char *data;
};

int fimage_init(struct fimage *out, const char *path);
void fimage_destroy(struct fimage *in);

#endif // IO_FIMAGE_H
