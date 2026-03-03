#ifndef POLYCALL_TELEMETRY_GUID_H
#define POLYCALL_TELEMETRY_GUID_H

#include <stdint.h>
#include <stddef.h>

#define POLYCALL_GUID_SEED_LEN   32
#define POLYCALL_GUID_STRING_LEN 65
#define POLYCALL_MAX_STATE_DEPTH 4096

typedef struct polycall_guid {
    uint8_t  seed[POLYCALL_GUID_SEED_LEN];
    char     string[POLYCALL_GUID_STRING_LEN];
    uint64_t timestamp_ns;
    uint32_t sequence_id;
} polycall_guid_t;

typedef enum polycall_state_type {
    STATE_REQUEST_RECEIVED  = 0x01,
    STATE_AUTH_CHECK        = 0x02,
    STATE_DATA_PROCESSED    = 0x03,
    STATE_RESPONSE_SENT     = 0x04,
    STATE_ERROR             = 0x05,
    STATE_ROUTE_TRANSITION  = 0x06,
    STATE_BINDING_INVOKED   = 0x07,
    STATE_EDGE_CACHE_HIT    = 0x08,
    STATE_EDGE_CACHE_MISS   = 0x09,
} polycall_state_type_t;

typedef struct polycall_state_node {
    polycall_guid_t              guid;
    polycall_state_type_t        type;
    char                         route[256];
    char                         binding[128];
    uint8_t                      action_hash[32];
    struct polycall_state_node  *prev;
    struct polycall_state_node  *next;
    uint64_t                     timestamp_ns;
} polycall_state_node_t;

typedef struct polycall_state_trace {
    polycall_state_node_t   *head;
    polycall_state_node_t   *tail;
    uint32_t                 depth;
    polycall_guid_t          trace_guid;
    char                     uid[64];
} polycall_state_trace_t;

int polycall_guid_generate(polycall_guid_t *guid, const char *route, uint32_t sequence_id);
int polycall_trace_push_state(polycall_state_trace_t *trace,
                              polycall_state_type_t type,
                              const char *route,
                              const char *binding,
                              const uint8_t *action_data,
                              size_t action_data_len);
int polycall_trace_pop_state(polycall_state_trace_t *trace, polycall_state_node_t *out_copy);
void polycall_trace_free(polycall_state_trace_t *trace);

#endif
