#include "input.h"
#include "modules.h"
#include <string.h>

static void module_message(ModuleStatus status) {
    if (status == MODULE_NOT_IMPLEMENTED) puts("This teammate's module is not implemented yet. State unchanged.");
    else if (status == MODULE_ERROR) puts("Module reported an error.");
}
static void print_log(const RecoveryLog *log, int resources) {
    if (!log->count) puts("No recovery actions recorded.");
    for (int k = 0; k < log->count; ++k) {
        const RecoveryAction *action = &log->actions[k];
        printf("Terminated P%d; work lost: %d; resources returned:", action->victim_pid, action->work_lost);
        for (int j = 0; j < resources; ++j) printf(" %d", action->released[j]);
        putchar('\n');
    }
}
static bool operation_input(const SystemState *state, int *pid, int values[]) {
    printf("Process ID (0-%d): ", state->process_count - 1);
    if (!input_vector(stdin, pid, 1, 0, state->process_count - 1) || state->status[*pid] != PROCESS_ACTIVE) {
        puts("Invalid or inactive process."); return false;
    }
    printf("Enter %d resource values (R0, R1, ...): ", state->resource_count);
    if (!input_vector(stdin, values, state->resource_count, 0, MAX_INSTANCES)) {
        puts("Invalid vector."); return false;
    }
    return true;
}
int main(void) {
    SystemState state = {0};
    RecoveryLog log = {0};
    bool loaded = false;
    char error[256];
    puts("ICS433 - Deadlock Detection, Avoidance, and Recovery Manager");
    for (;;) {
        puts("\n1. Enter scenario\n2. Load scenario file\n3. Display state\n4. Request resources (avoidance)\n5. Release resources\n6. Mark process completed\n7. Check safety\n8. Detect deadlock\n9. Recover from deadlock\n10. Show recovery log\n0. Exit");
        printf("Choice: "); fflush(stdout);
        int choice;
        if (!input_vector(stdin, &choice, 1, 0, 10)) {
            if (feof(stdin)) break;
            puts("Enter one menu number from 0 to 10."); continue;
        }
        if (choice == 0) break;
        if (choice == 1 || choice == 2) {
            bool success = false;
            if (choice == 1) success = scenario_keyboard(&state);
            else {
                char path[512];
                printf("File path (example: scenarios/safe.txt): ");
                if (!fgets(path, sizeof path, stdin)) break;
                if (!strchr(path, '\n') && !feof(stdin)) {
                    int ch; while ((ch = getchar()) != '\n' && ch != EOF) { }
                    puts("Path too long."); continue;
                }
                path[strcspn(path, "\r\n")] = '\0';
                success = scenario_load(path, &state, error, sizeof error);
                if (!success) printf("Scenario rejected: %s Previous state preserved.\n", error);
            }
            if (success) {
                loaded = true; log = (RecoveryLog){0};
                puts("Scenario loaded. Available and Need calculated. Recovery log reset.");
                state_display(&state);
            }
            continue;
        }
        if (!loaded) { puts("Enter or load a valid scenario first."); continue; }
        int pid, values[MAX_RESOURCES];
        ModuleStatus status;
        switch (choice) {
            case 3: state_display(&state); break;
            case 4: {
                if (!operation_input(&state, &pid, values)) break;
                bool valid = true;
                for (int j = 0; j < state.resource_count; ++j)
                    if (values[j] > state.need[pid][j]) valid = false;
                if (!valid) { puts("Request exceeds remaining Need. State unchanged."); break; }
                RequestDecision decision = REQUEST_INVALID;
                status = avoidance_request(&state, pid, values, &decision);
                if (status == MODULE_OK) {
                    const char *messages[] = {"Request granted.", "Invalid request.",
                        "Request deferred: insufficient available resources.", "Request deferred: tentative allocation is unsafe."};
                    if (decision >= REQUEST_GRANTED && decision <= REQUEST_UNSAFE) puts(messages[decision]);
                    state_display(&state);
                } else module_message(status);
                break;
            }
            case 5:
                if (!operation_input(&state, &pid, values)) break;
                if (state_release(&state, pid, values, error, sizeof error)) {
                    puts("Resources released."); state_display(&state);
                } else puts(error);
                break;
            case 6:
                printf("Process ID: ");
                if (!input_vector(stdin, &pid, 1, 0, state.process_count - 1)) { puts("Invalid ID."); break; }
                if (state_complete(&state, pid, error, sizeof error)) {
                    puts("Process completed; all its resources returned."); state_display(&state);
                } else puts(error);
                break;
            case 7: {
                SafetyResult result = {0};
                status = avoidance_safety(&state, &result);
                if (status != MODULE_OK) { module_message(status); break; }
                if (!result.safe) puts("Unsafe: no guaranteed safe sequence. Run detection to check for deadlock.");
                else {
                    printf("Safe sequence:");
                    for (int i = 0; i < result.sequence_length; ++i) printf(" P%d", result.sequence[i]);
                    putchar('\n');
                }
                break;
            }
            case 8: {
                DetectionResult result = {0};
                status = detection_run(&state, &result);
                if (status != MODULE_OK) { module_message(status); break; }
                if (!result.count) puts("No confirmed deadlock under the current-request completion assumption. This does not prove safety.");
                else {
                    printf("Deadlocked processes:");
                    for (int i = 0; i < result.count; ++i) printf(" P%d", result.pids[i]);
                    putchar('\n');
                }
                break;
            }
            case 9:
                status = recovery_run(&state, &log);
                if (status == MODULE_OK) { state_display(&state); print_log(&log, state.resource_count); }
                else module_message(status);
                break;
            case 10: print_log(&log, state.resource_count); break;
        }
    }
    puts("Goodbye.");
    return 0;
}
