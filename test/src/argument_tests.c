#include "chloros.h"
#include "test.h"

static int return_val = 0;

static void *modify_arg(void *arg) {
  int *target = (int *)arg;
  *target = 22;
  return (void *)0;
}

static bool arg_test() {

  grn_init(false);
  grn_spawn(modify_arg, &return_val);
  grn_wait();

  check_eq(return_val, 22);
  return true;
}

static const int ITER_NUM = 5;

static int ID_NUM = 0;

static void *ping_pong_arg(void *arg) {
  volatile int *check = (int *)arg;

  int myid = ID_NUM++;
  *check = myid;

  for (int i = 0; i < ITER_NUM; i++) {
    while (*check == myid)
      ;
    *check = myid;
  }

  return (void *)0;
}

static bool ping_pong_arg_test() {
  volatile int num = 0;

  grn_init(true);
  grn_spawn(ping_pong_arg, (void *)&num);
  grn_spawn(ping_pong_arg, (void *)&num);
  grn_wait();

  return true;
}

BEGIN_TEST_SUITE(argument_tests) {
  run_test(arg_test);
  run_test(ping_pong_arg_test);
}
