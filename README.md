<i> This project has been created as part of the 42 curriculum by \<janrodri\> .</i>

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

---

## Compilation

The project is compiled using the following flags:

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

## Execution

The program expects the following arguments:

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

## Synchronisation

The project uses mutexes and condition variables to protect shared state.

`mtx_dongles` protects the dongle state array: both `state_dongle` and
`available_at` are read and written together, never separately. `mtx_log`
protects output. `cond_dongles` wakes coders waiting for resources.

Initialisation and destruction live in `sync.c`. Initialisation reports which
primitive failed, and destruction only destroys what was successfully created,
using a count of created primitives.

Since 4.7a, `mtx_dongles` also protects `number_of_compiles_done`,
`last_compilation` and `simulation_status`. A coder updates `last_compilation`
inside `block_dongles` while the lock is held, the monitor scans the same fields
under the lock in `check_simulation_state`, and status reads outside the monitor
use `simulation_is_running`. This avoids a second mutex and a second lock order;
the monitor contends only once every 5 ms.

Mutexes protect:

- Dongle ownership.
- Dongle cooldown timestamps.
- Coder state (`number_of_compiles_done`, `last_compilation`).
- Simulation status.
- Output logging.
- Scheduler queues.

Condition variables will allow coder threads to wait until:

- Both required dongles become available.
- A cooldown expires.
- The simulation finishes.
- The scheduler selects a waiting coder.

The program must avoid holding a mutex while performing long operations such as:

- `usleep`.
- Compilation simulation.
- Debugging.
- Refactoring.
- Long output operations.

Critical sections should remain as short as possible.

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

## Current Implementation Status

### Implemented

- Command-line argument count validation.
- Numeric argument validation.
- Empty argument detection.
- Zero-value rejection.
- Negative-value rejection.
- Non-numeric character rejection.
- `INT_MAX` overflow detection.
- Scheduler argument validation.
- Control structure initialisation.
- Dynamic coder-array allocation.
- Dynamic dongle-array allocation.
- Coder initialisation.
- Dongle initialisation.
- Basic cleanup of allocated arrays.
- Nominal dongle assignment (circular topology).
- Synchronisation primitives initialisation and destruction (`sync.c`).
- Partial-cleanup discipline for synchronisation initialisation.
- Dongle readiness check: state plus cooldown timestamp evaluated together.
- Refusal to treat one dongle as two resources (`left_id == right_id`).
- Millisecond clock helper (`simulation.c`).
- `ms_to_timespec` helper (milliseconds to absolute `struct timespec` deadline),
  used by the cooldown wait.
- `cooldowns_deadline` helper (`resources.c`): returns `DEADLINE_NEVER` for the
  cases that have no measurable date, and the later of the two `available_at`
  values otherwise.
- Cooldown wait chosen inside the `while`: `pthread_cond_wait` when the
  unavailability has no date, `pthread_cond_timedwait` when it does.
- `put_dongles_blocked` helper (`dongles.c`): marks the acquired pair in use,
  extracted to keep `block_dongles` under the 25-line limit.
- Makefile header dependency tracking.
- Object files and binary excluded from version control.
- Norminette compliance across all source files.
- Serialised logging helper with the exact subject format.
- The five log messages defined as named constants in `codexion.h`.
- Message constants hold pure text; the newline is added by the formatter.
- The coder cycle functions (`compiling_phase`, `debugging_phase`, `refactoring_phase`).
- Thread entry point with the required `void *f(void *arg)` signature.
- Coder cycle: compile, debug, refactor, with millisecond-to-microsecond conversion.
- Atomic acquisition of both dongles, waiting on the condition variable.
- Cooldown wait without a relay thread: `pthread_cond_timedwait` until the
  stored timestamp when the pair has a measurable date, `pthread_cond_wait`
  when it does not.
- Release under the dongle mutex, writing state and cooldown timestamp together.
- Cooldown timestamp measured from the release, not from the acquisition.
- Notification of waiting coders on release (`pthread_cond_broadcast`).
- The two `has taken a dongle` logs, in subject order before `is compiling`.
- Single-exit acquisition helper that cannot fail: it returns holding the pair.
- Thread creation per coder, storing the pointer to the heap array element.
- `pthread_create` result checked on return value (`!= 0`), stopping at the first failure.
- Thread joining (`run_simulation` joins every thread that was created).
- Joining before destroying synchronisation objects and freeing memory.
- Monitor thread creation and joining, with success detection
  (`SIM_FINISHED`) when every coder completes its required compilations.

### Scheduler refactor (2026-10-26)

- Single heap-allocated request array (`scheduler_queue`) owned by the control
  structure; `waiting_queue`, `waiting_count` and `next_in_queue` removed.
- `init_waiting_queue` sets `size = 0` and `capacity = number_of_coders`; a
  failed allocation leaves the queue empty and releases nothing.
- Monotonic ticket assignment via `register_request`.
- `block_dongles` resets `waiting` under `mtx_dongles`, including on the burnout
  exit path, so each new round registers a fresh ticket.
- `deadline` returns `last_compilation + time_to_burnout`.
- `temp_beats_best` implements the FIFO (ticket) and EDF (deadline + ticket)
  keys.
- `wait_for_dongles` keeps a coder asleep until it is both available and
  elected; the winner marks its pair, broadcasts and leaves the queue. It uses
  `cond_wait` for a past/never cooldown deadline and `timedwait` only for a real
  future cooldown, avoiding a busy-spin.
- At the close of the queue, lifecycle and election work (2026-10-28), `make re`
  was green, `norminette` was 11/11 and Valgrind reported no leaks or errors on
  the smoke tests.

### Heap integration (2026-10-30)

- `scheduler_queue.items` is now a binary min-heap. The comparator is
  `temp_beats_best`: ticket in FIFO, deadline with ticket tie-break in EDF.
- `heap_push`, `heap_pop`, `heap_peek` and `heap_remove` (`heap_tools_aux.c`)
  plus `heap_up`/`heap_down` (`heap_tools.c`) maintain the heap property.
- `t_coder.heap_pos` records the coder's index in the heap. `heap_push` sets it,
  while `heap_pop` and `heap_remove` reset it to `-1`, so removal is
  `O(log n)` and no linear search is needed.
- `enqueue_coder` delegates to `heap_push`; `dequeue_coder` delegates to
  `heap_remove`.
- `coder_electable` combines the `REQUEST_WAITING` check with
  `dongles_available`.
- `drain_unserviceable` pops non-electable roots into the `deferred` buffer
  until the root is electable or the heap is empty. `restore_deferred` pushes
  every deferred coder back. This keeps work-conserving election with a heap.
- `choose_next_in_queue` captures `heap_peek` before restoring the deferred
  coders. It must not re-read the root after restoring, because a better-key
  non-serviceable coder could return to the root.
- Verified on 2026-10-30: `make re` green, `norminette` 13/13, N=1..5 with FIFO
  and EDF, cooldown 1 and 300, five N=5 FIFO stress runs, Valgrind clean, and
  gdb confirms FIFO tickets `0..8` for N=3 and disjoint `0+2` then `1+3` for
  N=4.

The scheduler is fully integrated into the acquisition path. Remaining Hito 4
work is the full stress/fairness matrix (4.6) and thread-creation cleanup (4.7b).

### Concurrency hardening (4.7a, 2026-10-30)

- `last_compilation` is written in `block_dongles` while `mtx_dongles` is held,
  so dongle acquisition and the start-of-compilation timestamp are atomic.
- `check_simulation_state` locks `mtx_dongles` once and calls
  `scan_coders_locked`; burnout and success updates happen under that same lock.
- `simulation_is_running` (`sync.c`) reads `simulation_status` under
  `mtx_dongles`. It is used by `coder_routine`, `compiling_phase` and
  `monitor_routine`; it must not be called while the caller already holds the
  mutex, which is why `block_dongles` and `wait_for_dongles` keep direct reads.
- ThreadSanitizer on 2026-10-30 reports 0 data races for
  `./codexion 3 2000 50 20 20 2 1 fifo`. The full stress matrix and Helgrind are
  still pending under 4.6 and 7.6.

### Resolved concurrency issues (4.7a)

ThreadSanitizer originally reported three data races. They did not fail the
smoke tests, but they were undefined behaviour and could produce a false burnout
under output backpressure. All three are fixed:

1. `last_compilation`: the write moved into `block_dongles` under
   `mtx_dongles`, and the monitor reads it under the same lock.
2. `number_of_compiles_done`: the monitor reads it under `mtx_dongles`.
3. `simulation_status`: the monitor writes it under `mtx_dongles`, and coder
   reads go through `simulation_is_running`.

Reproduce the current check with:

```bash
cc -Wall -Wextra -Werror -pthread -fsanitize=thread -g coders/*.c \
  -o /tmp/codexion_tsan
/tmp/codexion_tsan 3 2000 50 20 20 2 1 fifo
```

Expected result: no `WARNING: ThreadSanitizer` lines.

### Still to implement

- Full error cleanup for thread creation (4.7b).
- Full stress/fairness matrix (4.6) and TSan/Helgrind across that matrix.
- Valgrind on thread-creation error paths (4.7b).

### Open items

- When output is redirected to a pipe, `printf` is block-buffered, so logs may
  not appear until the process exits. Run with an unbuffered stdout
  (`stdbuf -oL`) when capturing output; on a terminal it is line-buffered.
- A coder with `number_of_coders = 1` never acquires its second dongle (the two
  dongles would be the same). It sleeps on the condition variable (0 % CPU) and
  terminates through burnout: the monitor logs `1 burned out` and stops the
  simulation.
- `simulation_status` uses `SIM_RUNNING = 1`, `SIM_STOPPED = 2` and
  `SIM_FINISHED = 3`. `0` is deliberately not used for a live state, since `0` is
  what uninitialised memory holds. The monitor sets `SIM_FINISHED` on success and
  `SIM_STOPPED` on burnout.
- The monitor samples every 5 ms (`usleep(5000)`), so the burnout message can be
  up to 5 ms late, inside the 10 ms window required by the subject. The monitor
  sleeps at 0 % CPU between samples.

---

## Development Strategy

The implementation will be developed incrementally.

The recommended order is:

```text
1. Finish static initialisation.
2. Assign each coder its nominal dongles.
3. Add simulation timestamps.
4. Add one simple coder thread.
5. Add thread creation for all coders.
6. Add mutex-protected dongle ownership.
7. Implement atomic acquisition of both dongles.
8. Add cooldown handling.
9. Add the monitor.                                   (done)
10. Add successful and burnout termination.           (done)
11. Add FIFO scheduling.
12. Add EDF scheduling.
13. Add complete error handling.
14. Run stress tests and race-condition tests.
```

Each step should be tested before adding the next concurrency mechanism.

This keeps the architecture understandable and makes failures easier to isolate.