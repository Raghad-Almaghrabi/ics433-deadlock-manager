#include <stdio.h>
#include <assert.h>
#include "modules.h"

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

    printf("\nSUCCESS! ALL RECOVERY TESTS PASSED!\n");

    return 0;
}
