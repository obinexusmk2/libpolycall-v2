#include "libpolycall/core/polycall_state_machine.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Compute simple checksum for integrity verification */
static uint32_t compute_state_checksum(const PolyCall_State* state) {
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)state->name;
    for (size_t i = 0; i < strlen(state->name); i++) {
        checksum = (checksum << 5) + checksum + data[i];
    }
    checksum ^= state->id;
    checksum ^= state->version;
    return checksum;
}

polycall_sm_status_t polycall_sm_create_with_integrity(
    polycall_context_t ctx,
    PolyCall_StateMachine** sm,
    PolyCall_StateIntegrityCheck integrity_check
) {
    if (!sm) return POLYCALL_SM_ERROR_INVALID_CONTEXT;

    PolyCall_StateMachine* machine = calloc(1, sizeof(PolyCall_StateMachine));
    if (!machine) return POLYCALL_SM_ERROR_INVALID_CONTEXT;

    machine->ctx = ctx;
    machine->is_initialized = true;
    machine->integrity_check = integrity_check;
    machine->current_state = 0;
    machine->num_states = 0;
    machine->num_transitions = 0;
    machine->machine_checksum = 0;
    machine->diagnostics.failed_transitions = 0;
    machine->diagnostics.integrity_violations = 0;
    machine->diagnostics.last_verification = (uint64_t)time(NULL);

    *sm = machine;
    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_add_state(
    PolyCall_StateMachine* sm,
    const char* name,
    PolyCall_StateAction on_enter,
    PolyCall_StateAction on_exit,
    bool is_final
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (!name) return POLYCALL_SM_ERROR_INVALID_STATE;
    if (sm->num_states >= POLYCALL_MAX_STATES) return POLYCALL_SM_ERROR_MAX_STATES_REACHED;

    PolyCall_State* state = &sm->states[sm->num_states];
    strncpy(state->name, name, POLYCALL_MAX_NAME_LENGTH - 1);
    state->name[POLYCALL_MAX_NAME_LENGTH - 1] = '\0';
    state->on_enter = on_enter;
    state->on_exit = on_exit;
    state->is_final = is_final;
    state->id = sm->num_states;
    state->version = 1;
    state->timestamp = (uint64_t)time(NULL);
    state->is_locked = false;
    state->checksum = compute_state_checksum(state);

    sm->num_states++;
    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_add_transition(
    PolyCall_StateMachine* sm,
    const char* name,
    unsigned int from_state,
    unsigned int to_state,
    PolyCall_StateAction action,
    bool (*guard_condition)(const PolyCall_State*, const PolyCall_State*)
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (!name) return POLYCALL_SM_ERROR_INVALID_TRANSITION;
    if (from_state >= sm->num_states || to_state >= sm->num_states)
        return POLYCALL_SM_ERROR_INVALID_STATE;
    if (sm->num_transitions >= POLYCALL_MAX_TRANSITIONS)
        return POLYCALL_SM_ERROR_MAX_TRANSITIONS_REACHED;

    PolyCall_Transition* trans = &sm->transitions[sm->num_transitions];
    strncpy(trans->name, name, POLYCALL_MAX_NAME_LENGTH - 1);
    trans->name[POLYCALL_MAX_NAME_LENGTH - 1] = '\0';
    trans->from_state = from_state;
    trans->to_state = to_state;
    trans->action = action;
    trans->is_valid = true;
    trans->guard_condition = guard_condition;
    trans->guard_checksum = 0;

    sm->num_transitions++;
    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_execute_transition(
    PolyCall_StateMachine* sm,
    const char* transition_name
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (!transition_name) return POLYCALL_SM_ERROR_INVALID_TRANSITION;

    for (unsigned int i = 0; i < sm->num_transitions; i++) {
        PolyCall_Transition* trans = &sm->transitions[i];
        if (strcmp(trans->name, transition_name) == 0 && trans->is_valid) {
            if (trans->from_state != sm->current_state) {
                sm->diagnostics.failed_transitions++;
                return POLYCALL_SM_ERROR_INVALID_STATE;
            }

            /* Check guard condition */
            if (trans->guard_condition) {
                if (!trans->guard_condition(
                        &sm->states[trans->from_state],
                        &sm->states[trans->to_state])) {
                    sm->diagnostics.failed_transitions++;
                    return POLYCALL_SM_ERROR_INTEGRITY_CHECK_FAILED;
                }
            }

            /* Check locked states */
            if (sm->states[trans->to_state].is_locked) {
                return POLYCALL_SM_ERROR_STATE_LOCKED;
            }

            /* Execute exit action */
            if (sm->states[sm->current_state].on_exit) {
                sm->states[sm->current_state].on_exit(sm->ctx);
            }

            /* Execute transition action */
            if (trans->action) {
                trans->action(sm->ctx);
            }

            /* Move to new state */
            sm->current_state = trans->to_state;
            sm->states[trans->to_state].version++;
            sm->states[trans->to_state].timestamp = (uint64_t)time(NULL);

            /* Execute enter action */
            if (sm->states[sm->current_state].on_enter) {
                sm->states[sm->current_state].on_enter(sm->ctx);
            }

            return POLYCALL_SM_SUCCESS;
        }
    }

    sm->diagnostics.failed_transitions++;
    return POLYCALL_SM_ERROR_INVALID_TRANSITION;
}

polycall_sm_status_t polycall_sm_verify_state_integrity(
    PolyCall_StateMachine* sm,
    unsigned int state_id
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (state_id >= sm->num_states) return POLYCALL_SM_ERROR_INVALID_STATE;

    sm->diagnostics.last_verification = (uint64_t)time(NULL);

    if (sm->integrity_check) {
        if (!sm->integrity_check(&sm->states[state_id])) {
            sm->diagnostics.integrity_violations++;
            return POLYCALL_SM_ERROR_INTEGRITY_CHECK_FAILED;
        }
    }

    uint32_t current = compute_state_checksum(&sm->states[state_id]);
    if (current != sm->states[state_id].checksum) {
        sm->diagnostics.integrity_violations++;
        return POLYCALL_SM_ERROR_INTEGRITY_CHECK_FAILED;
    }

    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_lock_state(
    PolyCall_StateMachine* sm,
    unsigned int state_id
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (state_id >= sm->num_states) return POLYCALL_SM_ERROR_INVALID_STATE;
    sm->states[state_id].is_locked = true;
    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_unlock_state(
    PolyCall_StateMachine* sm,
    unsigned int state_id
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (state_id >= sm->num_states) return POLYCALL_SM_ERROR_INVALID_STATE;
    sm->states[state_id].is_locked = false;
    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_get_state_version(
    const PolyCall_StateMachine* sm,
    unsigned int state_id,
    unsigned int* version
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (state_id >= sm->num_states) return POLYCALL_SM_ERROR_INVALID_STATE;
    if (!version) return POLYCALL_SM_ERROR_INVALID_STATE;
    *version = sm->states[state_id].version;
    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_create_state_snapshot(
    const PolyCall_StateMachine* sm,
    unsigned int state_id,
    PolyCall_StateSnapshot* snapshot
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (state_id >= sm->num_states) return POLYCALL_SM_ERROR_INVALID_STATE;
    if (!snapshot) return POLYCALL_SM_ERROR_INVALID_STATE;

    snapshot->state = sm->states[state_id];
    snapshot->timestamp = (uint64_t)time(NULL);
    snapshot->checksum = compute_state_checksum(&sm->states[state_id]);
    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_restore_state_from_snapshot(
    PolyCall_StateMachine* sm,
    const PolyCall_StateSnapshot* snapshot
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (!snapshot) return POLYCALL_SM_ERROR_INVALID_STATE;
    if (snapshot->state.id >= POLYCALL_MAX_STATES) return POLYCALL_SM_ERROR_INVALID_STATE;

    sm->states[snapshot->state.id] = snapshot->state;
    return POLYCALL_SM_SUCCESS;
}

polycall_sm_status_t polycall_sm_get_state_diagnostics(
    const PolyCall_StateMachine* sm,
    unsigned int state_id,
    PolyCall_StateDiagnostics* diagnostics
) {
    if (!sm || !sm->is_initialized) return POLYCALL_SM_ERROR_NOT_INITIALIZED;
    if (state_id >= sm->num_states) return POLYCALL_SM_ERROR_INVALID_STATE;
    if (!diagnostics) return POLYCALL_SM_ERROR_INVALID_STATE;

    const PolyCall_State* state = &sm->states[state_id];
    diagnostics->state_id = state->id;
    diagnostics->creation_time = state->timestamp;
    diagnostics->last_modified = state->timestamp;
    diagnostics->transition_count = 0;
    diagnostics->integrity_check_count = 0;
    diagnostics->is_locked = state->is_locked;
    diagnostics->current_checksum = state->checksum;
    return POLYCALL_SM_SUCCESS;
}

void polycall_sm_destroy(PolyCall_StateMachine* sm) {
    if (sm) {
        sm->is_initialized = false;
        free(sm);
    }
}
