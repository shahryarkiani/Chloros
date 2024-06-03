#ifndef CHLOROS_THREAD_H
#define CHLOROS_THREAD_H

#include <stdbool.h>
#include <stdint.h>

#include "chloros.h"

/*
 * Thread lookup and traversal.
 */
int64_t atomic_next_id();
void add_thread(grn_thread *);
void remove_thread(grn_thread *);
grn_thread *next_thread(grn_thread *);
grn_thread *next_waiting_thread(grn_thread *);
void add_waiting_thread(grn_thread *);
void remove_waiting_thread(grn_thread *);
void move_thread_to_waiting(grn_thread *);
void move_thread_to_active(grn_thread *);

/*
 * Thread creation and destruction.
 */
grn_thread *grn_new_thread(bool);
void grn_destroy_thread(grn_thread *);

/*
 * Pretty debug-printing for a thread structure.
 */
void debug_thread_print(grn_thread *);

/*
 * The functions below are implemented in context_switch.S.
 */
extern void grn_context_switch(grn_context *, grn_context *) asm("grn_context_switch");
extern void start_thread(void);

#endif
