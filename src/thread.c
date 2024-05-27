#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chloros.h"
#include "main.h"
#include "thread.h"
#include "utils.h"

#undef malloc
#undef calloc
#undef free

/**
 * Returns a unique number.
 *
 * Each call to this function returns a number that is one more than the number
 * returned from the previous call. The first call to this functions returns 0.
 *
 * @return 0 on the first invocation; after, a number than is one more than the
 * previously returned number
 */
int64_t atomic_next_id() {
  // TODO: Make atomic if multiplexing green threads onto OS threads.
  static int64_t number = 0;
  return number++;
}

/**
 * Adds the `thread` to the linked list headed by STATE.threads. Panics if the
 * pointer to the thread being added is NULL.
 *
 * @param thread the thread to add to the linked list; must be non-null
 */
void add_thread(grn_thread *thread) {
  assert(thread);
  if (STATE.threads) {
    STATE.threads->prev = thread;
  }

  thread->prev = NULL;
  thread->next = STATE.threads;
  STATE.threads = thread;
}

/**
 * Removes the `thread` to the linked list headed by STATE.threads. Panics if
 * the pointer to the thread being removed is NULL.
 *
 * @param thread the thread being removed from linked list; must be non-null
 */
void remove_thread(grn_thread *thread) {
  assert(thread);
  if (STATE.threads == thread) {
    STATE.threads = thread->next;
  }

  if (thread->next) {
    thread->next->prev = thread->prev;
  }

  if (thread->prev) {
    thread->prev->next = thread->next;
  }
}

/**
 * Returns a pointer to the thread following `thread` in the linked list headed
 * by STATE.threads. If `thread` is last  in the linked list, this function
 * returns the head of the linked list such that a cycle is formed. Panics if
 * the pointer to the thread parameter is NULL.
 *
 * @param thread to use as a basis for the next thread; must be non-null
 *
 * @return a pointer to the thread after `thread`
 */
grn_thread *next_thread(grn_thread *thread) {
  assert(thread);
  return (thread->next) ? thread->next : STATE.threads;
}

/**
 * Allocates a new grn_thread structure and returns a pointer to it.
 *
 * Allocates and a new grn_thread structure, zeroes out its context, sets its ID
 * to a unique number, sets its status to WAITING, and adds the thread to the
 * linked list headed by STATE.threads. If `alloc_stack` is true, a 16-byte
 * aligned memory region of size `STACK_SIZE` is allocated, and a pointer to the
 * region is stored in the thread's `stack` property.
 *
 * @param alloc_stack whether or not to allocate a stack for the thread
 *
 * @return a pointer to the newly allocated grn_thread structure
 */
grn_thread *grn_new_thread(bool alloc_stack) {

  grn_thread *new_thread = calloc(sizeof(grn_thread), 1);

  new_thread->id = atomic_next_id();

  if (alloc_stack) {
    int allocated = posix_memalign((void **)&new_thread->stack, 16, STACK_SIZE);
    assert(allocated == 0);
  }

  add_thread(new_thread);

  return new_thread;
}

/**
 * Frees the resources used by `thread` and the thread itself. Removes `thread`
 * from the linked list headed by STATE.threads.
 *
 * @param thread the thread to deallocate and remove from linked list
 */
void grn_destroy_thread(grn_thread *thread) {
  remove_thread(thread);

  if (thread->stack != NULL) {
    free(thread->stack);
  }

  free(thread);
}

/**
 * Prints a formatted debug message for `thread`.
 *
 * @param thread the thread to debug pretty-pring
 */
void debug_thread_print(grn_thread *thread) {
  const char *status;
  switch (thread->status) {
  case WAITING:
    status = "WAITING";
    break;
  case READY:
    status = "READY";
    break;
  case RUNNING:
    status = "RUNNING";
    break;
  case ZOMBIE:
    status = "ZOMBIE";
    break;
  default:
    status = "UNKNOWN";
  }

  fprintf(stderr, ":: Thread ID:\t %" PRId64 "\n", thread->id);
  fprintf(stderr, ":: Status:\t %s\n", status);
  fprintf(stderr, ":: Stack low:\t %p\n", thread->stack);
  fprintf(stderr, ":: Stack top:\t %p\n", &thread->stack[STACK_SIZE]);
  fprintf(stderr, ":: rsp reg:\t 0x%08" PRIu64 "x\n", thread->context.rsp);
  fflush(stderr);
}
