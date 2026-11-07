#include "modules.h"

/*
 * Deadlock Recovery Module
 * Detects deadlocked processes and selects the victim
  with the least completed work.
 * If two processes have equal work, the lower PID wins.
 *
 * Terminates the victim, releases its resources,
 * records the recovery action, and repeats detection.
 */
ModuleStatus recovery_run(SystemState *state, RecoveryLog *log)
{
    DetectionResult result;
    SystemState next;
    RecoveryLog new_log = {0};

    // Step 1: Validate inputs
    if (state == NULL || log == NULL)
        return MODULE_ERROR;

    if (!state_validate(state, NULL, 0))
        return MODULE_ERROR;

    // Work on a copy to protect the original state
    next = *state;

    // Step 2: Detect deadlocked processes
    while (1)
    {
        if (detection_run(&next, &result) != MODULE_OK)
            return MODULE_ERROR;

        // No deadlocked processes remain
        if (result.count == 0)
            break;

        // Validate the detection result
        if (result.count < 0 ||
            result.count > next.process_count)
            return MODULE_ERROR;

        // Ensure there is enough room in the recovery log
        if (new_log.count >= MAX_RECOVERY_ACTIONS)
            return MODULE_ERROR;

        // Step 3: Select victim with least completed work
        int victim = -1;

        for (int i = 0; i < result.count; i++)
        {
            int pid = result.pids[i];

            // Check that PID is valid and active
            if (pid < 0 || pid >= next.process_count ||
                next.status[pid] != PROCESS_ACTIVE)
                return MODULE_ERROR;

            if (victim == -1 ||
                next.work_done[pid] < next.work_done[victim] ||
                (next.work_done[pid] == next.work_done[victim] &&
                 pid < victim))
            {
                victim = pid;
            }
        }

        // Step 4: Prepare the recovery log entry
        RecoveryAction *action = &new_log.actions[new_log.count];

        action->victim_pid = victim;
        action->work_lost = next.work_done[victim];

        // Step 5: Release the victim's allocated resources
        for (int j = 0; j < next.resource_count; j++)
        {
            action->released[j] = next.allocation[victim][j];

            next.available[j] += next.allocation[victim][j];

            // Clear resources belonging to terminated process
            next.allocation[victim][j] = 0;
            next.need[victim][j] = 0;
            next.request[victim][j] = 0;
        }

        // Step 6: Mark the process as terminated
        next.status[victim] = PROCESS_TERMINATED;

        // Keep max and work_done as historical information
        new_log.count++;

        // Step 7: Verify that the updated state is valid
        if (!state_validate(&next, NULL, 0))
            return MODULE_ERROR;

        // The loop automatically runs detection again
    }

    // Step 8: Save changes only after successful recovery
    *state = next;
    *log = new_log;

    return MODULE_OK;
}
