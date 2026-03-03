#include "polycall/telemetry/guid.h"

#include <openssl/rand.h>
#include <openssl/sha.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static uint64_t polycall_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int polycall_guid_generate(polycall_guid_t *guid, const char *route, uint32_t sequence_id) {
    uint8_t random_bytes[16];
    uint8_t seed_material[256];
    size_t offset = 0;
    size_t route_len;
    uint64_t timestamp_ns;

    if (!guid || !route) {
        return -1;
    }

    if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
        return -2;
    }

    timestamp_ns = polycall_timestamp_ns();

    memcpy(seed_material + offset, random_bytes, sizeof(random_bytes));
    offset += sizeof(random_bytes);
    memcpy(seed_material + offset, &timestamp_ns, sizeof(timestamp_ns));
    offset += sizeof(timestamp_ns);
    memcpy(seed_material + offset, &sequence_id, sizeof(sequence_id));
    offset += sizeof(sequence_id);

    route_len = strlen(route);
    if (route_len > sizeof(seed_material) - offset) {
        route_len = sizeof(seed_material) - offset;
    }
    memcpy(seed_material + offset, route, route_len);
    offset += route_len;

    SHA256(seed_material, offset, guid->seed);

    for (size_t i = 0; i < POLYCALL_GUID_SEED_LEN; i++) {
        snprintf(guid->string + (i * 2U), 3, "%02x", guid->seed[i]);
    }
    guid->string[POLYCALL_GUID_STRING_LEN - 1] = '\0';
    guid->timestamp_ns = timestamp_ns;
    guid->sequence_id = sequence_id;

    return 0;
}

int polycall_trace_push_state(polycall_state_trace_t *trace,
                              polycall_state_type_t type,
                              const char *route,
                              const char *binding,
                              const uint8_t *action_data,
                              size_t action_data_len) {
    polycall_state_node_t *node;

    if (!trace || trace->depth >= POLYCALL_MAX_STATE_DEPTH) {
        return -1;
    }

    node = (polycall_state_node_t *)calloc(1, sizeof(*node));
    if (!node) {
        return -2;
    }

    if (polycall_guid_generate(&node->guid, route ? route : "unknown", trace->depth) != 0) {
        free(node);
        return -3;
    }

    node->type = type;
    strncpy(node->route, route ? route : "unknown", sizeof(node->route) - 1);
    strncpy(node->binding, binding ? binding : "none", sizeof(node->binding) - 1);

    if (action_data && action_data_len > 0) {
        SHA256(action_data, action_data_len, node->action_hash);
    }

    node->timestamp_ns = polycall_timestamp_ns();
    node->prev = trace->tail;

    if (trace->tail) {
        trace->tail->next = node;
    } else {
        trace->head = node;
    }

    trace->tail = node;
    trace->depth++;

    return 0;
}

int polycall_trace_pop_state(polycall_state_trace_t *trace, polycall_state_node_t *out_copy) {
    polycall_state_node_t *tail;

    if (!trace || !trace->tail) {
        return -1;
    }

    tail = trace->tail;
    trace->tail = tail->prev;
    if (trace->tail) {
        trace->tail->next = NULL;
    } else {
        trace->head = NULL;
    }

    if (trace->depth > 0) {
        trace->depth--;
    }

    if (out_copy) {
        *out_copy = *tail;
        out_copy->prev = NULL;
        out_copy->next = NULL;
    }

    free(tail);
    return 0;
}

void polycall_trace_free(polycall_state_trace_t *trace) {
    polycall_state_node_t *cursor;
    polycall_state_node_t *next;

    if (!trace) {
        return;
    }

    cursor = trace->head;
    while (cursor) {
        next = cursor->next;
        free(cursor);
        cursor = next;
    }

    trace->head = NULL;
    trace->tail = NULL;
    trace->depth = 0;
}
