#ifndef CHLOROS_MAIN_H
#define CHLOROS_MAIN_H

#include "chloros.h"

/**
 * This structure keeps track of the global state for the green threads library.
 */
typedef struct chloros_state_struct {
  /**
   * A pointer to the head of the linked list of active threads being managed
   * by the library.
   */
  grn_thread *active_threads;

  /**
   * A pointer to the head of the linked list of waiting threads
   */
  grn_thread *waiting_threads;

  /**
   * Pointer to the currently active thread.
   */
  grn_thread *current;
} chloros_state;

extern chloros_state STATE;

void grn_gc();

#endif
