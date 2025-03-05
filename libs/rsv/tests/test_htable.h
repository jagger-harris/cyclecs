#ifndef RSV_HTABLE_TEST
#define RSV_HTABLE_TEST

#include "test.h"
#include <stdio.h>

#define RSV_HTABLE_KEY_TYPE int
#define RSV_HTABLE_VALUE_TYPE int
#include <rsv/containers/htable.h>

int test_htable(void) {
  const size_t amount = 1024;
  const size_t table_size = 8;

  // test case - insert and find a single item
  {
    rsv_htable_int_int_entry* table = NULL;
    rsv_htable_new_int_int(&table, table_size);
    rsv_htable_insert_int_int(&table, 10, 100);

    int value = 0;
    rsv_htable_find_int_int(&table, 10, &value);
    TEST(value == 100);

    rsv_htable_delete_int_int(&table);
  }

  // test case - insert and find multiple items
  {
    rsv_htable_int_int_entry* table = NULL;
    rsv_htable_new_int_int(&table, table_size);

    for (size_t i = 0; i < amount; ++i) {
      rsv_htable_insert_int_int(&table, i, i * 2);
    }

    int value;
    for (size_t i = 0; i < amount; ++i) {
      rsv_htable_find_int_int(&table, i, &value);
      TEST(value == (int)i * 2);
    }

    rsv_htable_delete_int_int(&table);
  }

  // test case - remove a single item
  {
    rsv_htable_int_int_entry* table = NULL;
    rsv_htable_new_int_int(&table, table_size);
    rsv_htable_insert_int_int(&table, 10, 100);

    bool removed = 0;
    rsv_htable_remove_int_int(&table, 10, &removed);
    TEST(removed);

    int value = 0;
    rsv_htable_find_int_int(&table, 10, &value);
    TEST(!value);

    rsv_htable_delete_int_int(&table);
  }

  // test case - remove a non-existent key
  {
    rsv_htable_int_int_entry* table = NULL;
    rsv_htable_new_int_int(&table, table_size);
    rsv_htable_insert_int_int(&table, 1, 10);
    rsv_htable_insert_int_int(&table, 2, 20);
    rsv_htable_insert_int_int(&table, 3, 30);

    bool removed = 0;
    rsv_htable_remove_int_int(&table, 4, NULL);
    TEST(!removed);

    int value1 = 0;
    int value2 = 0;
    int value3 = 0;
    rsv_htable_find_int_int(&table, 1, &value1);
    rsv_htable_find_int_int(&table, 2, &value2);
    rsv_htable_find_int_int(&table, 3, &value3);
    TEST(value1 == 10);
    TEST(value2 == 20);
    TEST(value3 == 30);

    rsv_htable_delete_int_int(&table);
  }

  // test case - insert and shrink table
  {
    rsv_htable_int_int_entry* table = NULL;
    rsv_htable_new_int_int(&table, table_size);

    for (size_t i = 0; i < amount; ++i) {
      rsv_htable_insert_int_int(&table, i, i * 10);
    }

    size_t before_capacity = 0;
    rsv_htable_capacity_int_int(&table, &before_capacity);
    size_t length = 0;
    rsv_htable_length_int_int(&table, &length);

    for (size_t i = 0; i < (amount * 0.95); ++i) {
      rsv_htable_remove_int_int(&table, i, NULL);
    }

    size_t after_capacity = 0;
    rsv_htable_capacity_int_int(&table, &after_capacity);
    TEST(after_capacity < before_capacity);

    rsv_htable_delete_int_int(&table);
  }

  // test case - empty table lookup
  {
    rsv_htable_int_int_entry* table = NULL;
    rsv_htable_new_int_int(&table, table_size);
    int value = 0;
    rsv_htable_find_int_int(&table, 10, &value);
    TEST(!value);

    rsv_htable_delete_int_int(&table);
  }

  // test case - table with tombstones
  {
    rsv_htable_int_int_entry* table = NULL;
    rsv_htable_new_int_int(&table, table_size);
    rsv_htable_insert_int_int(&table, 10, 100);
    rsv_htable_remove_int_int(&table, 10,
                              NULL); // removes and sets tombstone
    int value = 0;
    rsv_htable_find_int_int(&table, 10, &value);
    TEST(!value); // should not find the tombstoned key

    rsv_htable_delete_int_int(&table);
  }

  // test case - multiple insertions and removals
  {
    rsv_htable_int_int_entry* table = NULL;
    rsv_htable_new_int_int(&table, table_size);
    rsv_htable_insert_int_int(&table, 34, 10);
    rsv_htable_insert_int_int(&table, 2, 20);
    rsv_htable_insert_int_int(&table, 3, 30);

    rsv_htable_remove_int_int(&table, 2, NULL); // remove an item

    int value1 = 0;
    int value3 = 0;
    rsv_htable_find_int_int(&table, 34, &value1);
    rsv_htable_find_int_int(&table, 3, &value3);
    TEST(value1 == 10);
    TEST(value3 == 30);

    rsv_htable_delete_int_int(&table);
  }

  return 0;
}

#endif // RSV_HTABLE_TEST
