#ifndef POLYCALL_TELEMETRY_SYMBOLIC_H
#define POLYCALL_TELEMETRY_SYMBOLIC_H

#include "polycall/telemetry/guid.h"

#define POLYCALL_SYMBOL_MAX_LEN   128
#define POLYCALL_FILTER_DEPTH     16

typedef struct polycall_symbol {
    char     name[POLYCALL_SYMBOL_MAX_LEN];
    uint32_t code;
    uint8_t  hash[32];
    int      is_bidirectional;
} polycall_symbol_t;

typedef struct polycall_filter_result {
    polycall_symbol_t      symbol;
    polycall_state_node_t *source_state;
    char                   sparse_repr[512];
    int                    is_terminal;
} polycall_filter_result_t;

typedef int (*polycall_filter_fn)(const polycall_state_node_t *in,
                                  polycall_filter_result_t *out);

typedef int (*polycall_flash_fn)(const polycall_filter_result_t *in,
                                 polycall_symbol_t *out_forward,
                                 polycall_symbol_t *out_inverse);

typedef struct polycall_symbolic_pipeline {
    polycall_filter_fn  filters[POLYCALL_FILTER_DEPTH];
    polycall_flash_fn   flash;
    uint32_t            filter_count;
    int                 bidirectional;
} polycall_symbolic_pipeline_t;

int polycall_symbolic_interpret_trace(const polycall_state_trace_t *trace,
                                      polycall_symbolic_pipeline_t *pipeline,
                                      polycall_symbol_t *symbols_out,
                                      uint32_t *symbols_count_out,
                                      uint32_t max_symbols);
int polycall_telemetry_build_default_pipeline(polycall_symbolic_pipeline_t *pipeline);

#endif
