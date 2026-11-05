#include "input.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void parse_checks(void) {
    const char *bad[] = {"1x 2\n", "1 2 3\n", "-1 2\n", "9999999999999999999999 2\n", "1\n", "\n"};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        FILE *f = tmpfile(); assert(f);
        fputs(bad[i], f); rewind(f);
        int values[] = {7, 8};
        assert(!input_vector(f, values, 2, 0, INT_MAX));
        assert(values[0] == 7 && values[1] == 8);
        fclose(f);
    }
    FILE *f = tmpfile(); assert(f);
    fputs("1 2\n", f); rewind(f);
    int values[2]; assert(input_vector(f, values, 2, 0, INT_MAX));
    assert(values[0] == 1 && values[1] == 2); fclose(f);
}
int main(void) {
    SystemState s = {0}; char error[256];
    assert(scenario_load("scenarios/safe.txt", &s, error, sizeof error));
    assert(s.available[0] == 3 && s.available[1] == 3 && s.available[2] == 2);
    assert(s.need[0][0] == 7 && s.need[0][1] == 4 && s.need[0][2] == 3);
    assert(state_validate(&s, error, sizeof error));
    SystemState before = s;
    int invalid[] = {1, 0, 999}; /* First entry valid: ensure no partial update. */
    assert(!state_release(&s, 1, invalid, error, sizeof error));
    assert(memcmp(&before, &s, sizeof s) == 0);
    assert(!state_release(&s, -1, invalid, error, sizeof error));
    assert(!scenario_load("scenarios/invalid.txt", &s, error, sizeof error));
    assert(memcmp(&before, &s, sizeof s) == 0);
    assert(!scenario_load("scenarios/missing.txt", &s, error, sizeof error));
    assert(memcmp(&before, &s, sizeof s) == 0);
    int release[] = {1, 0, 0};
    assert(state_release(&s, 1, release, error, sizeof error));
    assert(s.available[0] == 4 && s.allocation[1][0] == 1 && s.need[1][0] == 2);
    assert(state_validate(&s, error, sizeof error));
    assert(state_complete(&s, 1, error, sizeof error));
    assert(s.status[1] == PROCESS_COMPLETED && s.available[0] == 5 && s.need[1][0] == 0);
    assert(state_validate(&s, error, sizeof error));
    before = s;
    assert(!state_release(&s, 1, release, error, sizeof error));
    assert(!state_complete(&s, 1, error, sizeof error));
    assert(memcmp(&before, &s, sizeof s) == 0);
    assert(scenario_load("scenarios/unsafe.txt", &s, error, sizeof error));
    assert(s.available[0] == 1); /* Unsafe states must remain valid inputs. */
    assert(scenario_load("scenarios/deadlocked.txt", &s, error, sizeof error));
    assert(s.available[0] == 0 && s.available[1] == 0);
    before = s;
    assert(!state_complete(&s, 0, error, sizeof error));
    assert(memcmp(&before, &s, sizeof s) == 0);
    s.request[0][0] = 2;
    before = s;
    assert(!state_initialize(&s, error, sizeof error));
    assert(memcmp(&before, &s, sizeof s) == 0);
    s = before; s.request[0][0] = 0; s.available[0] = 1;
    assert(!state_validate(&s, error, sizeof error));
    parse_checks();
    puts("State, validation, release, completion, and parser checks passed.");
    return 0;
}
