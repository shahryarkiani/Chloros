#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

#include "test.h"
#include "chloros.h"

static const int ITER_NUM = 5;
static volatile uint64_t last_thread = 0;
static volatile bool finished = false;

static int ping_pong() {
  uint64_t my_id = grn_current()->id;
  last_thread = my_id;

  for (int i = 0; i < ITER_NUM; i++) {
    while (my_id == last_thread);
    last_thread = my_id;
  }

  finished = true;
  return 0;
}

static bool ping_pong_test() {
  finished = false;

  grn_init(true);
  grn_spawn(ping_pong);
  grn_spawn(ping_pong);
  grn_wait();

  check_eq(finished, true);
  return true;
}

/**
 * The phase1 test suite. This function is declared via the
 * BEGIN_TEST_SUITE macro for easy testing.
 *
 * A test suite function takes three parameters as pointers: the result, the
 * number of tests, and the number of tests passed. The test suite function
 * should increment the number of tests each time a test is run and the number
 * of tests passed when a test passes. If all tests pass, result should be set
 * to true. If any of the tests fail, result should be set to false.
 *
 * The helper macro/function run_test can manage the pointers' state when the
 * suite function is declared using the appropriate parameter names as is done
 * by BEGIN_TEST_SUITE.
 *
 * @param[out] _result Set to true if all tests pass, false otherwise.
 * @param[out] _num_tests Set to the number of tests run by this suite.
 * @param[out] _num_passed Set to the number of tests passed during this suite.
 */
BEGIN_TEST_SUITE(phase6_tests) {
  run_test(ping_pong_test);
}
