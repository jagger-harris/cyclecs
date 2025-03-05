#ifndef RSV_TEST_H
#define RSV_TEST_H

#include "test_darray.h"
#include "test_htable.h"

int rsv_test_all(void) {
  int failed_tests = 0;

  failed_tests += test_darray();
  failed_tests += test_htable();

  return failed_tests;
}

int main(void) {
  int failed_tests = rsv_test_all();

  if (failed_tests) {
    printf("%s%d test(s) failed!%s\n", "\x1b[31m", failed_tests, "\x1b[0m");
    exit(EXIT_FAILURE);
  }

  printf("%sAll tests passed!%s\n", "\x1b[32m", "\x1b[0m");
  exit(EXIT_SUCCESS);
}

#endif // RSV_TEST_H
