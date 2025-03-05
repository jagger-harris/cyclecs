#ifndef RSV_DARRAY_TEST
#define RSV_DARRAY_TEST

#include "test.h"
#include <stdio.h>

#define RSV_DARRAY_TYPE int
#include <rsv/containers/darray.h>

int test_darray(void) {
  const size_t amount = 1024;
  const size_t start_capacity = 8;

  // test case - insertion, get length, and deletion
  {
    int* array = NULL;
    rsv_darray_new_int(&array, start_capacity);

    for (size_t i = 0; i < amount; ++i) {
      rsv_darray_push_int(&array, i * 3);
    }

    size_t length = 0;
    rsv_darray_length_int(&array, &length);
    TEST(length == amount);

    for (size_t i = 0; i < length; ++i) {
      TEST(array[i] == (int)(i * 3));
    }

    int popped = 0;
    rsv_darray_pop_int(&array, &popped);
    TEST(popped == (int)((length - 1) * 3));

    rsv_darray_delete_int(&array);
  }

  return 0;
}

#endif // RSV_DARRAY_TEST
