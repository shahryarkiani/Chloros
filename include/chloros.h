#ifndef CHLOROS_H
#define CHLOROS_H

#include <stdint.h>
#include <stdbool.h>
#include <signal.h>

#if defined(__amd64__) || defined(__amd64) || defined(__x64_64__) \
    || defined(__x64_64) || defined(_M_AMD64) || defined(_M_X64)
  #define ARCH64 1
#else
  #define ARCH64 0
#endif

#if !ARCH64
  #error Library only implemented for AMD64!
#endif

typedef enum {
  WAITING,
  READY,
  RUNNING,
  ZOMBIE
} grn_status;

typedef struct grn_context_struct {
  uint64_t rsp;
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbx;
  uint64_t rbp;
} grn_context;

typedef struct grn_thread_struct {
  int64_t id;
  grn_status status;
  grn_context context;
  uint8_t *stack;
  struct grn_thread_struct *prev;
  struct grn_thread_struct *next;
} grn_thread;

/*
 * The type of a function that can be the initial function of a green thread.
 */
typedef int (*grn_fn)(void *);

void grn_init(bool);
int grn_spawn(grn_fn, void *);
int grn_yield();
int grn_wait();
grn_thread *grn_current();
void grn_exit();
sigset_t *get_sigset();

// 1 << 20 == 1MB * 2 == 2MB
static const uint64_t STACK_SIZE = (1 << 20) * 2;

#endif
