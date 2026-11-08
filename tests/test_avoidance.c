#include "input.h"
#include "modules.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static SystemState load(const char *path)
{
    SystemState s = {0};
    char error[256];
    assert(scenario_load(path, &s, error, sizeof error));
    return s;
}

/* Verify any returned sequence by replaying it, without depending on one order. */
static void verify_sequence(const SystemState *s, const SafetyResult *result)
{
    bool seen[MAX_PROCESSES] = {false};
    int work[MAX_RESOURCES] = {0};
    int active = 0;
    assert(result->safe);
    for (int r = 0; r < s->resource_count; ++r)
        work[r] = s->available[r];
    for (int p = 0; p < s->process_count; ++p)
        active += s->status[p] == PROCESS_ACTIVE;
    assert(result->sequence_length == active);
    for (int k = 0; k < result->sequence_length; ++k) {
        int pid = result->sequence[k];
        assert(pid >= 0 && pid < s->process_count);
        assert(s->status[pid] == PROCESS_ACTIVE && !seen[pid]);
        for (int r = 0; r < s->resource_count; ++r) {
            assert(s->need[pid][r] <= work[r]);
            work[r] += s->allocation[pid][r];
        }
        seen[pid] = true;
    }
}

static void check_safety(void)
{
    SystemState s = load("scenarios/safe.txt"), before = s;
    SafetyResult result = {0};
    assert(avoidance_safety(&s, &result) == MODULE_OK);
    verify_sequence(&s, &result);
    assert(memcmp(&s, &before, sizeof s) == 0);
    s = load("scenarios/unsafe.txt");
    assert(avoidance_safety(&s, &result) == MODULE_OK && !result.safe);
    assert(result.sequence_length == 0);
    DetectionResult detected = {0};
    assert(detection_run(&s, &detected) == MODULE_OK && detected.count == 0);
    s = load("scenarios/deadlocked.txt");
    assert(avoidance_safety(&s, &result) == MODULE_OK && !result.safe);
    assert(detection_run(&s, &detected) == MODULE_OK && detected.count == 2);
    s = load("scenarios/safe.txt");
    assert(state_complete(&s, 1, NULL, 0));
    assert(avoidance_safety(&s, &result) == MODULE_OK);
    verify_sequence(&s, &result);
    for (int p = 0; p < s.process_count; ++p)
        if (s.status[p] == PROCESS_ACTIVE)
            assert(state_complete(&s, p, NULL, 0));
    assert(avoidance_safety(&s, &result) == MODULE_OK);
    assert(result.safe && result.sequence_length == 0);
}

static void check_requests(void)
{
    SystemState s = load("scenarios/safe.txt");
    int grant[] = {1, 0, 2};
    /* Retrying the stored row tests that clearing it cannot alter input midway. */
    memcpy(s.request[1], grant, sizeof grant);
    RequestDecision decision = REQUEST_INVALID;
    assert(avoidance_request(&s, 1, s.request[1], &decision) == MODULE_OK);
    assert(decision == REQUEST_GRANTED);
    assert(s.available[0] == 2 && s.available[1] == 3 && s.available[2] == 0);
    assert(s.allocation[1][0] == 3 && s.allocation[1][2] == 2);
    assert(s.request[1][0] == 0 && s.request[1][2] == 0);
    assert(state_validate(&s, NULL, 0));

    s = load("scenarios/safe.txt");
    SystemState before = s, expected = s;
    int unsafe[] = {3, 3, 0};
    memcpy(expected.request[4], unsafe, sizeof unsafe);
    assert(avoidance_request(&s, 4, unsafe, &decision) == MODULE_OK);
    assert(decision == REQUEST_UNSAFE);
    assert(memcmp(&s, &expected, sizeof s) == 0);
    assert(state_validate(&s, NULL, 0));

    s = before;
    int waiting[] = {4, 0, 0};
    assert(avoidance_request(&s, 0, waiting, &decision) == MODULE_OK);
    assert(decision == REQUEST_UNAVAILABLE && s.request[0][0] == 4);
    /* Resubmission replaces, rather than adds to, a pending request. */
    int replacement[] = {5, 0, 0};
    assert(avoidance_request(&s, 0, replacement, &decision) == MODULE_OK);
    assert(decision == REQUEST_UNAVAILABLE && s.request[0][0] == 5);
    assert(s.available[0] == before.available[0]);
    assert(memcmp(s.allocation, before.allocation, sizeof s.allocation) == 0);
    assert(state_validate(&s, NULL, 0));

    before = s;
    /* Availability fails early, but a later negative component makes this invalid. */
    int negative[] = {4, -1, 0};
    assert(avoidance_request(&s, 0, negative, &decision) == MODULE_OK);
    assert(decision == REQUEST_INVALID && memcmp(&s, &before, sizeof s) == 0);
    int exceeds_need[] = {0, 0, 4};
    assert(avoidance_request(&s, 0, exceeds_need, &decision) == MODULE_OK);
    assert(decision == REQUEST_INVALID && memcmp(&s, &before, sizeof s) == 0);
    int zero[] = {0, 0, 0};
    assert(avoidance_request(&s, 0, zero, &decision) == MODULE_OK);
    assert(decision == REQUEST_GRANTED && s.request[0][0] == 0);
    assert(s.available[0] == before.available[0]);
    assert(state_complete(&s, 0, NULL, 0));
    before = s;
    assert(avoidance_request(&s, 0, zero, &decision) == MODULE_OK);
    assert(decision == REQUEST_INVALID && memcmp(&s, &before, sizeof s) == 0);
}

static void check_bad_arguments(void)
{
    SystemState s = load("scenarios/safe.txt"), before;
    SafetyResult result = {0}, old_result = result;
    RequestDecision decision = REQUEST_UNSAFE;
    int request[] = {1, 0, 0};
    assert(avoidance_safety(NULL, &result) == MODULE_ERROR);
    assert(avoidance_safety(&s, NULL) == MODULE_ERROR);
    assert(avoidance_request(NULL, 0, request, &decision) == MODULE_ERROR);
    assert(avoidance_request(&s, 0, NULL, &decision) == MODULE_ERROR);
    assert(avoidance_request(&s, 0, request, NULL) == MODULE_ERROR);
    assert(avoidance_request(&s, -1, request, &decision) == MODULE_ERROR);
    assert(avoidance_request(&s, s.process_count, request, &decision) == MODULE_ERROR);
    s.process_count = MAX_PROCESSES + 1;
    before = s;
    assert(avoidance_safety(&s, &result) == MODULE_ERROR);
    assert(memcmp(&result, &old_result, sizeof result) == 0);
    assert(avoidance_request(&s, 0, request, &decision) == MODULE_ERROR);
    assert(decision == REQUEST_UNSAFE && memcmp(&s, &before, sizeof s) == 0);
    s = load("scenarios/safe.txt");
    s.available[0]++;
    assert(avoidance_safety(&s, &result) == MODULE_ERROR);
}

int main(void)
{
    check_safety();
    check_requests();
    check_bad_arguments();
    puts("Banker safety, request, rollback, pending request, and validation checks passed.");
    return 0;
}
