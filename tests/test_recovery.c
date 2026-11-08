#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "modules.h"

/* Two independent resource cycles exercise repeated recovery and capacity. */
static SystemState cycles(bool second_cycle_waiting)
{
    SystemState state = {0};
    state.process_count = 4;
    state.resource_count = 4;
    const int work[] = {8, 3, 2, 6};
    for (int p = 0; p < 4; ++p) {
        state.total[p] = 1;
        state.max[p][p] = 1;
        state.max[p][p ^ 1] = 1;
        state.allocation[p][p] = 1;
        state.request[p][p ^ 1] = p < 2 || second_cycle_waiting;
        state.work_done[p] = work[p];
    }
    assert(state_initialize(&state, NULL, 0));
    return state;
}

static void check_history_and_capacity(void)
{
    SystemState state = cycles(false);
    RecoveryLog log = {0};
    assert(recovery_run(&state, &log) == MODULE_OK);
    assert(log.count == 1 && log.actions[0].victim_pid == 1);
    RecoveryLog saved = log;
    SystemState saved_state = state;
    assert(recovery_run(&state, &log) == MODULE_OK);
    assert(memcmp(&log, &saved, sizeof log) == 0);
    assert(memcmp(&state, &saved_state, sizeof state) == 0);

    /* A later request pattern forms the other independent cycle. */
    state.request[2][3] = state.request[3][2] = 1;
    assert(state_validate(&state, NULL, 0));
    assert(recovery_run(&state, &log) == MODULE_OK);
    assert(log.count == 2 && log.actions[1].victim_pid == 2);
    assert(memcmp(&log.actions[0], &saved.actions[0], sizeof log.actions[0]) == 0);
    DetectionResult result = {0};
    assert(detection_run(&state, &result) == MODULE_OK && result.count == 0);

    state = cycles(true);
    log = (RecoveryLog){0};
    log.count = MAX_RECOVERY_ACTIONS - 1;
    saved = log; saved_state = state;
    /* Two victims are needed, but only one log slot remains: commit neither. */
    assert(recovery_run(&state, &log) == MODULE_ERROR);
    assert(memcmp(&state, &saved_state, sizeof state) == 0);
    assert(memcmp(&log, &saved, sizeof log) == 0);
    log.count = -1;
    assert(recovery_run(&state, &log) == MODULE_ERROR);
    log.count = MAX_RECOVERY_ACTIONS + 1;
    assert(recovery_run(&state, &log) == MODULE_ERROR);
    assert(recovery_run(NULL, &log) == MODULE_ERROR);
    assert(recovery_run(&state, NULL) == MODULE_ERROR);

    state = cycles(true); log = (RecoveryLog){0};
    assert(recovery_run(&state, &log) == MODULE_OK);
    assert(log.count == 2);
    assert(log.actions[0].victim_pid == 2 && log.actions[1].victim_pid == 1);
    assert(log.actions[0].work_lost + log.actions[1].work_lost == 5);

    state = cycles(false); log = (RecoveryLog){0};
    state.work_done[0] = state.work_done[1] = 3;
    assert(recovery_run(&state, &log) == MODULE_OK);
    assert(log.count == 1 && log.actions[0].victim_pid == 0);
}

int main(void)
{
    SystemState state = {0};
    RecoveryLog log = {0};
    DetectionResult result = {0};
    char error[256];

    // Create 2 processes and 2 resource types
    state.process_count = 2;
    state.resource_count = 2;

    // Each resource type has 1 instance
    state.total[0] = 1;
    state.total[1] = 1;

    // Process P0 holds R0 and requests R1
    state.max[0][0] = 1;
    state.max[0][1] = 1;

    state.allocation[0][0] = 1;
    state.allocation[0][1] = 0;

    state.request[0][0] = 0;
    state.request[0][1] = 1;

    state.status[0] = PROCESS_ACTIVE;
    state.work_done[0] = 8;

    // Process P1 holds R1 and requests R0
    state.max[1][0] = 1;
    state.max[1][1] = 1;

    state.allocation[1][0] = 0;
    state.allocation[1][1] = 1;

    state.request[1][0] = 1;
    state.request[1][1] = 0;

    state.status[1] = PROCESS_ACTIVE;
    state.work_done[1] = 3;

    // Initialize system state
    assert(state_initialize(&state, error, sizeof(error)));

    printf("\n===== BEFORE RECOVERY =====\n");
    state_display(&state);

    // Confirm the deadlock exists
    assert(detection_run(&state, &result) == MODULE_OK);
    assert(result.count == 2);

    printf("\nDeadlock detected: %d processes\n", result.count);

    // Run Deema's recovery algorithm
    assert(recovery_run(&state, &log) == MODULE_OK);

    printf("\n===== RECOVERY RESULTS =====\n");

    // Print recovery actions
    for (int i = 0; i < log.count; i++)
    {
        printf("Terminated process: P%d\n",
               log.actions[i].victim_pid);

        printf("Work lost: %d units\n",
               log.actions[i].work_lost);

        for (int j = 0; j < state.resource_count; j++)
        {
            printf("Released R%d: %d\n",
                   j, log.actions[i].released[j]);
        }
    }

    printf("\n===== AFTER RECOVERY =====\n");
    state_display(&state);

    // Verify the correct victim was selected
    assert(log.count == 1);
    assert(log.actions[0].victim_pid == 1);
    assert(log.actions[0].work_lost == 3);

    // Verify resources were released
    assert(log.actions[0].released[0] == 0);
    assert(log.actions[0].released[1] == 1);

    // Verify P1 was terminated
    assert(state.status[1] == PROCESS_TERMINATED);
    assert(state.allocation[1][1] == 0);
    assert(state.request[1][0] == 0);
    assert(state.need[1][0] == 0);

    // Verify the available resources
    assert(state.available[0] == 0);
    assert(state.available[1] == 1);

    // Verify the updated state is valid
    assert(state_validate(&state, error, sizeof(error)));

    // Verify the deadlock is resolved
    assert(detection_run(&state, &result) == MODULE_OK);
    assert(result.count == 0);

    check_history_and_capacity();

    printf("\nSUCCESS! ALL RECOVERY TESTS PASSED!\n");

    return 0;
}
