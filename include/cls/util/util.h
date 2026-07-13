#ifndef CLS_UTIL_H
#define CLS_UTIL_H

/**
 * @def CLS_ARRAY_LENGTH
 * @brief Gets the length of an array.
 *
 * Calculates the number of elements in a fixed size array.
 *
 * @param a Array.
 *
 * @return Number of elements in `a`.
 *
 * @note Only works with arrays that have a compile time size. Passing a
 *       pointer will return an invalid result.
 *
 * @code
 * int values[10];
 * size_t len = CLS_ARRAY_LENGTH(values);
 * @endcode
 */
#define CLS_ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))

#endif // CLS_UTIL_H
