#include "test.h"
#include "chloros.h"




static int return_val = 0;

static int modify_arg(void *arg) {
  int *target = (int*) arg;
  *target = 22;
  return 0;
}

static bool arg_test() {

  grn_init(false);
  grn_spawn(modify_arg, &return_val);
  grn_wait();

  check_eq(return_val, 22);
  return true;
}

BEGIN_TEST_SUITE(argument_tests) {
  run_test(arg_test);
}
