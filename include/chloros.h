#ifndef CHLOROS_H
#define CHLOROS_H

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#if defined(__amd64__) || defined(__amd64) || defined(__x64_64__) || \
    defined(__x64_64) || defined(_M_AMD64) || defined(_M_X64)
#define ARCH64 1
#else
#define ARCH64 0
#endif

#if !ARCH64
#error Library only implemented for AMD64!
#endif

#define malloc chloros_malloc
#define calloc chloros_calloc
#define free chloros_free

typedef enum { WAITING, READY, RUNNING, ZOMBIE, JOINABLE } grn_status;

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
  void *return_value;
  struct grn_thread_struct *waiting;
  volatile uint16_t preempt_count;
  volatile bool should_reschedule;
} grn_thread;

/*
 * The type of a function that can be the initial function of a green thread.
 */
typedef void *(*grn_fn)(void *);

void grn_init(bool);
int grn_spawn(grn_fn, void *);
int grn_yield();
int grn_wait();
grn_thread *grn_current();
void grn_exit(void *);
int grn_join(int64_t, void **);
sigset_t *get_sigset();
void grn_preempt_enable();
void grn_preempt_disable();

// Wrapper functions around non-reentrant library calls
void *chloros_malloc(size_t);
void *chloros_calloc(size_t, size_t);
void chloros_free(void *);

// read()/write() syscall wrappers
ssize_t grn_read(int, void *, size_t);
ssize_t grn_write(int, const void *, size_t);

// accept() wrapper
int grn_accept(int, struct sockaddr *, socklen_t *);

// 1 << 20 == 1MB
static const uint64_t STACK_SIZE = (1 << 20);

#endif
