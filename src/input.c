#include "input.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

bool input_vector(FILE *stream, int output[], int count, int minimum, int maximum) {
    char line[4096];
    if (!fgets(line, sizeof line, stream)) return false;
    if (!strchr(line, '\n') && !feof(stream)) {
        int ch;
        while ((ch = fgetc(stream)) != '\n' && ch != EOF) { }
        return false;
    }
    int temporary[MAX_PROCESSES];
    if (count < 1 || count > MAX_PROCESSES) return false;
    char *cursor = line;
    for (int i = 0; i < count; ++i) {
        while (isspace((unsigned char)*cursor)) ++cursor;
        char *end;
        errno = 0;
        long value = strtol(cursor, &end, 10);
        if (end == cursor || errno == ERANGE || value < minimum || value > maximum ||
            (*end && !isspace((unsigned char)*end))) return false;
        temporary[i] = (int)value;
        cursor = end;
    }
    while (isspace((unsigned char)*cursor)) ++cursor;
    if (*cursor) return false;
    memcpy(output, temporary, (size_t)count * sizeof *output);
    return true;
}

static bool ask(const char *prompt, int output[], int count, int min, int max) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (input_vector(stdin, output, count, min, max)) return true;
        if (feof(stdin)) return false;
        printf("Enter exactly %d integer(s), each in [%d, %d].\n", count, min, max);
    }
}

bool scenario_keyboard(SystemState *state) {
    SystemState next = {0};
    if (!ask("Number of processes (1-20): ", &next.process_count, 1, 1, MAX_PROCESSES) ||
        !ask("Number of resource types (1-10): ", &next.resource_count, 1, 1, MAX_RESOURCES) ||
        !ask("Total vector: ", next.total, next.resource_count, 0, MAX_INSTANCES)) return false;
    for (int i = 0; i < next.process_count; ++i) {
        printf("P%d (resource order R0, R1, ...):\n", i);
        if (!ask("  Max vector: ", next.max[i], next.resource_count, 0, MAX_INSTANCES) ||
            !ask("  Allocation vector: ", next.allocation[i], next.resource_count, 0, MAX_INSTANCES) ||
            !ask("  Outstanding Request vector: ", next.request[i], next.resource_count, 0, MAX_INSTANCES) ||
            !ask("  Completed work units: ", &next.work_done[i], 1, 0, INT_MAX)) return false;
        next.status[i] = PROCESS_ACTIVE;
    }
    char error[256];
    if (!state_initialize(&next, error, sizeof error)) {
        printf("Scenario rejected: %s Previous state preserved.\n", error);
        return false;
    }
    *state = next;
    return true;
}

/* File tokens are bounded before strtol: avoids scanf integer overflow.
 * Files use whitespace-separated integers, without comments. */
static bool token(FILE *file, int *value) {
    char buffer[64];
    int ch;
    do { ch = fgetc(file); } while (ch != EOF && isspace((unsigned char)ch));
    if (ch == EOF) return false;
    size_t length = 0;
    do {
        if (length + 1 >= sizeof buffer) return false;
        buffer[length++] = (char)ch;
        ch = fgetc(file);
    } while (ch != EOF && !isspace((unsigned char)ch));
    buffer[length] = '\0';
    char *end;
    errno = 0;
    long number = strtol(buffer, &end, 10);
    if (errno == ERANGE || end == buffer || *end || number < 0 || number > INT_MAX) return false;
    *value = (int)number;
    return true;
}

bool scenario_load(const char *path, SystemState *state, char *error, size_t capacity) {
    FILE *file = fopen(path, "r");
    if (!file) {
        snprintf(error, capacity, "Cannot open scenario file.");
        return false;
    }
    SystemState next = {0};
    bool ok = token(file, &next.process_count) && token(file, &next.resource_count);
    ok = ok && next.process_count >= 1 && next.process_count <= MAX_PROCESSES &&
         next.resource_count >= 1 && next.resource_count <= MAX_RESOURCES;
    for (int j = 0; ok && j < next.resource_count; ++j) ok = token(file, &next.total[j]);
    for (int i = 0; ok && i < next.process_count; ++i) {
        for (int j = 0; ok && j < next.resource_count; ++j) ok = token(file, &next.max[i][j]);
        for (int j = 0; ok && j < next.resource_count; ++j) ok = token(file, &next.allocation[i][j]);
        for (int j = 0; ok && j < next.resource_count; ++j) ok = token(file, &next.request[i][j]);
        if (ok) ok = token(file, &next.work_done[i]);
        next.status[i] = PROCESS_ACTIVE;
    }
    int ch;
    if (ok) {
        do { ch = fgetc(file); } while (ch != EOF && isspace((unsigned char)ch));
        ok = ch == EOF && !ferror(file);
    }
    fclose(file);
    if (!ok) {
        snprintf(error, capacity, "Malformed scenario: check counts, integers, missing or extra values.");
        return false;
    }
    if (!state_initialize(&next, error, capacity)) return false;
    *state = next;
    return true;
}
