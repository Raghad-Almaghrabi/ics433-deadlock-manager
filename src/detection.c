#include "modules.h"

/*
 * Run the standard multi-instance deadlock detection algorithm.
 *
 * This function uses the current outstanding Request matrix, not Need.
 * It treats an active process with no allocated resources as finished for
 * detection purposes, because such a process cannot be part of a circular
 * wait. The input state is read only.
 */
ModuleStatus detection_run(const SystemState *state, DetectionResult *result)
{
    char error[128];
    if (!result || !state_validate(state, error, sizeof(error)))
        return MODULE_ERROR;

    DetectionResult output = {0};
    int work[MAX_RESOURCES] = {0};
    bool finish[MAX_PROCESSES] = {false};

    /* Work begins with the currently available resource instances. */
    for (int r = 0; r < state->resource_count; ++r)
        work[r] = state->available[r];

    /*
     * Processes that are inactive or hold no resources are already finishable.
     * Active processes holding resources must still be shown able to complete
     * before their allocations can be added back to Work.
     */
    for (int p = 0; p < state->process_count; ++p)
    {
        finish[p] = state->status[p] != PROCESS_ACTIVE;
        if (state->status[p] == PROCESS_ACTIVE)
        {
            bool holds_resources = false;
            for (int r = 0; r < state->resource_count; ++r)
            {
                if (state->allocation[p][r] > 0)
                {
                    holds_resources = true;
                    break;
                }
            }
            if (!holds_resources)
                finish[p] = true;
        }
    }

    /*
     * Repeatedly find an unfinished process whose entire current request can
     * be met. Assume it finishes and releases its allocation, then try again.
     */
    bool found_process;
    do
    {
        found_process = false;

        for (int p = 0; p < state->process_count; ++p)
        {
            if (finish[p] || state->status[p] != PROCESS_ACTIVE)
                continue;

            bool request_can_be_met = true;
            for (int r = 0; r < state->resource_count; ++r)
            {
                if (state->request[p][r] > work[r])
                {
                    request_can_be_met = false;
                    break;
                }
            }

            if (!request_can_be_met)
                continue;

            for (int r = 0; r < state->resource_count; ++r)
                work[r] += state->allocation[p][r];

            finish[p] = true;
            found_process = true;
        }
    } while (found_process);

    /* Any active process still unfinished belongs to the deadlocked set. */
    for (int p = 0; p < state->process_count; ++p)
    {
        if (state->status[p] == PROCESS_ACTIVE && !finish[p])
            output.pids[output.count++] = p;
    }

    *result = output;
    return MODULE_OK;
}
