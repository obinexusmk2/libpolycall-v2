#include "../../src/cli/commands/telemetry_cmd.h"

#include <stdio.h>

static int expect_zero(const char *name, int argc, char **argv) {
    int rc = polycall_cmd_telemetry(argc, argv);
    if (rc != 0) {
        fprintf(stderr, "%s failed: expected rc=0, got %d\n", name, rc);
        return 1;
    }
    return 0;
}

int main(void) {
    char *observe[] = {"telemetry", "--observe", "service://daemon"};
    char *replay[] = {"telemetry", "--replay", "trace-guid", "--inverse"};
    char *dump[] = {"telemetry", "--dump", "trace-guid"};
    char *ticket_ok[] = {
        "telemetry", "--ticket", "trace-guid", "--uid", "user-123", "--endpoint", "/v1/telemetry"
    };
    char *ticket_bad[] = {"telemetry", "--ticket", "trace-guid"};

    if (expect_zero("observe", 3, observe) != 0) return 1;
    if (expect_zero("replay", 4, replay) != 0) return 2;
    if (expect_zero("dump", 3, dump) != 0) return 3;
    if (expect_zero("ticket_ok", 7, ticket_ok) != 0) return 4;

    if (polycall_cmd_telemetry(3, ticket_bad) != 1) {
        fprintf(stderr, "ticket_bad failed: expected rc=1\n");
        return 5;
    }

    puts("telemetry integration test passed");
    return 0;
}
