

# Completed Features
 - [x] Preemptive Scheduling
 - [x] Joining threads by id
 - [x] Passing arguments and receiving returns from threads
 - [x] Add separate list for threads blocked on grn_join() to reduce scheduling time. Done, waiting threads no longer poll repeatedly to see if the target thread has returned, instead the target thread knows which thread to move to the active list so they can be scheduled.
# To-Do
 - [ ] Wrappers around read()/write() syscalls that use poll() underneath
 - [ ] Run user threads on multiple kernel threads
 - [ ] Add instructions to README on how to build and link library, along with simple documentation for the actual API
