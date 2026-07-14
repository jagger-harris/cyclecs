#ifndef CLS_STRING_H
#define CLS_STRING_H

/**
 * @brief Formats a string into a newly allocated, null-terminated buffer.
 *
 * @param[in] fmt Format string.
 * @param[in] ... Format arguments.
 *
 * @return Formatted string, or NULL on failure. Caller must free().
 *
 * @warning `fmt` must not be NULL.
 */
char *cls_str_fmt(const char *fmt, ...);

#endif // CLS_STRING_H
