#include "telemetry_cmd.h"

#include "polycall/telemetry/guid.h"
#include "polycall/telemetry/replay.h"
#include "polycall/telemetry/symbolic.h"

#include <stdio.h>
#include <string.h>

typedef enum telemetry_cmd_mode {
    TELEM_MODE_OBSERVE,
    TELEM_MODE_REPLAY,
    TELEM_MODE_TICKET,
    TELEM_MODE_DUMP,
} telemetry_cmd_mode_t;

int polycall_cmd_telemetry(int argc, char **argv) {
#ifndef POLYCALL_BUILD_DEVELOPMENT
    (void)argc;
    (void)argv;
    fprintf(stderr, "error: telemetry replay/debug commands are disabled in production builds\n");
    return 1;
#else
    telemetry_cmd_mode_t mode = TELEM_MODE_DUMP;
    const char *target = NULL;
    const char *uid = NULL;
    const char *endpoint = NULL;
    int inverse = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--observe") == 0 && i + 1 < argc) {
            mode = TELEM_MODE_OBSERVE;
            target = argv[++i];
        } else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            mode = TELEM_MODE_REPLAY;
            target = argv[++i];
        } else if (strcmp(argv[i], "--inverse") == 0) {
            inverse = 1;
        } else if (strcmp(argv[i], "--ticket") == 0 && i + 1 < argc) {
            mode = TELEM_MODE_TICKET;
            target = argv[++i];
        } else if (strcmp(argv[i], "--dump") == 0 && i + 1 < argc) {
            mode = TELEM_MODE_DUMP;
            target = argv[++i];
        } else if (strcmp(argv[i], "--uid") == 0 && i + 1 < argc) {
            uid = argv[++i];
        } else if (strcmp(argv[i], "--endpoint") == 0 && i + 1 < argc) {
            endpoint = argv[++i];
        }
    }

    switch (mode) {
        case TELEM_MODE_OBSERVE:
            fprintf(stdout, "[polycall telemetry] observing: %s\n", target ? target : "(none)");
            break;
        case TELEM_MODE_REPLAY:
            fprintf(stdout, "[polycall telemetry] replaying trace: %s [%s]\n",
                    target ? target : "(none)", inverse ? "INVERSE" : "FORWARD");
            break;
        case TELEM_MODE_TICKET:
            if (!uid || !endpoint || !target) {
                fprintf(stderr, "error: --ticket requires trace guid + --uid + --endpoint\n");
                return 1;
            }
            fprintf(stdout, "[polycall telemetry] creating ticket for trace: %s\n", target);
            break;
        case TELEM_MODE_DUMP:
            fprintf(stdout, "[polycall telemetry] symbolic dump of trace: %s\n",
                    target ? target : "(none)");
            break;
    }

    return 0;
#endif
}
