#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_PROCESSES 20
#define MAX_RESOURCES 10
#define MAX_INSTANCES 1000000

typedef enum { PROCESS_ACTIVE, PROCESS_COMPLETED, PROCESS_TERMINATED } ProcessStatus;

/* All modules use this structure; PIDs and resource indices start at zero.
 * Request is the CURRENT outstanding request, not the remaining maximum Need.
 * No real operating-system processes or threads are created. */
typedef struct {
    int process_count;
    int resource_count;
    int total[MAX_RESOURCES];
    int available[MAX_RESOURCES];
    int max[MAX_PROCESSES][MAX_RESOURCES];
    int allocation[MAX_PROCESSES][MAX_RESOURCES];
    int need[MAX_PROCESSES][MAX_RESOURCES];
    int request[MAX_PROCESSES][MAX_RESOURCES];
    ProcessStatus status[MAX_PROCESSES];
    int work_done[MAX_PROCESSES];
} SystemState;

/* Derives Available and Need. Commits only if all input is valid.
 * Used after reading a scenario; does NOT run safety or detection. */
bool state_initialize(SystemState *state, char *error, size_t capacity);
/* Checks the stored Available/Need as well as all other invariants. */
bool state_validate(const SystemState *state, char *error, size_t capacity);
bool state_release(SystemState *state, int pid, const int release[],
                   char *error, size_t capacity);
bool state_complete(SystemState *state, int pid, char *error, size_t capacity);
void state_display(const SystemState *state);
const char *state_status_name(ProcessStatus status);

#endif
