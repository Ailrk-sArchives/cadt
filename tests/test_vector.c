#include "unity.h"
#include "../vector.h"
#include "../cadt.h"

void Setup() {
}

void tearDown() {
}

void test_CADT_Vec_new() {
  CADT_Vec *vector = CADT_Vec_new(10, sizeof(int));
  TEST_ASSERT_EQUAL(10, vector->size);
  TEST_ASSERT_GREATER_THAN(vector->size, vector->len);
  TEST_ASSERT_NULL(vector->buf);
  TEST_ASSERT_EQUAL(sizeof(int), vector->memsz);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_CADT_Vec_new);
  return UNITY_END();
}
