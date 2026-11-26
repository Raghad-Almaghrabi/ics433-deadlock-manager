# ICS433 — Deadlock Detection, Avoidance, and Recovery Manager

A C terminal simulator for processes requesting and releasing resource instances. It displays resource allocation, distinguishes unsafe states from confirmed deadlocks, and demonstrates recovery by process termination or resource preemption with simplified rollback. The processes and resources are simulated; the program does not control real operating-system processes or require threads.

## Implemented core features

- Shared state containing process/resource counts, Total, Available, Max, Allocation, Need, outstanding Request, process status, and completed simulated work units.
- Keyboard and text-file scenario input with validation and preservation of the previous state when loading fails.
- Terminal tables displaying the state and a menu connecting the algorithm modules.
- Banker's safety analysis with a safe sequence when all active processes can finish under their declared maximum demands.
- Resource-request evaluation with tentative allocation, safety checking, commit or rollback, and pending-request tracking.
- Multi-instance deadlock detection based on current outstanding requests.
- Termination-based recovery with automatic least-work victim selection, resource reclamation, repeated detection, and recovery records.
- Resource preemption with simplified rollback: reclaim resources, reset the selected process's completed work to zero, and keep it active.
- Normal voluntary resource release and explicit process completion.

The simulator implements the core resource-management algorithms. The final submission includes the source-code ZIP archive and technical report, with a live demonstration during evaluation.

## Build and run

Requirements: a C11 compiler and Make. Python 3 is used only by the terminal integration tests.

On Ubuntu, install these tools if needed:

```bash
sudo apt install build-essential python3
```

From the project root:

```bash
make
make run
```

For the full automated checks:

```bash
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
| 9 | Recover from confirmed deadlock by process termination |
| 10 | Display the recovery history for the current scenario |
| 11 | Recover from confirmed deadlock by resource preemption and simplified rollback |
| 0 | Exit |

All process and resource IDs are zero-based: P0, P1, ... and R0, R1, .... Resource vectors must contain one integer per resource type, in resource order.

## Shared state and validation

The simulator supports up to 20 processes, 10 resource types, and 1,000,000 instances per type. It validates nonnegative input and maintains:

- `Available + sum(Allocation) = Total`
- `Need = Max - Allocation` for active processes
- `0 <= Allocation <= Max <= Total` and `0 <= Request <= Need`

Invalid scenario loads preserve the current state and history; successful loads reset recovery history. Releases return resources to Available. Completion returns all held resources and removes the process from active analysis.

## Resource requests and safety

Requests exceeding Need or containing negative values are rejected. Unavailable or unsafe requests remain pending without changing allocations. A tentative allocation is committed only if Banker's safety check succeeds; granted requests are cleared.

New requests replace previous pending requests. Retry them through option 4; a zero vector cancels the pending request.

Safety uses **Need**, while detection uses current **Request**. An unsafe state does not necessarily mean deadlock, and no detected deadlock does not prove safety.

## Detection and recovery policy

Detection simulates completion when current requests can be satisfied, assuming those processes can finish and release their allocations. It does not modify the state.

Both recovery methods select a deadlocked process with the least completed work, breaking ties by lowest PID, return its resources, record the action and work lost, and rerun detection until no deadlock remains. This greedy policy minimizes the selected victim's lost work at each step; it does not guarantee a globally minimum total loss.

- **Termination (option 9):** marks the victim terminated and clears its Allocation, Need, and Request.
- **Simplified rollback (option 11):** releases all victim allocations, restores Need to Max, resets completed work to zero, preserves the outstanding Request, and keeps the process active. All simulated resources are assumed reclaimable; intermediate checkpoints and automatic process execution/restarting are not implemented.

Recovery history records the recovery type, victim, work lost, and resources returned. It is preserved until a new scenario is successfully loaded or the program exits. Both methods work on copies and commit only on success; an error preserves the original state and history.

## Scenario format

Files contain whitespace-separated integers without comments:

```text
process_count resource_count
Total vector
P0 Max vector
P0 Allocation vector
P0 outstanding Request vector
P0 work_done
...repeat the four process fields for each remaining process...
```

Processes start active; Available and Need are calculated automatically.

| Scenario | Safety result | Detection result |
| --- | --- | --- |
| `scenarios/safe.txt` | Safe; one sequence is P1 P3 P4 P0 P2 | No deadlock |
| `scenarios/unsafe.txt` | Unsafe | No deadlock; current requests are zero |
| `scenarios/deadlocked.txt` | Unsafe | P0 and P1 deadlocked |
| `scenarios/invalid.txt` | Invalid: allocations exceed Total | Previous state preserved |

## Demonstrations

- **Safe request:** load `scenarios/safe.txt`, choose option 4, PID `1`, vector `1 0 2`. Granted; Available becomes `2 3 0`.
- **Unsafe request:** reload `scenarios/safe.txt`, choose option 4, PID `4`, vector `3 3 0`. Deferred; allocations and Available remain unchanged, and the request is recorded.
- **Recovery:** load `scenarios/deadlocked.txt`. Option 8 reports P0 and P1; option 9 terminates P1, returns `0 1`, and records 2 work units lost. Option 8 then reports no deadlock; option 10 displays the log. Repeating option 9 preserves the record.
- **Rollback:** load a fresh `scenarios/deadlocked.txt`, then choose option 11. P1 remains active, its completed work becomes zero, and Available becomes `0 1`. Option 8 reports no confirmed deadlock; option 10 shows the rollback record.

The automated suites cover shared state, input, avoidance, detection, termination recovery, and terminal integration. They currently do not exercise `recovery_rollback()`; verify option 11 separately until automated rollback coverage is added.
