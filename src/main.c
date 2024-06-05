/* #define DEBUG */

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "chloros.h"
#include "main.h"
#include "thread.h"
#include "utils.h"

#define INTERVAL 10000

#undef malloc
#undef calloc
#undef free

/*
 * Initial global state.
 */
chloros_state STATE = {
    .active_threads = NULL,
    .waiting_threads = NULL,
    .current = NULL};

sigset_t timer_sig;

/**
 * Signal Handler for timer interrupts
 *
 * Simply calls grn_yield() to schedule another thread.
 */
void grn_handle_interrupt(int signum) {
  UNUSED(signum);
  grn_yield();
}

/**
 * Sets up timed interrupts to enable preemption
 *
 * This function will be called only once by grn_init if the user wants preemption.
 * We also need to wrap non reentrant functions(such as malloc/free) with sigprocmasks
 * to ensure that they don't get interrupted.
 */
void grn_interrupt_init() {
  // Configure the signal set we want to listen for
  sigemptyset(&timer_sig);
  sigaddset(&timer_sig, SIGALRM);

  // Configure the timer
  struct itimerval itimer;
  itimer.it_interval.tv_sec = 0;
  itimer.it_interval.tv_usec = INTERVAL;
  itimer.it_value.tv_sec = 0;
  itimer.it_value.tv_usec = INTERVAL;

  // Configure action handling
  struct sigaction timeout_action;
  timeout_action.sa_handler = grn_handle_interrupt;
  timeout_action.sa_mask = timer_sig;
  timeout_action.sa_flags = 0;

  if (sigaction(SIGALRM, &timeout_action, NULL) != 0) {
    err_exit("sigaction failed: %s\n", strerror(errno));
  }
  if (setitimer(ITIMER_REAL, &itimer, NULL) != 0) {
    err_exit("setitimer failed: %s\n", strerror(errno));
  }
}

/**
 * Initializes the choloros green thread library.
 *
 * Creates the initial green thread from the currently executing context. The
 * `preempt` parameters specifies whether the scheduler is preemptive or not.
 * This function should only be called once.
 *
 * @param preempt true if the scheduler should preempt, false otherwise
 */
void grn_init(bool preempt) {
  STATE.current = grn_new_thread(false);
  assert_malloc(STATE.current);
  STATE.current->status = RUNNING;

  STATE.waiting_threads = NULL;

  if (preempt) {
    // The user has requested preemption. Enable the functionality.
    grn_interrupt_init();
  }
}

/**
 * Creates a new green thread and executes `fn` inside that thread.
 *
 * Allocates and initializes a new green thread so that the parameter `fn` is
 * executed inside of the new thread. Each thread is allocated its own stack.
 * After allocating and initialization the new thread, the current thread yields
 * its execution.
 *
 * @param fn The function to execute inside a new green thread.
 *
 * @return The thread ID of the newly spawned process.
 */
int grn_spawn(grn_fn fn, void *arg) {
  sigprocmask(SIG_BLOCK, &timer_sig, NULL);
  grn_thread *new_thread = grn_new_thread(true);
  // When the context switch enters this thread and returns, we should be in start_thread
  // and start_thread should have the function we want to run on the top of the stack
  int stack_sizeq = (STACK_SIZE) / 8;
  uint64_t *stackq = (uint64_t *)new_thread->stack;

  stackq[stack_sizeq - 4] = (uint64_t)start_thread;
  stackq[stack_sizeq - 2] = (uint64_t)arg;
  stackq[stack_sizeq - 1] = (uint64_t)fn;
  new_thread->context.rsp = (uint64_t)&stackq[stack_sizeq - 4];

  new_thread->status = READY;

  grn_yield();
  sigprocmask(SIG_UNBLOCK, &timer_sig, NULL);
  return new_thread->id;
}

/**
 * Garbage collects ZOMBIEd threads.
 *
 * Frees the resources for all threads marked ZOMBIE.
 */
void grn_gc() {

  // We don't gc the current thread, since we will context switch from it
  grn_thread *iter_thread = next_thread(grn_current());

  while (iter_thread != grn_current()) {
    if (iter_thread->status == ZOMBIE) {
      grn_thread *next_iter_thread = next_thread(iter_thread);

      grn_destroy_thread(iter_thread);

      iter_thread = next_iter_thread;
    } else {
      iter_thread = next_thread(iter_thread);
    }
  }
}

/**
 * Yields the execution time of the current thread to another thread.
 *
 * If there is at least one READY thread, this function chooses one through an
 * arbitrary search and context switches into it. The current thread is marked
 * READY if it was previous RUNNING, otherwise, its status remained unchanged.
 * The status of the thread being switched to is marked RUNNING. If no READY
 * thread is found, this function return -1. Otherwise, it returns 0.
 *
 * @return 0 if execution was yielded, -1 if no yielding occured
 */
int grn_yield() {

  // We don't want to be interrupted when we're scheduling the next thread
  sigprocmask(SIG_BLOCK, &timer_sig, NULL);

  grn_gc();

  grn_thread *prev = STATE.current;

  // We start our search for the next thread to run at the next thread pointed to by our current thread in the linked list
  grn_thread *next = next_thread(prev);

  // We loop until we find a ready thread, or until we've searched through all the threads and looped back to the original one
  while (next->status != READY && next != prev) {
    next = next_thread(next);
  }

  // If we got back to the original thread, that means we couldn't find anything else to schedule, so we return -1 to indicate that no yielding happened
  if (next == prev) {
    sigprocmask(SIG_UNBLOCK, &timer_sig, NULL);
    return -1;
  }

  // Else, we swap out the current thread for the new one
  STATE.current = next;

  // Update statuses
  next->status = RUNNING;
  // We only set the prev thread to READY if it was running before, which tells us that it didn't yield because it's work was complete
  if (prev->status == RUNNING)
    prev->status = READY;
  else if (prev->status == WAITING) {
    move_thread_to_waiting(prev);
  }

  grn_context_switch(&prev->context, &next->context);

  sigprocmask(SIG_UNBLOCK, &timer_sig, NULL);
  return 0;
}

/**
 * Blocks until all threads are finished executing.
 *
 * TODO: Keep track of parent->children relationships so that a thread only
 * waits for the threads it spawned. TODO: Take in a list of thread IDs as a
 * parameter and wait for those threads.
 *
 * @return 0 on successful wait, nonzero otherwise
 */
int grn_wait() {
  // Loop until grn_yield returns nonzero.
  while (!grn_yield())
    ;

  return 0;
}
/**
 *  Blocks until the specified thread has finished executing.
 *
 *  If ret is not NULL
 *  Stores the return value of the specified thread in the location pointed to by ret
 *  A thread's resources will only be freed after it has been joined
 *
 *  @return 0 on succesfful join, -1 otherwise
 */
int grn_join(int64_t thread_id, void **return_value_ptr) {

  sigprocmask(SIG_BLOCK, &timer_sig, NULL);

  grn_thread *joining = next_thread(STATE.current);

  // Might want to add a hashmap for fast lookup of threads by id
  while (joining != STATE.current && joining->id != thread_id) {
    joining = next_thread(joining);
  }

  if (joining == STATE.current || joining->status == ZOMBIE || joining->waiting != NULL) {
    return -1; // Can't join this thread
  }

  joining->waiting = STATE.current;

  if (joining->status != JOINABLE) {
    debug("Thread %" PRId64 " is joining Thread %" PRId64 ". \n", STATE.current->id, joining->id);

    // Mark the current thread as WAITING, the next time it runs, joining->status will be JOINABLE
    STATE.current->status = WAITING;
    grn_yield();
  } else {
    debug("Thread %" PRId64 " is already JOINABLE, by Thread %" PRId64 "\n", joining->id, STATE.current->id);
  }

  joining->status = ZOMBIE;

  if (return_value_ptr != NULL) {
    *return_value_ptr = joining->return_value;
  }

  sigprocmask(SIG_UNBLOCK, &timer_sig, NULL);

  return 0;
}

/**
 * Exits from the calling thread.
 *
 * If the calling thread is the initial thread, then this function exits the
 * progam. Otherwise, the calling thread is marked ZOMBIE so that it is never
 * rescheduled and is eventually garbage collected. This function never returns.
 */
void grn_exit(void *ret) {
  sigprocmask(SIG_BLOCK, &timer_sig, NULL);
  debug("Thread %" PRId64 " is exiting.\n", STATE.current->id);
  if (STATE.current->id == 0) {
    exit(0);
  }

  STATE.current->return_value = ret;

  // A thread must be joined before it can be garbage collected
  // TODO: Let the user indicate whether they want a thread to be joinable at creation
  STATE.current->status = JOINABLE;

  // There is a thread waiting for us to be JOINABLE
  if (STATE.current->waiting != NULL) {
    debug("Thread %" PRId64 " is waking up Thread %" PRId64 "\n", STATE.current->id, STATE.current->waiting->id);
    move_thread_to_active(STATE.current->waiting);
  }

  grn_yield();
}

/**
 * For compatbility across name manglers.
 */
void _grn_exit(void *ret) { grn_exit(ret); }

sigset_t *get_sigset() {
  return &timer_sig;
}

sigset_t *_get_sigset() {
  return get_sigset();
}

/**
 * Returns a pointer to the current thread if there is one. This pointer is only
 * valid during the lifetime of the thread.
 *
 * @return a pointer to the current thread or NULL if the library hasn't been
 * initialized
 */
grn_thread *grn_current() {
  return STATE.current;
}

// Wrapper functions around non-reentrant library calls

void *chloros_malloc(size_t size) {
  sigprocmask(SIG_BLOCK, &timer_sig, NULL);
  void *ret_val = malloc(size);
  sigprocmask(SIG_UNBLOCK, &timer_sig, NULL);
  return ret_val;
}

void *chloros_calloc(size_t nmemb, size_t size) {
  sigprocmask(SIG_BLOCK, &timer_sig, NULL);
  void *ret_val = calloc(nmemb, size);
  sigprocmask(SIG_UNBLOCK, &timer_sig, NULL);
  return ret_val;
}

void chloros_free(void *ptr) {
  sigprocmask(SIG_BLOCK, &timer_sig, NULL);
  free(ptr);
  sigprocmask(SIG_UNBLOCK, &timer_sig, NULL);
}
