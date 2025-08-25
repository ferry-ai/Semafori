# Semaphores API

- `disastrOS_semOpen(id, init)` opens or creates a semaphore with initial count `init`.
- `disastrOS_semClose(id)` releases the semaphore.
- `disastrOS_semWait(id)` decrements the semaphore and possibly blocks.
- `disastrOS_semPost(id)` increments the semaphore and wakes a waiter if any.
- `count` represents available resources; negative `count` tracks waiting processes.
- Invariant: `count >= 0` implies no process is waiting.
- Invariant: `count < 0` implies `abs(count)` processes are waiting.
- Waiters are served in FIFO order.

