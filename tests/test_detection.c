#include "input.h"
#include "modules.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check_scenario(const char *path, const int expected_pids[], int expected_count)
{
    SystemState state = {0};
    char error[256];
    assert(scenario_load(path, &state, error, sizeof(error)));

    SystemState before = state;
    DetectionResult result = {0};
    assert(detection_run(&state, &result) == MODULE_OK);
    assert(result.count == expected_count);
    for (int i = 0; i < expected_count; ++i)
        assert(result.pids[i] == expected_pids[i]);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
}

static void check_progress_releases_resources(void)
{
    SystemState state = {0};
    char error[256];
    assert(scenario_load("scenarios/deadlocked.txt", &state, error, sizeof(error)));

    /* P0 can finish without waiting; its allocation then lets P1 finish. */
    state.request[0][1] = 0;
    assert(state_validate(&state, error, sizeof(error)));

    SystemState before = state;
    DetectionResult result = {0};
    assert(detection_run(&state, &result) == MODULE_OK);
    assert(result.count == 0);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
}

static void check_waiting_process_with_no_allocation(void)
{
    SystemState state = {0};
    char error[256];
    state.process_count = 2;
    state.resource_count = 1;
    state.total[0] = 1;

    state.status[0] = PROCESS_ACTIVE;
    state.max[0][0] = 1;
    state.allocation[0][0] = 1;
    state.request[0][0] = 0;

    state.status[1] = PROCESS_ACTIVE;
    state.max[1][0] = 1;
    state.allocation[1][0] = 0;
    state.request[1][0] = 1;

    assert(state_initialize(&state, error, sizeof(error)));
    DetectionResult result = {0};
    assert(detection_run(&state, &result) == MODULE_OK);
    /* P1 is waiting, but it holds no resource and is not deadlocked. */
    assert(result.count == 0);
}

int main(void)
{
    const int deadlocked_pids[] = {0, 1};

    check_scenario("scenarios/safe.txt", NULL, 0);
    check_scenario("scenarios/unsafe.txt", NULL, 0);
    check_scenario("scenarios/deadlocked.txt", deadlocked_pids, 2);
    check_progress_releases_resources();
    check_waiting_process_with_no_allocation();

    puts("Deadlock detection scenario checks passed.");
    return 0;
}
