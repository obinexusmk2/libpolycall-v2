#include "../../src/cli/commands/telemetry_cmd.h"

#include <stdio.h>

int main(void) {
    char *argv[] = {"telemetry", "--dump", "trace-001"};
    int rc = polycall_cmd_telemetry(3, argv);

    if (rc != 1) {
        fprintf(stderr, "expected production telemetry command to return 1, got %d\n", rc);
        return 1;
    }

    puts("telemetry unit test passed");
    return 0;
}
