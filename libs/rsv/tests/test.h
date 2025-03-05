#ifndef TEST_H
#define TEST_H

#define TEST(condition)                                                        \
  if (!(condition)) {                                                          \
    printf("%sTest failed: %s (file: %s, line: %d)%s\n", "\x1b[31m",           \
           #condition, __FILE__, __LINE__, "\x1b[0m");                         \
    return 1;                                                                  \
  }

#endif // TEST_H
