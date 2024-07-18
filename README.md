# Usage
You can build the library by running `make all`, which creates `libchloros.a`. You can link it to your program by running `CC *.o -L{lib_path} -lchloros` where CC is your C compiler. 
Make sure to use `include "chloros.h"` wherever you use the functions. Example programs are located in `examples/`

Functions:

`void grn_init(bool)` : Initializes the thread library, should only be called once from the main(initial) thread and before any other grn_* functions. `bool`, true to enable preemption, `false` if you don't want preemption

`int grn_spawn(grn_fn, void *)` : Creates a new thread and returns its id. The new thread is immediately context switched into. `grn_fn` is a function pointer that refers to a function like this: `void* func(void* arg) {}`. `void *` is the argument to be passed into the function the thread will run.

`int grn_yield()` : Yields the current thread, allowing a different thread to be scheduled. Returns `0` if a new thread was scheduled, or `-1` if no scheduling occured(same thread is running before and after the yield call).

`int grn_wait()` : Loops while repeatedly calling `grn_yield()`, ends looping after `grn_yield` returns `-1`. `grn_join` is almost always a better choice

`void grn_exit(void *)` : Stops execution of the current thread, loads the `void *` arg into the return value of the thread, so any joining thread will get that as the return value. If called by the main thread, this will call `exit(0)`. `grn_exit` is automatically called with the return value of the `grn_fn` a thread ran  after `grn_fn` returns(see `start_thread` in `context_switch.S`), so generally you don't need to call this.

`int grn_join(int64_t, void**)` : Joins the thread with the given id in `int64_t`, stores the return value of the thread in the `void**` pointer which can be NULL if you don't care about the return value. Returns `0` on successful join, returns `-1` when unable to join the thread. Calling `grn_join` on the same thread id multiple times is undefined behavior, it may return `-1` or it might cause the thread calling `grn_join` to never be ran again.

`void* chloros_malloc(size_t), void* chloros_calloc(size_t, size_t), void chloros_free(void *)` : These are wrapper functions that are necessary when preemption is enabled, `chloros.h` includes macros to convert regular calls into these wrapper calls, so you shouldn't need to interact with these directly. This doesn't work for externally linked functions which might use these calls internally.

`ssize_t grn_read(int, void *, size_t), ssize_t grn_write(int, const void*, size_t), int grn_accept(int, struct sockaddr *, socklen_t *)` : Wrapper functions that don't block, use these for I/O instead of the regular syscalls. You must call these directly(no macro to replace regular calls). See `/examples` for programs that use this. These should be generally used with preemption enabled(although with proper use of grn_yield(), they can still work).





# Completed Features
 - [x] Preemptive Scheduling
 - [x] Joining threads by id
 - [x] Passing arguments and receiving returns from threads
 - [x] Add separate list for threads blocked on grn_join() to reduce scheduling time. Done, waiting threads no longer poll repeatedly to see if the target thread has returned, instead the target thread knows which thread to move to the active list so they can be scheduled.
 - [x] Implemented wrappers around read()/write()/accept() syscalls that use epoll underneath. See examples/server_echo.c
# To-Do
 - [ ] Run user threads on multiple kernel threads
 - [ ] Add instructions to README on how to build and link library, along with simple documentation for the actual API
