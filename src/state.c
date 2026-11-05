#include "state.h"
#include <stdio.h>

static bool fail(char *error, size_t size, const char *message) {
    if (error && size) snprintf(error, size, "%s", message);
    return false;
}

const char *state_status_name(ProcessStatus status) {
    switch (status) {
        case PROCESS_ACTIVE: return "active";
        case PROCESS_COMPLETED: return "completed";
        case PROCESS_TERMINATED: return "terminated";
    }
    return "invalid";
}

/* Compute in a copy, so even a late validation failure preserves the caller. */
bool state_initialize(SystemState *state, char *error, size_t capacity) {
    if (!state) return fail(error, capacity, "Missing system state.");
    SystemState next = *state;
    if (next.process_count < 1 || next.process_count > MAX_PROCESSES ||
        next.resource_count < 1 || next.resource_count > MAX_RESOURCES)
        return fail(error, capacity, "Process/resource count is out of range.");
    for (int j = 0; j < next.resource_count; ++j) {
        if (next.total[j] < 0 || next.total[j] > MAX_INSTANCES)
            return fail(error, capacity, "Total instances are out of range.");
        long long allocated = 0;
        for (int i = 0; i < next.process_count; ++i) {
            if (next.status[i] < PROCESS_ACTIVE || next.status[i] > PROCESS_TERMINATED ||
                next.work_done[i] < 0)
                return fail(error, capacity, "Invalid process status or work units.");
            int maximum = next.max[i][j], allocation = next.allocation[i][j];
            int request = next.request[i][j];
            if (maximum < 0 || maximum > next.total[j] || allocation < 0 ||
                allocation > maximum || request < 0 || request > maximum - allocation)
                return fail(error, capacity, "Require 0 <= Allocation <= Max <= Total and 0 <= Request <= Need.");
            if (next.status[i] != PROCESS_ACTIVE && (allocation != 0 || request != 0))
                return fail(error, capacity, "Inactive processes must hold/request zero resources.");
            next.need[i][j] = next.status[i] == PROCESS_ACTIVE ? maximum - allocation : 0;
            allocated += allocation;
        }
        if (allocated > next.total[j])
            return fail(error, capacity, "Combined allocations exceed total resource instances.");
        next.available[j] = next.total[j] - (int)allocated;
    }
    *state = next;
    if (error && capacity) error[0] = '\0';
    return true;
}

bool state_validate(const SystemState *state, char *error, size_t capacity) {
    if (!state) return fail(error, capacity, "Missing system state.");
    SystemState expected = *state;
    if (!state_initialize(&expected, error, capacity)) return false;
    for (int j = 0; j < state->resource_count; ++j) {
        if (expected.available[j] != state->available[j])
            return fail(error, capacity, "Available does not match Total minus allocations.");
        for (int i = 0; i < state->process_count; ++i)
            if (expected.need[i][j] != state->need[i][j])
                return fail(error, capacity, "Need is inconsistent with allocation/status.");
    }
    return true;
}

bool state_release(SystemState *state, int pid, const int release[],
                   char *error, size_t capacity) {
    if (!state_validate(state, error, capacity)) return false;
    if (!release || pid < 0 || pid >= state->process_count || state->status[pid] != PROCESS_ACTIVE)
        return fail(error, capacity, "Choose an active process ID.");
    /* Validate the ENTIRE vector before mutating any resource. */
    for (int j = 0; j < state->resource_count; ++j)
        if (release[j] < 0 || release[j] > state->allocation[pid][j])
            return fail(error, capacity, "Release must be nonnegative and cannot exceed allocation.");
    for (int j = 0; j < state->resource_count; ++j) {
        state->allocation[pid][j] -= release[j];
        state->available[j] += release[j];
        state->need[pid][j] = state->max[pid][j] - state->allocation[pid][j];
    }
    /* A release does not automatically grant anyone's pending request. */
    return true;
}

bool state_complete(SystemState *state, int pid, char *error, size_t capacity) {
    if (!state_validate(state, error, capacity)) return false;
    if (pid < 0 || pid >= state->process_count || state->status[pid] != PROCESS_ACTIVE)
        return fail(error, capacity, "Choose an active process ID.");
    for (int j = 0; j < state->resource_count; ++j)
        if (state->request[pid][j] != 0)
            return fail(error, capacity, "A process with a pending request cannot complete.");
    /* Explicit user-reported completion, not an automatic safety inference.
     * Max is an upper bound: completion does not require Need == 0. */
    for (int j = 0; j < state->resource_count; ++j) {
        state->available[j] += state->allocation[pid][j];
        state->allocation[pid][j] = state->need[pid][j] = state->request[pid][j] = 0;
    }
    state->status[pid] = PROCESS_COMPLETED;
    return true;
}

static void vector(const char *name, const int data[], int count) {
    printf("%-12s", name);
    for (int j = 0; j < count; ++j) printf(" %8d", data[j]);
    putchar('\n');
}
static void matrix(const char *name, const int data[][MAX_RESOURCES], int n, int m) {
    printf("\n%s\n           ", name);
    for (int j = 0; j < m; ++j) printf(" R%-7d", j);
    putchar('\n');
    for (int i = 0; i < n; ++i) {
        printf("P%-10d", i);
        for (int j = 0; j < m; ++j) printf(" %8d", data[i][j]);
        putchar('\n');
    }
}
void state_display(const SystemState *state) {
    printf("\nProcesses: %d | Resource types: %d\n            ", state->process_count, state->resource_count);
    for (int j = 0; j < state->resource_count; ++j) printf(" R%-7d", j);
    putchar('\n');
    vector("Total", state->total, state->resource_count);
    vector("Available", state->available, state->resource_count);
    matrix("Max", state->max, state->process_count, state->resource_count);
    matrix("Allocation", state->allocation, state->process_count, state->resource_count);
    matrix("Need", state->need, state->process_count, state->resource_count);
    matrix("Request (currently outstanding)", state->request, state->process_count, state->resource_count);
    puts("\nProcess      Status       Work done");
    for (int i = 0; i < state->process_count; ++i)
        printf("P%-11d %-12s %d\n", i, state_status_name(state->status[i]), state->work_done[i]);
}
