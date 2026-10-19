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
- `cooldown_available_time_dongle`: written for the cooldown clock, but **not
  yet called by anyone** (see "Open items").

### `compiling.c`

Holds the workflow executed by each coder thread:

- `block_dongles`: acquires both required dongles atomically, waiting on the
  condition variable while they are unavailable.
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
  not contain the same dongle twice.
- `log_line`: the serialised log formatter.

### `sync.c`

Holds `init_sync` and `destroy_sync`. Initialisation reports which primitive
failed; destruction only destroys what was successfully created, using a count
of created primitives.

### `simulation.c`

Holds the time helpers:

- `current_time`: milliseconds since the epoch, from `gettimeofday`.
- `ms_to_timespec`: converts milliseconds into a `struct timespec` absolute
  deadline. Written in preparation for `pthread_cond_timedwait` (the cooldown
  clock, micro-objective 3.4) and **not yet called**.

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

The possible states are conceptually:

```text
BLOCKED
FREE
COOLDOWN
```

The current code represents the states with named `#define` constants:
`DONGLE_BLOCKED`, `DONGLE_FREE` and `DONGLE_COOLDOWN`. This avoids numeric
magic values while keeping the state type simple. A C `enum` has not yet been
introduced in the current implementation.

Only `BLOCKED` and `FREE` are written by the simulation today. `DONGLE_COOLDOWN`
is declared but **never assigned**: the cooldown is expressed by the
`available_at` timestamp instead, so a cooling dongle keeps the `FREE` state. A
dedicated `COOLDOWN` state is deliberately avoided because nothing would return
the dongle to `FREE`.

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
checked together inside the critical section, and a `while` loop around
`pthread_cond_wait` handles the unavailable case. Both dongles are marked as
in use only after the check succeeds, so no coder ever holds a partial pair.

The release half is implemented in `compiling_phase`: after the compilation
sleep, a second critical section marks both dongles free, stores their cooldown
timestamp, and notifies every waiting coder with `pthread_cond_broadcast`. The
state and the timestamp are written together, and the notification happens
after the writes and inside the mutex, so a woken coder always observes the new
state.

One piece of the cooldown is still missing: a release notifies waiters, but
nothing notifies them when a cooldown **expires**. A dongle that is free but
still cooling is seen as unavailable, and if every coder ends up waiting on it,
nobody rings the bell again. The fix is either a timed wait
(`pthread_cond_timedwait` with a deadline computed from `available_at`) or a
relay thread that wakes waiters when the shortest cooldown elapses.

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
3. Start the monitor.                                (pending: Hito 6)
4. Wait for all coder threads with pthread_join.      (done)
5. Wait for the monitor if necessary.                 (pending)
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
3. Create the monitor when that phase is implemented.
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

FIFO means First In, First Out.

The first coder that enters the waiting queue should be considered before coders that arrive later.

A FIFO queue will eventually be stored on the heap, as required by the subject.

The queue must:

- Store waiting coder references or identifiers.
- Preserve insertion order.
- Remove coders in the same order in which they entered.
- Be protected by the appropriate synchronisation mechanism.

---

## EDF Scheduler

EDF means Earliest Deadline First.

The coder with the closest burnout deadline should receive priority.

The deadline is conceptually calculated as:

```text
last_compilation_time + time_to_burnout
```

The EDF structure will also be dynamically allocated on the heap.

When selecting the next coder, the scheduler must compare the waiting coders' deadlines and choose the earliest one.

The queue and its ordering must be protected from concurrent access.

---

## Monitor

The subject requires a separate monitor thread that detects burnout with
precision and stops the simulation.

It will verify whether:

- A coder has exceeded its burnout limit.
- The simulation has finished.
- Every coder has completed the required number of compilations.
- A fatal error has occurred.

The burnout message must be printed within 10 ms of the real burnout moment,
which bounds how often the monitor may poll.

The monitor must not inspect mutable coder or dongle state without synchronisation.

When the simulation ends, the monitor will update the shared simulation status and notify waiting threads with `pthread_cond_broadcast`.

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

The pending design decision is which mutex protects the per-coder counters
`number_of_compiles_done` and `last_compilation`. Reusing `mtx_dongles` adds
no primitives but makes the monitor contend with acquisition. A dedicated
mutex reduces contention but introduces a lock ordering that must be proved.

Mutexes will protect:

- Dongle ownership.
- Dongle cooldown timestamps.
- Coder state.
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
- `ms_to_timespec` helper (milliseconds to `struct timespec` deadline), written
  for the cooldown clock; not yet called.
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
- Release under the dongle mutex, writing state and cooldown timestamp together.
- Cooldown timestamp measured from the release, not from the acquisition.
- Notification of waiting coders on release (`pthread_cond_broadcast`).
- The two `has taken a dongle` logs, in subject order before `is compiling`.
- Single-exit acquisition helper that cannot fail: it returns holding the pair.
- Thread creation per coder, storing the pointer to the heap array element.
- `pthread_create` result checked on return value (`!= 0`), stopping at the first failure.
- Thread joining (`run_simulation` joins every thread that was created).
- Joining before destroying synchronisation objects and freeing memory.

### Still to implement

- Cooldown expiry notification (timed wait or relay thread).
- Burnout detection by the monitor thread.
- Simulation status flag checked by waiting and looping threads.
- FIFO queue.
- EDF queue.
- Complete termination protocol.
- Full error cleanup for thread creation.

### Open items

- A dongle that is free but still cooling is never re-announced when the
  cooldown expires. If every coder waits on it, nobody wakes them. The
  condition variable needs a timed wait or a relay. This has been observed at
  runtime: coders log one complete cycle and then the program blocks in
  `pthread_join` because a sleeping coder is never woken.
- When output is redirected to a pipe, `printf` is block-buffered, so logs may
  not appear until the process exits. Run with an unbuffered stdout
  (`stdbuf -oL`) when capturing output; on a terminal it is line-buffered.
- A coder with `number_of_coders = 1` waits forever for its second dongle by
  design; only the monitor (Hito 6) can end that situation.
- Dead code carried ahead of its wiring: `cooldown_available_time_dongle` and
  `ms_to_timespec` are defined and prototyped but never called, and the
  `DONGLE_COOLDOWN` state is never assigned. They are the scaffolding for the
  cooldown clock (3.4). `simulation_status` is set to `1` in `init.c` and never
  read (2.9).

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
9. Add the monitor.
10. Add successful and burnout termination.
11. Add FIFO scheduling.
12. Add EDF scheduling.
13. Add complete error handling.
14. Run stress tests and race-condition tests.
```

Each step should be tested before adding the next concurrency mechanism.

This keeps the architecture understandable and makes failures easier to isolate.