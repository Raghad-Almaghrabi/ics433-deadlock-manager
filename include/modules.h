
#ifndef MODULES_H
#define MODULES_H

#include "state.h"

typedef enum {
    MODULE_OK,
    MODULE_NOT_IMPLEMENTED,
    MODULE_ERROR
} ModuleStatus;

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
    REQUEST_GRANTED,
    REQUEST_INVALID,
    REQUEST_UNAVAILABLE,
    REQUEST_UNSAFE
} RequestDecision;

/* Recovery methods */
typedef enum {
    RECOVERY_TERMINATION,
    RECOVERY_ROLLBACK
} RecoveryType;

/* Information recorded for each recovery action */
typedef struct {
    RecoveryType type;
    int victim_pid;
    int work_lost;
    int released[MAX_RESOURCES];
} RecoveryAction;

#define MAX_RECOVERY_ACTIONS MAX_PROCESSES

typedef struct {
    RecoveryAction actions[MAX_RECOVERY_ACTIONS];
    int count;
} RecoveryLog;

/* Deadlock avoidance - Dena */
ModuleStatus avoidance_safety(const SystemState *state,
                              SafetyResult *result);

ModuleStatus avoidance_request(SystemState *state,
                               int pid,
                               const int request[],
                               RequestDecision *decision);

/* Deadlock detection - Farah */
ModuleStatus detection_run(const SystemState *state,
                           DetectionResult *result);

/* Deadlock recovery using process termination - Deema */
ModuleStatus recovery_run(SystemState *state,
                          RecoveryLog *log);

/* Deadlock recovery using preemption and rollback - Deema */
ModuleStatus recovery_rollback(SystemState *state,
                               RecoveryLog *log);

#endif
