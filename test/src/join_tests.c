#include "chloros.h"
#include "test.h"

static void *double_arg(void *arg) {
  long input = (long)arg;
  return (void *)(input * 2);
}

static bool simple_join_test() {

  grn_init(false);
  int64_t id = grn_spawn(double_arg, (void *)21);
  int return_val = 0;
  grn_join(id, (void **)&return_val);

  id = grn_spawn(double_arg, (void *)100);
  grn_join(id, (void **)&return_val);

  check_eq(return_val, 200);
  return true;
}

BEGIN_TEST_SUITE(join_tests) {
  run_test(simple_join_test);
}
