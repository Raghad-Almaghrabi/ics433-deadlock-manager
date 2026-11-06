# ICS 433 Deadlock Manager

A C terminal simulator for resource allocation and deadlock management.

## Implemented so far

- **Raghad:** terminal menu, scenario input and validation, shared system state, state display, and resource release/completion.
- **Farah:** multi-instance deadlock detection using the current outstanding requests. It reports deadlocked process IDs without changing the system state.

The program builds with `make`. The detector was checked with the provided scenarios: `deadlocked.txt` reports P0 and P1; `safe.txt` and `unsafe.txt` report no confirmed deadlock.

## Still to implement

- Banker’s algorithm for deadlock avoidance.
- Deadlock recovery by terminating a deadlocked process.
- Final integration and progress report.

## Run the program

From this folder, run:

```sh
make
./build/deadlock_manager
```

In the menu, choose **2** to load a scenario file and **8** to detect deadlock. For example, load `scenarios/deadlocked.txt`.
