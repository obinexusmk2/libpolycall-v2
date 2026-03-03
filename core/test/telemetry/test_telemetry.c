#include "polycall/telemetry/guid.h"
#include "polycall/telemetry/replay.h"
#include "polycall/telemetry/symbolic.h"

#include <stdio.h>
#include <string.h>

static int test_guid_uniqueness(void) {
    polycall_guid_t g1;
    polycall_guid_t g2;

    if (polycall_guid_generate(&g1, "/api/test", 1) != 0) return 1;
    if (polycall_guid_generate(&g2, "/api/test", 1) != 0) return 2;
    if (strcmp(g1.string, g2.string) == 0) return 3;
    if (strlen(g1.string) != 64 || strlen(g2.string) != 64) return 4;

    return 0;
}

static int test_replay_and_symbolic(void) {
    polycall_state_trace_t trace = {0};
    polycall_replay_result_t replay = {0};
    polycall_symbolic_pipeline_t pipeline = {0};
    polycall_symbol_t symbols[8];
    uint32_t symbol_count = 0;
    const uint8_t payload[] = "payload";

    if (polycall_trace_push_state(&trace, STATE_REQUEST_RECEIVED, "/v1/ping?x=1", "c", payload, sizeof(payload)-1) != 0) return 10;
    if (polycall_trace_push_state(&trace, STATE_ERROR, "/v1/ping", "c", payload, sizeof(payload)-1) != 0) return 11;

    if (polycall_replay_trace(&trace, REPLAY_FORWARD, &replay) != 1) return 12;
    if (!replay.reproduced || replay.failure_guid[0] == '\0') return 13;

    if (polycall_telemetry_build_default_pipeline(&pipeline) != 0) return 14;
    if (polycall_symbolic_interpret_trace(&trace, &pipeline, symbols, &symbol_count, 8) != 0) return 15;
    if (symbol_count != 2) return 16;
    if (strncmp(symbols[0].name, "/v1/ping::REQ", 12) != 0) return 17;

    polycall_trace_free(&trace);
    return 0;
}

int main(void) {
    int rc;

    rc = test_guid_uniqueness();
    if (rc != 0) {
        fprintf(stderr, "test_guid_uniqueness failed: %d\n", rc);
        return rc;
    }

    rc = test_replay_and_symbolic();
    if (rc != 0) {
        fprintf(stderr, "test_replay_and_symbolic failed: %d\n", rc);
        return rc;
    }

    puts("telemetry tests passed");
    return 0;
}
