#include "polycall/telemetry/replay.h"

#include <stdio.h>
#include <string.h>

int polycall_replay_trace(const polycall_state_trace_t *trace,
                          polycall_replay_direction_t direction,
                          polycall_replay_result_t *result) {
    polycall_state_node_t *cursor;
    uint32_t step = 0;
    char snapshot_buf[256];

    if (!trace || !result) {
        return -1;
    }

    memset(result, 0, sizeof(*result));
    cursor = (direction == REPLAY_FORWARD) ? trace->head : trace->tail;

    while (cursor) {
        snprintf(snapshot_buf, sizeof(snapshot_buf),
                 "[%u] type=%d route=%s binding=%s guid=%s ts=%llu\n",
                 step,
                 cursor->type,
                 cursor->route,
                 cursor->binding,
                 cursor->guid.string,
                 (unsigned long long)cursor->timestamp_ns);
        strncat(result->summary,
                snapshot_buf,
                sizeof(result->summary) - strlen(result->summary) - 1);

        if (cursor->type == STATE_ERROR) {
            result->failure_state = cursor;
            strncpy(result->failure_guid,
                    cursor->guid.string,
                    sizeof(result->failure_guid) - 1);
            result->reproduced = 1;
            break;
        }

        result->states_replayed = ++step;
        cursor = (direction == REPLAY_FORWARD) ? cursor->next : cursor->prev;
    }

    return result->reproduced ? 1 : 0;
}

int polycall_replay_create_ticket(const polycall_replay_result_t *result,
                                  const char *uid,
                                  const char *ticket_endpoint,
                                  char *ticket_id_out,
                                  size_t ticket_id_len) {
    if (!result || !uid || !ticket_endpoint || !ticket_id_out || ticket_id_len == 0) {
        return -1;
    }

    snprintf(ticket_id_out, ticket_id_len, "PCTT-%.16s-%.8s", result->failure_guid, uid);

    fprintf(stdout,
            "[POLYCALL TICKET] id=%s endpoint=%s states=%u reproduced=%d\n",
            ticket_id_out,
            ticket_endpoint,
            result->states_replayed,
            result->reproduced);

    return 0;
}
