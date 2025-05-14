/**
 * Context switches to new_context from old_context.
 *
 * Saves the current execution context into the grn_context structure pointed to
 * by the first paramter and restores the execution context pointed to by the
 * second paramter. As such, when this function returns, it jumps to the address
 * at the top of the stack (pointed to by %rsp).
 *
 * This function has the following signature in C:
 * grn_context_switch(grn_context *old_context, grn_context *new_context)
 *
 * @param old_context pointer to where to save the current context(in %rdi)
 * @param new_context pointer to the context to restore(in %rsi)
 */
.globl grn_context_switch
grn_context_switch:
  //Save current context
  mov	  %rsp, (%rdi)
  mov	  %r15, 8(%rdi)
  mov	  %r14, 16(%rdi)
  mov	  %r13, 24(%rdi)
  mov	  %r12, 32(%rdi)
  mov	  %rbx, 40(%rdi)
  mov	  %rbp, 48(%rdi)
  //Load new context
  mov     (%rsi), %rsp
  mov     8(%rsi), %r15
  mov     16(%rsi), %r14
  mov     24(%rsi), %r13
  mov 	  32(%rsi), %r12
  mov	  40(%rsi), %rbx
  mov	  48(%rsi), %rbp
  ret

/**
 * This needs to be called the first time a thread executes(below in start_thread)
 * It unblocks the SIGALRM signal so the thread can be interrupted and the scheduler can switch
 * the current thread. After the first time, the unblocking will be handled when returning from grn_yield()
*/
unblock_timer:
  callq   get_sigset
  mov	  %rax, %rsi
  mov	  $1, %rdi
  mov	  $0, %rdx
  callq   sigprocmask@PLT
  ret
/**
 * Initial function implicitly executed by a thread.
 *
 * This function should be jumped into implicitly by the green threads library.
 * It expects a function pointer at the top of the stack and subsequently calls
 * that function. When that function returns, this function calls grn_exit. It
 * does not expect grn_exit to return. If it does, this function loops
 * infinitely.
 *
 * @param fn [expected at top of stack] a function to call
 */
.globl start_thread
start_thread:
  callq   unblock_timer
  add	    $0x8, %rsp 
  mov	    (%rsp), %rdi
  mov     0x8(%rsp), %r11
  callq   *%r11
  mov     %rax, %rdi
  callq   _grn_exit
loop:
  jmp     loop
