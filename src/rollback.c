
#include "modules.h"

/*
 * Deadlock recovery using resource preemption
 * and rollback to the initial checkpoint.
 *
 * The victim remains ACTIVE.
 * Its resources are released and work resets to zero.
 * Outstanding requests are preserved.
 *
 * Assumption: all simulated resources are reclaimable.
 */
ModuleStatus recovery_rollback(SystemState *state,
                               RecoveryLog *log)
{
    if (state == NULL || log == NULL)
        return MODULE_ERROR;

    if (log->count < 0 ||
        log->count > MAX_RECOVERY_ACTIONS ||
        !state_validate(state, NULL, 0))
        return MODULE_ERROR;

    SystemState next = *state;
    RecoveryLog new_log = *log;
    DetectionResult result;

    while (1)
    {
        if (detection_run(&next, &result) != MODULE_OK)
            return MODULE_ERROR;

        if (result.count < 0 ||
            result.count > next.process_count)
            return MODULE_ERROR;

        /* Deadlock resolved */
        if (result.count == 0)
            break;

        int victim = -1;

        /* Select deadlocked victim with least work */
        for (int i = 0; i < result.count; i++)
        {
            int pid = result.pids[i];

            if (pid < 0 || pid >= next.process_count ||
                next.status[pid] != PROCESS_ACTIVE)
                return MODULE_ERROR;

            /* Reject duplicate detection results */
            for (int k = 0; k < i; k++)
                if (result.pids[k] == pid)
                    return MODULE_ERROR;

            /* Only preempt processes holding resources */
            bool holds_resources = false;

            for (int j = 0; j < next.resource_count; j++)
            {
                if (next.allocation[pid][j] > 0)
                {
                    holds_resources = true;
                    break;
                }
            }

            if (!holds_resources)
                continue;

            if (victim == -1 ||
                next.work_done[pid] < next.work_done[victim] ||
                (next.work_done[pid] == next.work_done[victim] &&
                 pid < victim))
            {
                victim = pid;
            }
        }

        /* Cannot recover through preemption */
        if (victim == -1)
            return MODULE_ERROR;

        if (new_log.count >= MAX_RECOVERY_ACTIONS)
            return MODULE_ERROR;

        RecoveryAction *action =
            &new_log.actions[new_log.count];

        action->type = RECOVERY_ROLLBACK;
        action->victim_pid = victim;
        action->work_lost = next.work_done[victim];

        /* Release resources and restore checkpoint */
        for (int j = 0; j < next.resource_count; j++)
        {
            int released = next.allocation[victim][j];

            action->released[j] = released;
            next.available[j] += released;
            next.allocation[victim][j] = 0;
            next.need[victim][j] = next.max[victim][j];
        }

        /* Roll back completed work */
        next.work_done[victim] = 0;

        /* Process is not terminated */
        next.status[victim] = PROCESS_ACTIVE;

        new_log.count++;

        if (!state_validate(&next, NULL, 0))
            return MODULE_ERROR;
    }

    /* Commit only after successful recovery */
    *state = next;
    *log = new_log;

    return MODULE_OK;
}
