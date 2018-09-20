#include <check.h>
#include <stdlib.h>

START_TEST(test_dummy_create) {
  ck_assert(0 == 0);
  ck_assert(1 == 1);
  ck_assert(1 != 0);
}
END_TEST

Suite* suite(void) {
  Suite* s = suite_create("dummy");
  TCase* tc_core = tcase_create("core");
  tcase_add_test(tc_core, test_dummy_create);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void) {
  int number_failed;
  Suite* s = suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
