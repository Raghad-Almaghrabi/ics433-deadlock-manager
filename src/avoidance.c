#include "modules.h"
#include <stdbool.h>
#include <string.h>

/*
 * Banker's Algorithm Safety Check
 * Simulates whether all active processes can finish given current Available resources and Need.
 */
ModuleStatus avoidance_safety(const SystemState *state, SafetyResult *result) {
    if (!state || !result) {
        return MODULE_ERROR;
    }

    int work[MAX_RESOURCES];
    bool finish[MAX_PROCESSES];
    int count = 0;

    // Step 1: Initialize Work = Available, and Finish array
    for (int r = 0; r < state->resource_count; ++r) {
        work[r] = state->available[r];
    }

    for (int p = 0; p < state->process_count; ++p) {
        finish[p] = (state->status[p] != PROCESS_ACTIVE);
    }

    // Step 2 & 3: Find an active process whose Need <= Work
    bool found;
    do {
        found = false;
        for (int p = 0; p < state->process_count; ++p) {
            if (!finish[p] && state->status[p] == PROCESS_ACTIVE) {
                bool can_finish = true;
                for (int r = 0; r < state->resource_count; ++r) {
                    if (state->need[p][r] > work[r]) {
                        can_finish = false;
                        break;
                    }
                }

                if (can_finish) {
                    for (int r = 0; r < state->resource_count; ++r) {
                        work[r] += state->allocation[p][r];
                    }
                    finish[p] = true;
                    result->sequence[count++] = p;
                    found = true;
                }
            }
        }
    } while (found);

    // Step 4: Check if all active processes were able to finish
    result->sequence_length = count;
    result->safe = true;
    for (int p = 0; p < state->process_count; ++p) {
        if (state->status[p] == PROCESS_ACTIVE && !finish[p]) {
            result->safe = false;
            break;
        }
    }

    return MODULE_OK;
}

/*
 * Banker's Algorithm Request Evaluation
 * Evaluates whether granting a request keeps the system in a safe state.
 */
ModuleStatus avoidance_request(SystemState *state, int pid, const int request[],
                               RequestDecision *decision) {
    if (!state || !request || pid < 0 || pid >= state->process_count || !decision) {
        return MODULE_ERROR;
    }

    if (state->status[pid] != PROCESS_ACTIVE) {
        *decision = REQUEST_INVALID;
        return MODULE_OK;
    }

    // Step 1: Check if request exceeds remaining Need or currently Available resources
    for (int r = 0; r < state->resource_count; ++r) {
        if (request[r] > state->need[pid][r] || request[r] > state->available[r]) {
            *decision = REQUEST_INVALID;
            return MODULE_OK;
        }
    }

    // Step 2: Tentatively allocate resources on a copy of the system state
    SystemState temp_state = *state;
    for (int r = 0; r < state->resource_count; ++r) {
        temp_state.available[r] -= request[r];
        temp_state.allocation[pid][r] += request[r];
        temp_state.need[pid][r] -= request[r];
    }

    // Step 3: Run the safety check on this tentative state
    SafetyResult safety_res = {0};
    ModuleStatus status = avoidance_safety(&temp_state, &safety_res);
    if (status != MODULE_OK) {
        return status;
    }

    // Step 4 & 5: Commit changes if safe, otherwise reject as unsafe
    if (safety_res.safe) {
        *state = temp_state;
        *decision = REQUEST_GRANTED;
    } else {
        *decision = REQUEST_UNSAFE;
    }

    return MODULE_OK;
}
