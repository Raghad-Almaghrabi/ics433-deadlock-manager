#ifndef MODULES_H
#define MODULES_H
#include "state.h"

typedef enum { MODULE_OK, MODULE_NOT_IMPLEMENTED, MODULE_ERROR } ModuleStatus;
typedef struct {
    bool safe;
    int sequence[MAX_PROCESSES];
    int sequence_length;
} SafetyResult;
typedef struct {
    int pids[MAX_PROCESSES];
    int count;
} DetectionResult;
typedef enum {
    REQUEST_GRANTED, REQUEST_INVALID, REQUEST_UNAVAILABLE, REQUEST_UNSAFE
} RequestDecision;
typedef struct {
    int victim_pid;
    int work_lost;
    int released[MAX_RESOURCES];
} RecoveryAction;
#define MAX_RECOVERY_ACTIONS MAX_PROCESSES
typedef struct {
    RecoveryAction actions[MAX_RECOVERY_ACTIONS];
    int count;
} RecoveryLog;

/* Dena: safety uses Need; analyze ACTIVE processes only.
 * A safe request updates state atomically. Unavailable/unsafe requests remain
 * outstanding in request[pid]; invalid requests must not change the state.
 * Never add a new request to an old pending request: it REPLACES that row.
 * Return MODULE_ERROR for a malformed state or invalid output arguments. */
ModuleStatus avoidance_safety(const SystemState *state, SafetyResult *result);
ModuleStatus avoidance_request(SystemState *state, int pid, const int request[],
                               RequestDecision *decision);
/* Farah: detection uses outstanding Request, NOT Need. Do not mutate state.
 * Assume a process whose current request can be met can finish and release
 * its allocations. Return only deadlocked ACTIVE PIDs (zero-based). */
ModuleStatus detection_run(const SystemState *state, DetectionResult *result);
/* Deema: rerun detection, choose least-work deadlocked victim (lowest PID tie),
 * terminate and reclaim; append an action for every victim, then detect again.
 * Report an error if log capacity is insufficient, before any mutation.
 * Initial recovery scope is termination. Preemption requires a rollback model.
 * Set allocation/need/request to zero for terminated processes; keep Max/work
 * as historical values. Never recover merely because a state is unsafe. */
ModuleStatus recovery_run(SystemState *state, RecoveryLog *log);
#endif
