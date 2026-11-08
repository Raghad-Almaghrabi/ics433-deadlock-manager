#include "modules.h"

/* Safety uses remaining maximum Need, not the currently outstanding Request.
 * Work and Finish simulate completion without changing the shared state. */
ModuleStatus avoidance_safety(const SystemState *state, SafetyResult *result)
{
    if (!result || !state_validate(state, NULL, 0))
        return MODULE_ERROR;

    SafetyResult output = {0};
    int work[MAX_RESOURCES] = {0};
    bool finish[MAX_PROCESSES] = {false};
    for (int r = 0; r < state->resource_count; ++r)
        work[r] = state->available[r];
    for (int p = 0; p < state->process_count; ++p)
        finish[p] = state->status[p] != PROCESS_ACTIVE;

    bool progress;
    do {
        progress = false;
        for (int p = 0; p < state->process_count; ++p) {
            if (finish[p])
                continue;
            bool can_finish = true;
            for (int r = 0; r < state->resource_count; ++r)
                if (state->need[p][r] > work[r]) {
                    can_finish = false;
                    break;
                }
            if (!can_finish)
                continue;
            for (int r = 0; r < state->resource_count; ++r)
                work[r] += state->allocation[p][r];
            finish[p] = true;
            output.sequence[output.sequence_length++] = p;
            progress = true;
        }
    } while (progress);

    output.safe = true;
    for (int p = 0; p < state->process_count; ++p)
        if (!finish[p]) {
            output.safe = false;
            /* A partial completion order is not a safe sequence. */
            output.sequence_length = 0;
            break;
        }
    *result = output;
    return MODULE_OK;
}

ModuleStatus avoidance_request(SystemState *state, int pid, const int request[],
                               RequestDecision *decision)
{
    if (!request || !decision || !state_validate(state, NULL, 0))
        return MODULE_ERROR;
    if (pid < 0 || pid >= state->process_count)
        return MODULE_ERROR;
    if (state->status[pid] != PROCESS_ACTIVE) {
        *decision = REQUEST_INVALID;
        return MODULE_OK;
    }

    /* Copy first: a caller may pass state->request[pid] to retry a request.
     * Validate every component before classifying availability or mutating. */
    int proposed[MAX_RESOURCES] = {0};
    bool available = true;
    bool zero = true;
    for (int r = 0; r < state->resource_count; ++r) {
        proposed[r] = request[r];
        if (proposed[r] < 0 || proposed[r] > state->need[pid][r]) {
            *decision = REQUEST_INVALID;
            return MODULE_OK;
        }
        if (proposed[r] > state->available[r])
            available = false;
        if (proposed[r] != 0)
            zero = false;
    }

    /* A zero vector cancels the outstanding request; no resources change. */
    if (zero) {
        for (int r = 0; r < state->resource_count; ++r)
            state->request[pid][r] = 0;
        *decision = REQUEST_GRANTED;
        return MODULE_OK;
    }
    if (!available) {
        for (int r = 0; r < state->resource_count; ++r)
            state->request[pid][r] = proposed[r];
        *decision = REQUEST_UNAVAILABLE;
        return MODULE_OK;
    }

    SystemState next = *state;
    for (int r = 0; r < next.resource_count; ++r) {
        next.available[r] -= proposed[r];
        next.allocation[pid][r] += proposed[r];
        next.need[pid][r] -= proposed[r];
        next.request[pid][r] = 0;
    }
    SafetyResult safety = {0};
    if (avoidance_safety(&next, &safety) != MODULE_OK)
        return MODULE_ERROR;
    if (safety.safe) {
        *state = next;
        *decision = REQUEST_GRANTED;
    } else {
        /* Roll back the tentative allocation; retain only the pending request.
         * A new submission replaces the old row rather than adding to it. */
        for (int r = 0; r < state->resource_count; ++r)
            state->request[pid][r] = proposed[r];
        *decision = REQUEST_UNSAFE;
    }
    return MODULE_OK;
}
