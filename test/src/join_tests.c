#include "chloros.h"
#include "test.h"

static void *double_arg(void *arg) {
  long input = (long)arg;
  return (void *)(input * 2);
}

static bool simple_join_test() {

  grn_init(true);
  int64_t id = grn_spawn(double_arg, (void *)21);
  int return_val = 0;
  grn_join(id, (void **)&return_val);
  check_eq(return_val, 42);

  id = grn_spawn(double_arg, (void *)100);
  grn_join(id, (void **)&return_val);

  check_eq(return_val, 200);
  return true;
}

static void *recurse(void *arg) {
  long input = (long)arg;
  if (input == 10)
    return (void *)10;

  int64_t id = grn_spawn(recurse, (void *)input + 1);
  int return_val = 0;
  grn_join(id, (void **)&return_val);

  return (void *)(return_val + input);
}

static bool nested_join_test() {

  grn_init(true);
  int64_t id = grn_spawn(recurse, (void *)1);
  int return_val = 0;
  grn_join(id, (void **)&return_val);

  check_eq(return_val, 55);

  return true;
}

BEGIN_TEST_SUITE(join_tests) {
  run_test(simple_join_test);
  run_test(nested_join_test);
}
