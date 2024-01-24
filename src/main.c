/* #define DEBUG */

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>

#include "chloros.h"
#include "utils.h"
#include "main.h"
#include "thread.h"
#include "utils.h"

/*
 * Initial global state.
 */
chloros_state STATE = {
  .threads = NULL,
  .current = NULL
};

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

  if (preempt) {
    // FIXME: The user has requested preemption. Enable the functionality.
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
int grn_spawn(grn_fn fn) {
   grn_thread *new_thread = grn_new_thread(true);

   //When the context switch enters this thread and returns, we should be in start_thread
   //and start_thread should have the function we want to run on the top of the stack
   int stack_sizeq = (STACK_SIZE) / 8;
   uint64_t *stackq = (uint64_t *) new_thread->stack;

   stackq[stack_sizeq - 2] = (uint64_t) start_thread;
   stackq[stack_sizeq - 1] = (uint64_t) fn;
   new_thread->context.rsp = (uint64_t) &stackq[stack_sizeq - 2];

   new_thread->status = READY;

   grn_yield();

   return new_thread->id;
}

/**
 * Garbage collects ZOMBIEd threads.
 *
 * Frees the resources for all threads marked ZOMBIE.
 */
void grn_gc() {

  //We don't gc the current thread, since we will context switch from it
  grn_thread* iter_thread = next_thread(grn_current());
  
  while(iter_thread != grn_current()) {
    if(iter_thread->status == ZOMBIE) {
      grn_thread* next_iter_thread = next_thread(iter_thread);
      
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
  // FIXME: Yield the current thread's execution time to another READY thread.

  grn_gc();
  
  grn_thread *prev = STATE.current;
  
  //We start our search for the next thread to run at the next thread pointed to by our current thread in the linked list
  grn_thread *next = next_thread(prev);

  //We loop until we find a ready thread, or until we've searched through all the threads and looped back to the original one
  while(next->status != READY && next != prev) {
    next = next_thread(next);
  }

  //If we got back to the original thread, that means we couldn't find anything else to schedule, so we return -1 to indicate that no yielding happened
  if(next == prev)
    return -1;

  //Else, we swap out the current thread for the new one
  STATE.current = next;

  //Update statuses
  next->status = RUNNING;
  //We only set the prev thread to READY if it was running before, which tells us that it didn't yield because it's work was complete
  if(prev->status == RUNNING)
    prev->status = READY;

  grn_context_switch(&prev->context, &next->context);

  
  
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
  while (!grn_yield());

  return 0;
}

/**
 * Exits from the calling thread.
 *
 * If the calling thread is the initial thread, then this function exits the
 * progam. Otherwise, the calling thread is marked ZOMBIE so that it is never
 * rescheduled and is eventually garbage collected. This function never returns.
 */
void grn_exit() {
  debug("Thread %" PRId64 " is exiting.\n", STATE.current->id);
  if (STATE.current->id == 0) {
    exit(0);
  }

  STATE.current->status = ZOMBIE;
  grn_yield();
}

/**
 * For compatbility across name manglers.
 */
void _grn_exit() { grn_exit(); }

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
