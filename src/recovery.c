#include "modules.h"

/*
 * Deadlock Recovery Module
 * Detects deadlocked processes, selects the process
 * with the least completed work, terminates it,
 * releases its resources, and records the recovery.
 */
ModuleStatus recovery_run(SystemState *state, RecoveryLog *log)
{
    DetectionResult result;

    // Step 1: Validate input
    if (state == NULL || log == NULL)
        return MODULE_ERROR;

    // Check if the system state is valid
    if (!state_validate(state, NULL, 0))
        return MODULE_ERROR;

    // Start a new recovery log
    log->count = 0;

    // Step 2: Detect deadlocked processes
    if (detection_run(state, &result) != MODULE_OK)
        return MODULE_ERROR;

    // Continue until no deadlock remains
    while (result.count > 0)
    {
        // Check log capacity before modifying state
        if (log->count >= MAX_RECOVERY_ACTIONS)
            return MODULE_ERROR;

        // Step 3: Select victim with least work done
        int victim = result.pids[0];

        for (int i = 1; i < result.count; i++)
        {
            int pid = result.pids[i];

            if (state->work_done[pid] < state->work_done[victim] ||
                (state->work_done[pid] == state->work_done[victim] &&
                 pid < victim))
            {
                victim = pid;
            }
        }

        // Step 4: Record the recovery action
        RecoveryAction *action = &log->actions[log->count];

        action->victim_pid = victim;
        action->work_lost = state->work_done[victim];

        // Step 5: Release all allocated resources
        for (int j = 0; j < state->resource_count; j++)
        {
            action->released[j] = state->allocation[victim][j];

            state->available[j] += state->allocation[victim][j];

            state->allocation[victim][j] = 0;
            state->need[victim][j] = 0;
            state->request[victim][j] = 0;
        }

        // Step 6: Mark process as terminated
        state->status[victim] = PROCESS_TERMINATED;

        // Save recovery action
        log->count++;

        // Step 7: Run detection again
        if (detection_run(state, &result) != MODULE_OK)
            return MODULE_ERROR;
    }

    return MODULE_OK;
}
