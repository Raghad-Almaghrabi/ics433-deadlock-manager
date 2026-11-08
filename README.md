# ICS433 — Deadlock Detection, Avoidance, and Recovery Manager

A C terminal simulator for processes requesting and releasing resource instances. It displays resource allocation, distinguishes unsafe states from confirmed deadlocks, and demonstrates recovery by process termination. The processes and resources are simulated; the program does not control real operating-system processes or require threads.

## Implemented core features

- Shared state containing process/resource counts, Total, Available, Max, Allocation, Need, outstanding Request, process status, and completed simulated work units.
- Keyboard and text-file scenario input with validation and preservation of the previous state when loading fails.
- Terminal tables displaying the state and a menu connecting the algorithm modules.
- Banker's safety analysis with a safe sequence when all active processes can finish under their declared maximum demands.
- Resource-request evaluation with tentative allocation, safety checking, commit or rollback, and pending-request tracking.
- Multi-instance deadlock detection based on current outstanding requests.
- Termination-based recovery with automatic least-work victim selection, resource reclamation, repeated detection, and recovery records.
- Normal voluntary resource release and explicit process completion.

The core simulator is implemented. Phase 2 also requires the progress report and screenshots; final submission preparation is separate from passing the code tests.

## Build and run

Requirements: a C11 compiler and Make. Python 3 is used only by the terminal integration tests.

On Ubuntu, install these tools if needed:

```sh
sudo apt install build-essential python3
```

From the project root:

```sh
make
make run
```

For the full automated checks:

```sh
make test
```

Remove generated binaries with `make clean`. The `build/` directory is excluded from Git.

## Terminal menu

| Option | Operation |
| --- | --- |
| 1 | Enter a scenario using the keyboard |
| 2 | Load a scenario text file |
| 3 | Display the state |
| 4 | Submit or retry a resource request through Banker's algorithm |
| 5 | Release resources voluntarily |
| 6 | Mark a process completed and return its resources |
| 7 | Check safety and display a safe sequence if one exists |
| 8 | Detect deadlock from current requests |
| 9 | Recover from confirmed deadlock |
| 10 | Display the recovery history for the current scenario |
| 0 | Exit |

All process and resource IDs are zero-based: P0, P1, ... and R0, R1, .... Resource vectors must contain one integer per resource type, in resource order.

## Shared state and validation

The initial implementation uses fixed-size arrays for up to 20 processes, 10 resource types, and 1,000,000 instances of each resource type. Work units are nonnegative C integers.

- For every resource type, `Available + sum(Allocation) = Total`.
- For active processes, `Need = Max - Allocation`.
- Resource entries satisfy `0 <= Allocation <= Max <= Total` and `0 <= Request <= Need`.
- A pending request may exceed Available; this represents a waiting process.
- Scenario loading derives Available and Need and accepts resource-consistent safe, unsafe, and deadlocked states.
- A failed load preserves both the previous state and recovery history. Successfully loading a new scenario resets recovery history.
- Normal release reduces Allocation and increases Available and the active process's Need. It does not automatically retry pending requests.
- Explicit completion is permitted only for an active process without a pending request. It returns all allocations, clears Need and Request, and marks the process completed. Max is an upper bound, so completion does not require Need to have reached zero.
- Completed and terminated processes retain historical Max and completed work, hold/request nothing, have Need zero, and are excluded from algorithm processing.

Fixed-size arrays simplify ownership of the shared state. This differs from the dynamic-array checks mentioned in Phase 1 and should be reflected in the architecture description.

## Resource requests and safety

A submitted request replaces that process's previous pending vector; it is not added to it.

1. Reject negative values and requests exceeding remaining Need without changing the state.
2. If resources are unavailable, retain the request as pending and return `REQUEST_UNAVAILABLE` without changing allocations.
3. Otherwise, allocate on a copy, clear that process's pending request in the copy, and run the safety check.
4. Commit a safe allocation and return `REQUEST_GRANTED`; otherwise discard the tentative allocation, retain the submitted request as pending, and return `REQUEST_UNSAFE`.

A zero vector cancels the current pending request without allocating resources. Waiting requests must be retried explicitly through option 4 after the state changes.

Safety uses remaining maximum Need. Detection uses current outstanding Request. Failure to find a safe sequence means the state is unsafe; it does not itself establish deadlock. Likewise, detection finding no current deadlock does not prove safety.

## Detection and recovery policy

Detection starts with Available and simulates completion for processes whose current requests can be met. It assumes such a process can eventually finish and release its allocations. Active processes with no allocated resources cannot hold resources in a deadlock and are excluded from the deadlocked set. Detection does not change the state.

Recovery follows the original proposal: automatically select a deadlocked process with the least completed work, breaking ties by the lowest PID. Terminate it, return its allocations to Available, clear its Allocation/Need/Request, record the victim, resources returned and work lost, and rerun detection. Repeat until no deadlock remains.

Choosing the least-work victim aims to reduce discarded work, although it may require more terminations than another selection policy. Recovery is triggered by confirmed deadlock, not merely by an unsafe state.

Recovery records are kept in memory for the current scenario. Later recovery calls preserve earlier entries and append new actions; a call with no deadlock leaves history intact. Recovery works on copies and commits only after successful completion, so an error such as insufficient log capacity preserves the original state and history.

Termination is the implemented recovery strategy. Resource preemption, rollback, priorities, and timeouts are not implemented; they are optional extensions rather than part of this core implementation.

## Scenario format

Files contain whitespace-separated integers without labels or comments:

```text
process_count resource_count
Total vector
P0 Max vector
P0 Allocation vector
P0 outstanding Request vector
P0 work_done
...repeat the four process fields for every remaining process...
```

All imported processes start active. Available and Need are calculated from Total, Max, and Allocation.

| Scenario | Safety result | Detection result |
| --- | --- | --- |
| `scenarios/safe.txt` | Safe; one sequence is P1 P3 P4 P0 P2 | No confirmed deadlock |
| `scenarios/unsafe.txt` | Unsafe | No confirmed deadlock: current requests are zero |
| `scenarios/deadlocked.txt` | Unsafe | P0 and P1 are deadlocked |
| `scenarios/invalid.txt` | Input rejected: combined allocations exceed Total | Previous state preserved |

### Safe request demonstration

Load `scenarios/safe.txt` using option 2. Choose option 4, PID `1`, vector `1 0 2`. The request is granted and Available becomes `2 3 0`.

### Unsafe request demonstration

Load a fresh `scenarios/safe.txt`. Choose option 4, PID `4`, vector `3 3 0`. The request fits Need and Available, but the tentative allocation is unsafe. It is deferred; Available remains `3 3 2`, allocations stay unchanged, and P4's pending Request becomes `3 3 0`.

### Deadlock and recovery demonstration

Load `scenarios/deadlocked.txt`. Option 8 reports P0 and P1. Option 9 terminates P1, returns `0 1`, and records 2 work units lost. Option 8 then reports no confirmed deadlock. Use option 10 to display the recovery record; selecting option 9 again preserves that record.

## Source layout and assigned responsibilities

| Path | Purpose | Assigned owner |
| --- | --- | --- |
| `include/state.h`, `src/state.c` | Shared state, validation, display, normal release and completion | Raghad |
| `include/input.h`, `src/input.c` | Keyboard and file scenario input | Raghad |
| `src/main.c`, `Makefile` | Terminal integration and build/test commands | Raghad |
| `include/modules.h` | Shared algorithm interfaces and result types | Team |
| `src/avoidance.c` | Banker's safety and resource requests | Dena |
| `src/detection.c` | Multi-instance deadlock detection | Farah |
| `src/recovery.c` | Victim selection, termination and recovery history | Deema |
| `scenarios/` | Safe, unsafe, deadlocked and invalid fixtures | Team |
| `tests/` | Module and terminal integration checks | Module owners and integration |

The table describes responsibility assignments; contributions should be recorded according to the work actually performed.

## Verification

`make test` builds and runs all five suites:

| Suite | Coverage |
| --- | --- |
| `test_state` | Resource accounting, validation, releases, completion, scenario loading and parsing |
| `test_detection` | Safe, unsafe and deadlocked cases, progress through resource release and zero-allocation processes |
| `test_avoidance` | Replay of safe sequences, unsafe classification, grant/defer decisions, rollback, pending-row replacement/clearing, inactive processes and invalid arguments |
| `test_recovery` | Victim selection, PID ties, work lost, resource reclamation, repeated calls, appended history, multiple victims and atomic capacity failure |
| `test_cli.py` | Menu integration, grants, unsafe/unavailable requests, pending retries, detection, recovery-history preservation/reset and malformed input |

All five suites pass on the corrected code. These tests establish the checked behavior; they do not claim correctness for every possible input. Verify the final copy on the team's Ubuntu environment before submission and perform the planned Valgrind check there.

## Phase 2 submission

Deadline: **November 10, 2026**. Submit one group progress report PDF and organized, commented C source code through GitHub or ZIP. The report should include the project recap, implemented features, updated architecture, screenshots, challenges and remaining work.

Refresh the report's test results and remaining-work section after adopting these fixes. Capture evidence for unsafe without deadlock, unsafe-request deferral and post-recovery detection. Final verification, report updates and demonstration preparation remain part of submission preparation.
