#ifndef INPUT_H
#define INPUT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "state.h"
/* Reads exactly count integers from one line. Rejects overflow and extra input. */
bool input_vector(FILE *stream, int output[], int count, int minimum, int maximum);
bool scenario_keyboard(SystemState *state);
/* On any error, the caller's previous state is preserved. */
bool scenario_load(const char *path, SystemState *state, char *error, size_t capacity);
#endif
