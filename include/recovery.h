#ifndef RECOVERY_H
#define RECOVERY_H

#include "state.h"

/* Terminate a selected simulated process and release its resources. */
bool recovery_terminate(SystemState *state, int pid,
                        char *error, size_t capacity);

#endif
