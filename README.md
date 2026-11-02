<i>This project has been created as part of the 42 curriculum by \<janrodri>.</i>

# Codexion

## Description

Codexion is a concurrency project written in C. It simulates several coders working concurrently while sharing a limited number of dongles.

Each coder repeatedly performs the following actions:

1. Acquires two dongles.
2. Compiles.
3. Releases the dongles.
4. Debugs.
5. Refactors.
6. Repeats until the required number of compilations is reached or the coder burns out.

The project focuses on:

- POSIX threads.
- Mutexes and condition variables.
- Dynamic memory allocation.
- Resource management.
- Deadlock prevention.
- FIFO and EDF scheduling.
- Precise time management.
- Safe termination and cleanup.

## Required sections — quick map

- [Description](#description)
- [Instructions](#instructions)
- [Resources and Use of AI](#resources)
- [Blocking cases handled](#blocking-cases-handled)
- [Thread synchronization mechanisms](#thread-synchronization-mechanisms)

---

## Resources

- `Codexion.md`: project subject and required behaviour.
- `Norma - 42.pdf`: 42 Norm and style constraints.
- POSIX man pages: `pthread_create(3)`, `pthread_join(3)`,
  `pthread_mutex_init(3)`, `pthread_mutex_destroy(3)`, `pthread_cond_init(3)`,
  `pthread_cond_broadcast(3)`, `pthread_cond_timedwait(3)`, `gettimeofday(2)`
  and `usleep(3)`.
- `CONTROL.md`: implementation log, verification evidence and defence notes.

### Use of AI

AI was used as a tutoring and review assistant: explaining concurrency concepts
(mutexes, condition variables, heaps, EDF and monitor timing), helping design
edge cases and stress tests, and reviewing code and documentation. The final
implementation was written, integrated and validated by the author; no
AI-generated code was copied verbatim into the final source.

---

## Instructions

Build and run the project with the commands below.

### Compilation

The project is compiled using the following flags (specified in the FLAGS variable in the Makefile):

```bash
-Wall -Wextra -Werror -pthread
```

To compile the project:

```bash
make
```

To remove object files:

```bash
make clean
```

To remove object files and the executable:

```bash
make fclean
```

---

### Execution

The program expects the following 8 arguments:

```text
./codexion number_of_coders \
time_to_burnout \
time_to_compile \
time_to_debug \
time_to_refactor \
number_of_compiles_required \
dongle_cooldown \
scheduler
```

The scheduler must be either:

```text
fifo
```

or:

```text
edf
```

Example:

```bash
./codexion 4 800 200 150 100 3 50 fifo
```

This example creates:

- 4 coders.
- 4 dongles.
- A burnout limit of 800 milliseconds.
- A compilation duration of 200 milliseconds.
- A debugging duration of 150 milliseconds.
- A refactoring duration of 100 milliseconds.
- 3 required compilations per coder.
- A dongle cooldown of 50 milliseconds.
- FIFO scheduling.

---

## Project Structure

The current source files are organised as follows:

```text
coders/
├── codexion.h
├── main.c
├── init.c
├── coders.c
├── dongles.c
├── resources.c
├── compiling.c
├── queue.c
├── queue_aux.c
├── heap_tools.c
├── heap_tools_aux.c
├── simulation.c
└── sync.c
```

`sync.c` holds the initialisation and destruction of the synchronisation
primitives. `simulation.c` holds the clock helpers.

### `codexion.h`

Contains:

- Structure declarations.
- Type aliases.
- Function prototypes.
- Required system headers.

### `main.c`

Responsible for:

- Creating the main control structure on the stack.
- Initialising its dynamic pointers to `NULL`.
- Validating program arguments.
- Delegating the coder and dongle arrays to `init_coders` / `init_dongles`.
- Applying the nominal dongle topology (`dongles_assignation`).
- Taking the simulation start timestamp.
- Orchestrating the lifecycle: `init_sync` → `run_simulation` → `destroy_sync`.
- Releasing allocated memory with `free_control` on every exit path.

### `init.c`

Responsible for:

- Parsing and validating command-line arguments.
- Converting valid arguments into integer values.
- Storing the validated values in the control structure.

### `coders.c`

Responsible for:

- Allocating the coder array.
- Initialising each coder.
- Assigning each coder its identifier and shared control structure.
- `run_simulation`: creating one thread per coder and joining every thread that
  was successfully created. The loop counter doubles as the created-thread
  count, so a partial failure joins exactly `0..i - 1` threads.

### `dongles.c`

Responsible for:

- Allocating the dongle array.
- Initialising each dongle.
- Applying the circular nominal topology (`dongles_assignation`).
- `put_dongles_blocked`: marking the acquired pair as in use, called from
  `block_dongles` while the dongle mutex is held.

### `compiling.c`

Holds the workflow executed by each coder thread:

- `block_dongles`: acquires both required dongles atomically, waiting on the
  condition variable while they are unavailable. The sleep is re-chosen on every
  pass: a timed wait when the pair has a measurable cooldown deadline, an
  untimed wait otherwise.
- `compiling_phase`: logs the compilation, sleeps for `time_to_compile`, then
  releases both dongles.
- `debugging_phase` and `refactoring_phase`: log and sleep, touching no shared
  resource.
- `coder_routine`: the thread entry point, running the cycle above.

### `resources.c`

Contains the resource-availability helpers:

- `dongle_ready`: a dongle is usable when its state is free and its cooldown
  timestamp has elapsed.
- `dongles_available`: both dongles of a pair must be ready, and the pair must
  not contain the same dongle twice. It answers whether the pair can be taken,
  not why it cannot.
- `cooldowns_deadline`: given a pair, the instant at which it becomes
  available, or `DEADLINE_NEVER` when no such instant exists yet.
- `log_line`: the serialised log formatter.

### `queue.c`, `queue_aux.c`, `heap_tools.c` and `heap_tools_aux.c`

Hold the single scheduler heap and the arbitration helpers:

- `init_waiting_queue` (`queue_aux.c`): allocates the only `t_coder *` array
  (`scheduler_queue.items`, `number_of_coders` slots) and the temporary
  `deferred` buffer of the same capacity. It sets `size = 0` and
  `capacity = number_of_coders`. A failed allocation leaves the queue empty and
  releases whatever was already reserved. Both arrays are freed on every exit
  path.
- `register_request`: assigns a monotonic `ticket` and marks the coder as
  `REQUEST_WAITING`. It runs under `mtx_dongles`, so the ticket order matches the
  order in which requests reached the critical section.
- `enqueue_coder` delegates to `heap_push`; `dequeue_coder` (`queue_aux.c`)
  delegates to `heap_remove`. The heap is the only owner of pending requests.
- `heap_push`, `heap_pop`, `heap_peek` and `heap_remove`
  (`heap_tools_aux.c`) maintain a binary min-heap. `heap_up` and `heap_down`
  (`heap_tools.c`) restore the heap property after an insertion or removal.
  `heap_pos` in `t_coder` records the position inside the heap, so removal is
  `O(log n)` instead of a linear search.
- `deadline`: returns the absolute burnout instant,
  `last_compilation + time_to_burnout`. It does **not** read the current clock;
  it is a date, not a remaining time.
- `temp_beats_best`: the comparator. In `fifo` the smaller ticket wins; in `edf`
  the smaller deadline wins and equal deadlines are broken by the smaller
  ticket. The ticket gives a total, reproducible order.
- `coder_electable` (`queue_aux.c`): a coder is electable when it is still
  `REQUEST_WAITING` and both of its dongles are available.
- `choose_next_in_queue`: the election. It calls `drain_unserviceable`, which
  pops non-electable roots into the `deferred` buffer until the root is
  electable (or the heap is empty). It captures that root with `heap_peek`,
  restores every deferred coder with `heap_push`, and returns the captured
  pointer. It is called with `mtx_dongles` held.

The heap is ordered by `temp_beats_best`, but the election is **work-conserving**:
a coder whose pair is blocked or cooling cannot stall a coder whose pair is
free. The lazy `deferred` extraction removes only the non-electable roots that
hide the best electable one; the winner is still inserted in the heap and is
removed later by `dequeue_coder`.

The winner is recomputed on demand from the heap and the current dongle state,
which avoids a stale cursor after a release, a cooldown expiry or a coder
leaving the queue.

### `sync.c`

Holds `init_sync` and `destroy_sync`. Initialisation reports which primitive
failed; destruction only destroys what was successfully created, using a count
of created primitives.

### `simulation.c`

Holds the time helpers:

- `current_time`: milliseconds since the epoch, from `gettimeofday`.
- `ms_to_timespec`: converts milliseconds into a `struct timespec` absolute
  deadline. Called by `block_dongles` when the cooldown wait needs a date.

---

## Main Data Structures

### `t_control`

`t_control` contains the global state shared by the whole simulation.

It stores:

- The number of coders.
- The scheduler type.
- Timing parameters.
- The number of compilations required.
- The simulation status (initialised to 1; not yet read by any thread).
- The simulation start timestamp used to compute relative log times.
- The synchronisation primitives (`mtx_dongles`, `mtx_log`, `cond_dongles`).
- The dynamically allocated coder array.
- The dynamically allocated dongle array.

The control structure is created in `main` and passed to other structures by pointer.

This avoids copying the complete simulation state into every coder. All coders access the same shared control structure.

---

### `t_coder`

Each coder represents one participant in the simulation.

A coder stores:

- Its identifier.
- The number of compilations already completed.
- The time of its last compilation.
- A pointer to the shared control structure.
- The identifiers of its left and right dongles.
- Its thread identifier when the thread layer is implemented.

The coder structures are stored in a dynamically allocated array.

The array is allocated with one `malloc` call:

```text
number_of_coders × sizeof(t_coder)
```

Each element is then initialised individually.

---

### `t_dongle`

Each dongle represents one shared resource.

A dongle stores:

- Its identifier.
- Its current state.
- The instant at which it becomes available again (`available_at`).

The possible states are conceptually:

```text
BLOCKED
FREE (possibly still cooling until available_at)
```

The current code represents the states with named `#define` constants:
`DONGLE_BLOCKED` and `DONGLE_FREE`. This avoids numeric magic values while
keeping the state type simple. A C `enum` has not yet been introduced in the
current implementation.

The cooldown is expressed by the `available_at` timestamp rather than a dedicated
`COOLDOWN` state: a cooling dongle keeps the `FREE` state but is not ready until
`current_time` reaches `available_at`. A dedicated `COOLDOWN` state is
deliberately avoided because nothing would return the dongle to `FREE`.

The dongles are also stored in a dynamically allocated array.

---

## Argument Validation

Arguments are validated before the control structure is populated.

The parser checks that:

- The argument exists.
- The argument is not empty.
- The argument contains only digits.
- The value is greater than zero.
- The value does not overflow an `int`.
- The scheduler is either `fifo` or `edf`.
- The expected number of arguments has been provided.

The parser currently uses different negative return values to distinguish error categories:

```text
-1  Invalid format or negative value
-2  Value equal to zero
-3  Value greater than INT_MAX
-4  Empty or invalid argument
```

The parsed value is stored only after it has passed all validation checks.

This prevents invalid input from reaching the simulation logic.

---

## Integer Overflow Protection

Using `atoi` directly is not sufficient for validation because `atoi` does not reliably report overflow.

Therefore, the project validates the textual representation before accepting the result.

The validation process is based on:

1. Checking that every character is a digit.
2. Checking the length of the value.
3. Comparing ten-digit values against `2147483647`.
4. Rejecting values with more than ten digits.

This allows the program to reject values outside the range of a signed 32-bit integer before using them in the control structure.

Leading-zero handling is intentionally kept simple for the current implementation. The main objective is to reject invalid, empty, zero, negative, non-numeric, and overflowing values.

---

## Dynamic Memory Management

The coder and dongle arrays are allocated dynamically because their size depends on the command-line arguments.

The control structure itself is currently stored on the stack because:

- Its lifetime is limited to `main`.
- Its size is fixed.
- It does not need to outlive `main`.
- It only contains pointers to the dynamically allocated arrays.

The arrays are allocated on the heap because their size is known only at runtime.

The ownership model is:

```text
main owns t_control
t_control owns the coder array
t_control owns the dongle array
```

Each successful allocation must have exactly one corresponding `free`.

The cleanup function releases:

1. The coder array.
2. The dongle array.
3. Any future resources allocated by the simulation.

The individual array elements are not freed separately because they are not individually allocated. They are part of one contiguous allocation.

---

## Initialisation Strategy

Initialisation is separated into small functions.

The general process is:

```text
1. Create the control structure.
2. Set dynamic pointers to NULL.
3. Validate all arguments.
4. Store validated values in t_control.
5. Allocate the coder array.
6. Initialise every coder.
7. Allocate the dongle array.
8. Initialise every dongle.
9. Start the simulation.
10. Join all threads.
11. Destroy synchronisation objects.
12. Free all allocated memory.
```

This order makes ownership and error handling easier to reason about.

If an allocation fails:

- The program must stop starting the simulation.
- Previously allocated resources must be released.
- No uninitialised pointer may be freed.
- The program must return an error status.

---

## Dongle Model

The number of dongles is normally equal to the number of coders.

There is one special case:

```text
number_of_coders == 1
```

In this case, the project must create only one dongle, as required by the subject.

However, a coder normally needs two different dongles to compile. Therefore, one coder cannot successfully acquire the required pair.

This case is not rejected during argument validation. It is considered a valid configuration that cannot complete compilation and must eventually terminate through burnout.

The subject does not describe a special case for it. It only states that one person means one dongle on the table, and that the simulation stops when someone burns out. Therefore the configuration is handled by the ordinary termination mechanism, not by argument validation.

The program must not:

- Use the same dongle twice as if it were two resources.
- Lock the same mutex twice from the same thread.
- Pretend that one dongle satisfies the two-dongle requirement.
- Enter an infinite waiting state.

### Where the protection lives

The nominal assignment keeps `left_dongle == right_dongle` for the single
coder. The invalid pair is refused at the point of use, inside
`dongles_available`, before any state is inspected. This keeps the invariant
in one place instead of special-casing the topology.

A mutex cannot enforce this restriction. Locking a mutex twice from the same
thread does not fail, it deadlocks silently. The refusal is a predicate
evaluated inside the critical section, not a property of the lock.

The refusal is permanent rather than a retry. A coder that holds no pair must
wait for termination instead of spinning, otherwise it burns CPU while the
burnout never arrives.

Burnout is detected by the monitor thread, as the subject requires. A coder
that cannot compile never declares its own burnout.

---

## Nominal Dongle Assignment

The current design uses a fixed nominal topology.

For two or more coders, the intended circular assignment is:

```text
Coder 1: dongle 1 and dongle 2
Coder 2: dongle 2 and dongle 3
Coder 3: dongle 3 and dongle 4
...
Coder N: dongle N and dongle 1
```

This assignment is nominal because it describes which two resources belong to each coder's position in the circular table.

The dongles are not permanently owned by a coder. Their availability changes dynamically during the simulation.

A coder may only compile when both of its required dongles are available at the same time.

---

## Atomic Dongle Acquisition

Acquiring two dongles must be treated as one logical operation.

The intended protocol is:

```text
1. Enter the central resource-management section.
2. Check whether both required dongles are available.
3. If both are available:
   - Mark both dongles as in use.
   - Leave the resource-management section.
   - Start compiling.
4. If one or both are unavailable:
   - Wait for a state change.
5. After compilation:
   - Enter the resource-management section.
   - Release both dongles.
   - Mark them as available or cooling down.
   - Notify waiting coders.
```

The coder must never hold one dongle while waiting for the second one.

This rule is important because acquiring one resource and waiting for another could create a circular wait and lead to deadlock.

Compilation must happen outside the resource-management mutex. The mutex should protect only the shared dongle state, not the entire compilation period.

### Current state of the protocol

The acquisition half is implemented in `block_dongles`: both dongles are
checked together inside the critical section, and a `while` loop around the wait
handles the unavailable case. Both dongles are marked as in use only after the
check succeeds, so no coder ever holds a partial pair. The marking itself is
delegated to `put_dongles_blocked` in `dongles.c`.

The release half is implemented in `compiling_phase`: after the compilation
sleep, a second critical section marks both dongles free, stores their cooldown
timestamp, and notifies every waiting coder with `pthread_cond_broadcast`. The
state and the timestamp are written together, and the notification happens
after the writes and inside the mutex, so a woken coder always observes the new
state.

A release notifies waiters, but it does not notify them when a cooldown
**expires**. The two unavailability cases therefore need two different sleeps,
re-chosen on every pass of the `while`:

- If either dongle is not `FREE`, or the coder would use the same dongle twice
  (`left == right`), there is no date to compute: only a release can wake the
  waiter, so it sleeps with `pthread_cond_wait`. The helper returns
  `DEADLINE_NEVER` for these cases.
- If both are `FREE`, the pair becomes available when the later of the two
  `available_at` timestamps is reached. That is an absolute instant on
  `CLOCK_REALTIME`, the same clock `pthread_cond_timedwait` counts on, so the
  waiter sleeps with `pthread_cond_timedwait` and wakes itself up.

The deadline is produced by `cooldowns_deadline` in `resources.c` and converted
with `ms_to_timespec`. A deadline that is not strictly in the future would make
`pthread_cond_timedwait` return immediately and the loop spin instead of sleep,
which is why the two guards above are not optional.

---

## Dongle Cooldown

After a dongle is released, it cannot immediately be reused if the cooldown period has not elapsed.

The cooldown can be represented by storing a timestamp:

```text
available_at
```

The dongle is available only when:

```text
current_time >= available_at
```

When a coder releases a dongle:

```text
available_at = current_time + dongle_cooldown
```

This avoids sleeping while holding the shared resource mutex.

The mutex protects the timestamp and state transition, while the actual waiting happens outside the critical section.

---

## Threads

Each coder is represented by one POSIX thread, created by `run_simulation`.

The main thread:

```text
1. Initialise the simulation.                        (done)
2. Create one thread per coder.                      (done)
3. Start the monitor.                                (done: monitor_routine)
4. Wait for all coder threads with pthread_join.      (done)
5. Wait for the monitor.                              (done)
6. Destroy mutexes and condition variables.           (done)
7. Free allocated memory.                             (done)
```

Thread creation order does not guarantee execution order.

The operating system scheduler decides when each thread actually runs. Therefore, the program must not depend on a specific thread starting before another one.

The scheduler requested by the command-line arguments controls the order in which coders are selected when resources become available. It does not directly control the operating system's thread scheduler.

### Thread completion and `pthread_join`

The main thread does not discover that another thread has finished by polling
the `pthread_t` value. It waits explicitly with `pthread_join`.

The intended lifecycle is:

```text
1. Initialise synchronization objects.
2. Create each coder thread and store its pthread_t.
3. Create the monitor thread and store its pthread_t.
4. Join every thread that was successfully created.
5. Destroy synchronization objects only after all joins return.
6. Free the shared arrays.
```

If creation fails after some threads have already started, the program must
stop creating new threads, notify the existing ones if necessary, join only
the threads that were successfully created, and then clean up. A thread that
was never created must not be joined.

---

## FIFO Scheduler

FIFO means First In, First Out: the request that arrived first is considered
before later requests.

### Election model

There is no mutable `next_in_queue` cursor. Whenever a coder has to decide
whether it may take the dongles, the election is recomputed from the queue and
the current dongle state:

1. The coder registers once (`register_request`) and is appended once
   (`enqueue_coder`), both under `mtx_dongles`.
2. It waits on `cond_dongles`. A release marks both dongles free and broadcasts;
   it does not hand-pick the next coder.
3. On wake-up, every waiter recomputes `choose_next_in_queue` while holding
   `mtx_dongles`. Only the elected coder leaves the wait loop, marks its pair as
   blocked, broadcasts and then removes itself from the queue.

`choose_next_in_queue` ignores coders whose pair is not available, so a coder
blocked on a busy dongle cannot stall a coder whose pair is free. For FIFO the
comparator is the `ticket` assigned in `register_request`: the smallest ticket
wins.

Recomputing on demand avoids a stale cursor after a release, a cooldown expiry
or a coder leaving the queue (burnout or termination).

### Wait predicate and progress

The wait loop keeps a coder asleep while the simulation runs and it is not both
available and elected: logically `running && !(available && elected)`. Writing
it as `!available && !elected` is wrong: a coder whose pair is free but is not
the elected one would leave the loop and steal the pair. De Morgan's law gives
the safe form `running && (!available || elected != coder)`. The availability
test comes first, so `choose_next_in_queue` is only called when the pair could
actually be taken.

After the winner marks both dongles `BLOCKED`, it broadcasts before releasing
the mutex. Waiters recompute: disjoint pairs see their dongles free and compile
in parallel, while overlapping pairs sleep again. A broadcast (not a signal) is
needed because several disjoint pairs may be valid successors; the extra
wake-ups are bounded by the number of coders.

The wait uses the condition variable in two modes. `cooldowns_deadline` returns
`DEADLINE_NEVER` when a dongle is blocked, and otherwise the later
`available_at`. If that instant is in the past (the pair is already free), a
timed wait would return immediately and spin. So a coder waits without timeout
when the deadline is `DEADLINE_NEVER` or `<= now`, and uses `timedwait` only for
a real future cooldown. Every state change happens under `mtx_dongles` followed
by a broadcast, so no wake-up is lost.

The winner removes itself from the queue and resets `waiting` before unlocking,
so no other waiter can elect a coder that has already acquired or left.

### Heap requirement

The subject requires a priority queue (heap) for scheduling. The code now uses
a dynamically allocated binary min-heap (`scheduler_queue.items`) with the
comparator selected by the scheduler. Removal uses the `heap_pos` index stored
in each coder, so it is `O(log n)`; there is no linear scan in the election.

Because a heap ordered only by ticket/deadline could leave a non-serviceable
coder at the root, `choose_next_in_queue` performs a lazy extraction: the
non-electable roots are moved to the `deferred` buffer, the first electable root
is captured, and every deferred coder is pushed back before returning. This
keeps the heap intact and preserves work-conserving behaviour.

### Integration status

The acquisition path enforces the election: a coder only leaves
`wait_for_dongles` when it is both available and elected. The request lifecycle
is closed too: the winner marks its pair, broadcasts, removes itself from the
heap and resets `waiting` under `mtx_dongles`, so its next round registers a
fresh ticket.

Verified with gdb on 2026-10-30: with N=3 (all pairs overlap) FIFO acquisitions
follow tickets `0..8` in order; with N=4 disjoint pairs `0+2` run in parallel,
then `1+3`. Smoke tests with N=1..5, FIFO/EDF and cooldown 1/300 complete, and
Valgrind reports no leaks or errors.

---

## EDF Scheduler

EDF means Earliest Deadline First: the coder with the closest burnout deadline
wins.

The absolute deadline is:

```text
deadline = last_compilation + time_to_burnout
```

`deadline` (`queue.c`) computes exactly that expression without reading the
current clock. `last_compilation` is initialised to `start_time` and updated at
the start of each real compilation, so every waiting coder has a comparable
deadline.

`temp_beats_best` compares deadlines (`<`) and breaks equal deadlines with the
smaller ticket (`<`). This matters because the heap is not sorted by arrival:
after every push or removal the heap property is restored, so the physical order
does not match the arrival order. The ticket is a unique, monotonically
increasing key, so the election is deterministic for equal deadlines.

The same election model as FIFO applies: a coder is only grantable if its two
dongles are available, and the winner is recomputed on demand under
`mtx_dongles`.

### EDF validation (Hito 5, 2026-10-31)

- Viable EDF scenario `5 600 100 50 50 3 20 edf`: 20 runs, 15 compilations per
  run, `exit=0`, 0 burnouts and 0 failures.
- Deliberately tight FIFO scenario `2 15 100 10 10 1 1 fifo`: 10 runs, burnout
  in 10/10 (for example at 17 ms).
- FIFO preserves arrival order, so a coder can wait behind older overlapping
  requests until its deadline expires. EDF selects the closest deadline first;
  with feasible parameters every coder finishes before burning out. EDF still
  burns out if the workload itself is infeasible.
- Valgrind is clean on both scenarios and Norminette reports 13/13 files OK.

---

## Monitor

The subject requires a separate monitor thread that detects burnout with
precision and stops the simulation.

It verifies whether:

- A coder has exceeded its burnout limit.
- Every coder has completed the required number of compilations (success).

The monitor loop implements both outcomes:

- On success (every coder reached `number_of_compiles_required`) it sets
  `SIM_FINISHED` and returns.
- On burnout (`current_time() - last_compilation > time_to_burnout` for any
  coder) it logs `X burned out`, sets `SIM_STOPPED` and returns. It stops at the
  first burnout it finds, logging a single `burned out` line.

The scan gives success precedence per coder: a coder that already reached
`number_of_compiles_required` is counted as finished and is **not** checked for
burnout. Without that order, a coder that completed its last compile but was
still in debug/refactor could be reported as `burned out` because its
`last_compilation` was old. Burnout is still evaluated for every coder that has
not completed its required compiles.

The monitor scan and the state change happen under one `mtx_dongles` critical
section. `check_simulation_state` takes the lock, calls `scan_coders_locked`,
and, depending on the result, logs the burnout or marks success before a single
`pthread_cond_broadcast`. `scan_coders_locked` assumes the lock is already held.
Status reads outside the monitor go through `simulation_is_running`, which takes
and releases `mtx_dongles`; `block_dongles` and `wait_for_dongles` keep direct
reads because they already hold that mutex.

The burnout message must be printed within 10 ms of the real burnout moment,
which bounds how often the monitor may poll. The loop samples on a fixed
interval for that reason.

The monitor must not inspect mutable coder or dongle state without synchronisation.

When the simulation ends, the monitor updates the shared simulation status and notifies waiting threads with `pthread_cond_broadcast`.

The monitor is mandatory for every configuration, including the single coder,
which never starts compiling and can only terminate through burnout.

---

## Blocking cases handled

- **Deadlock and Coffman conditions:** mutual exclusion is inherent to the
  dongle state protected by `mtx_dongles`; hold-and-wait is eliminated because
  the two dongles are acquired atomically; circular wait cannot form because no
  coder holds one dongle while requesting the second; no-preemption is respected
  because only the holder releases, and both dongles are returned together.
- **Starvation:** FIFO uses a monotonic ticket; EDF uses the closest deadline
  with the ticket as tie-break. The election is recomputed on every wake-up and
  skips coders whose pair is blocked or cooling, so a coder waiting for a busy
  pair cannot stall a coder whose pair is free.
- **Cooldown:** `available_at` is protected by `mtx_dongles`; waiters use
  `pthread_cond_timedwait` only for a real future cooldown, and a release uses
  `pthread_cond_broadcast`. There is no polling or busy-wait.
- **Lost and spurious wake-ups:** every wait is inside a `while` predicate; the
  shared state changes before the broadcast, while the mutex is held.
- **Burnout precision:** the monitor samples every 5 ms and uses a strict `>`
  comparison. Detection is at most one sample late (measured at most 7 ms in
  the final tests), inside the subject's 10 ms window; the log timestamp is taken
  when burnout is detected.
- **Log serialisation:** every complete log line is printed under `mtx_log`, so
  lines from different threads cannot interleave.
- **Single coder / same dongle:** a coder whose left and right ids are the same
  refuses the pair, sleeps without spinning, and terminates through burnout.
- **Partial thread creation:** only successfully created coder threads are
  joined; the simulation stops and wakes any waiters before joining.
- **No mutex during long work:** compile, debug, refactor, `usleep` and output
  happen outside `mtx_dongles`; critical sections only change state or queue
  entries.
- **Lock order:** the only nested order is monitor `mtx_dongles` → `mtx_log`;
  no path takes them in the opposite order.

## Thread synchronization mechanisms

The implementation uses two mutexes and one condition variable.

- `mtx_dongles` is the main data mutex. It protects dongle ownership
  (`state_dongle`), cooldown timestamps (`available_at`), the scheduler queue
  (`items`, `size`, `capacity`, `deferred`), coder counters
  (`number_of_compiles_done`, `last_compilation`) and `simulation_status`.
  Keeping them together makes the pair check and the pair mark atomic.
- `mtx_log` serializes complete output lines. It is held only while printing one
  line; `log_line` does not take `mtx_dongles`, so there is no inverse lock order.
- `cond_dongles` is the wait/wake channel. Coders wait for both dongles, a
  cooldown deadline, scheduler election or simulation stop; the monitor
  broadcasts after changing the state.

The wait protocol is:

- Every wait is inside a `while` and rechecks its predicate, so spurious wake-ups
  cannot acquire a pair.
- The state changes before `pthread_cond_broadcast`, under the mutex; this
  prevents lost wake-ups.
- `pthread_cond_timedwait` is used only when there is a real future cooldown
  deadline. If the deadline is absent or already past, the coder uses
  `pthread_cond_wait` instead of spinning.
- No mutex is held during `usleep`, compile, debug or refactor. Critical sections
  are kept short and only change shared state.

Initialization and destruction live in `sync.c`. `init_sync` initializes
`mtx_dongles`, `mtx_log` and `cond_dongles` in order and cleans up partial
initialization in reverse order. `destroy_sync` destroys only what was
successfully created; all threads are joined before destroy/free.

---

## Logging

Output must be serialised so that messages from different threads do not overlap.

`mtx_log` protects logging operations. The rule is:

```text
1. Lock the output mutex.
2. Print one complete message.
3. Unlock the output mutex.
```

The output mutex is separate from the dongle-management mutex to avoid unnecessary contention.

The elapsed time is computed before the lock is taken. The lock only has to
protect the shared output stream, not the clock.

### Log format

The subject requires exactly five state messages:

```text
timestamp_in_ms X has taken a dongle
timestamp_in_ms X is compiling
timestamp_in_ms X is debugging
timestamp_in_ms X is refactoring
timestamp_in_ms X burned out
```

The message text is held as named constants in `codexion.h`, so the exact
wording is defined in one place. `log_line` only formats and serialises; it
does not decide which state is being reported.

The caller decides the message. A coder thread logs its own phases, and the
monitor logs the burnout. The same helper serves every caller.

The newline belongs to the formatter, not to the message constants.

---

## Termination

The simulation terminates when one of the following conditions occurs:

### Successful termination

Every coder has completed the required number of compilations.

### Burnout termination

At least one coder exceeds its burnout limit.

### Error termination

A fatal error occurs, such as an unrecoverable allocation or thread initialisation failure.

Once the simulation status indicates termination:

- New compilation attempts must stop.
- Waiting coder threads must be notified.
- Existing threads must exit cleanly.
- The main thread must join all created threads.
- Synchronisation objects must be destroyed.
- Dynamically allocated memory must be released.

---

## Error Handling

The program must handle:

- Invalid argument count.
- Empty arguments.
- Negative values.
- Zero values.
- Non-numeric values.
- Integer overflow.
- Invalid scheduler names.
- `malloc` failure.
- Thread creation failure.
- Mutex initialisation failure.
- Condition-variable initialisation failure.
- The single-coder configuration.
- A coder burning out while waiting for dongles.
- Termination while other coders are asleep or waiting.

Cleanup must be performed in reverse order of successful initialisation.

For example:

```text
If dongles are allocated after coders:
    destroy dongle-related resources
    free dongles
    free coders
```

If a later initialisation step fails, only resources that were successfully created should be destroyed.

### Error and stress validation (Hito 7, 2026-10-31)

- 10 invalid-argument cases (count, empty, negative, zero, non-numeric,
  overflow, invalid/empty scheduler) print a clear error and exit with code 1.
- `malloc` failure injection at the four allocation points (queue, deferred
  buffer, coders, dongles) exits with code 1 without crashing.
- `pthread_create` failure injection (`FAIL_AT=1,2,5`) stops the simulation,
  joins only created coders and never joins a missing monitor.
- Stress: 138 valid runs (4.6) plus 20 viable EDF and 10 tight FIFO runs
  (Hito 5), all with timeouts and compilation-count checks.
- Valgrind: 0 leaks and 0 errors across success, burnout, N=1, argument errors,
  thread-creation failures and the EDF/FIFO scenarios.
- TSan: 0 data races on the 18-run matrix. Helgrind: 0 data races, with the
  documented timed-wait `Dubious` warning.

### Defence notes

- **Deadlock:** both dongles are acquired atomically under `mtx_dongles`; a coder
  never holds one dongle while waiting for the other, so there is no
  hold-and-wait cycle.
- **Heap:** the subject requires a priority queue; the binary min-heap supports
  `O(log n)` insertion/removal, and the deferred buffer preserves
  work-conserving election when the root is not serviceable.
- **10 ms burnout window:** the monitor is a separate thread sampling every
  5 ms; with the strict `>` comparison, detection is at most one sample late.
- **EDF arbitration:** `deadline()` computes `last_compilation +
  time_to_burnout`; `temp_beats_best` picks the minimum deadline with a ticket
  tie-break, and the winner is recomputed on every wake-up under `mtx_dongles`.

---

## Restrictions

The project must respect the subject restrictions.

Important constraints include:

- No global variables.
- No libft.
- Only authorised functions may be used.
- The project must compile with `-Wall -Wextra -Werror`.
- Thread-related operations must use POSIX threads.
- FIFO and EDF queues must be allocated dynamically on the heap.
- Shared state must be protected against data races.
- The implementation must respect Norminette requirements.

---