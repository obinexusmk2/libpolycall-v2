#ifndef POLYCALL_TELEMETRY_REPLAY_H
#define POLYCALL_TELEMETRY_REPLAY_H

#include "polycall/telemetry/guid.h"

typedef enum polycall_replay_direction {
    REPLAY_FORWARD = 0,
    REPLAY_INVERSE = 1,
} polycall_replay_direction_t;

typedef struct polycall_replay_result {
    polycall_state_node_t *failure_state;
    uint32_t states_replayed;
    char failure_guid[POLYCALL_GUID_STRING_LEN];
    char summary[1024];
    int reproduced;
} polycall_replay_result_t;

int polycall_replay_trace(const polycall_state_trace_t *trace,
                          polycall_replay_direction_t direction,
                          polycall_replay_result_t *result);
int polycall_replay_create_ticket(const polycall_replay_result_t *result,
                                  const char *uid,
                                  const char *ticket_endpoint,
                                  char *ticket_id_out,
                                  size_t ticket_id_len);

#endif
