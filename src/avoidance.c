#include "modules.h"
/* Owner: Dena. Replace these stubs with Banker's algorithms. */
ModuleStatus avoidance_safety(const SystemState *state, SafetyResult *result) {
    (void)state; (void)result;
    return MODULE_NOT_IMPLEMENTED;
}
ModuleStatus avoidance_request(SystemState *state, int pid, const int request[],
                               RequestDecision *decision) {
    (void)state; (void)pid; (void)request; (void)decision;
    return MODULE_NOT_IMPLEMENTED;
}
