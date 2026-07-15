#ifndef CLS_IMAGE_H
#define CLS_IMAGE_H

#include <cls/util/error.h>
#include <cls/util/types.h>

/**
 * @struct cls_image
 * @brief Image.
 */
struct cls_image {
    int width;
    int height;
    int channels;
    u8 *data;
};

/**
 * @brief Loads an image.
 *
 * Loads and decodes an image from `path`. Destroy the returned image data
 * with cls_image_destroy().
 *
 * @param[out] img  Image.
 * @param[in]  path Image file path.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR        If `img` or `path` is NULL.
 * @retval CLS_FILE_NOT_FOUND If loading the image fails.
 *
 * @code
 * struct cls_image img;
 * cls_image_init(&img, "texture.png");
 * // Use img.
 * cls_image_destroy(&img);
 * @endcode
 */
cls_error cls_image_init(struct cls_image *img, const char *path);

/**
 * @brief Destroys an image.
 *
 * Releases the image pixel data.
 *
 * @param[in] img Image to destroy.
 */
void cls_image_destroy(struct cls_image *img);

/**
 * @brief Saves an image.
 *
 * Writes the image data to `path` as a PNG file.
 *
 * @param[in] img  Image to save.
 * @param[in] path Output file path.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `img`, `img->data`, or `path` is NULL.
 * @retval CLS_INVALID_ARG If the image size or channel count is invalid.
 * @retval CLS_FAILURE     If writing the image fails.
 */
cls_error cls_image_save(const struct cls_image *img, const char *path);

#endif // CLS_IMAGE_H
