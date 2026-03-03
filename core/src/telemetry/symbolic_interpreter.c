#include "polycall/telemetry/symbolic.h"

#include <openssl/sha.h>

#include <stdio.h>
#include <string.h>

static int filter_route_normalize(const polycall_state_node_t *in,
                                  polycall_filter_result_t *out) {
    const char *q;
    size_t base_len;

    if (!in || !out) {
        return -1;
    }

    out->source_state = (polycall_state_node_t *)in;
    q = strchr(in->route, '?');
    base_len = q ? (size_t)(q - in->route) : strlen(in->route);
    if (base_len >= sizeof(out->sparse_repr)) {
        base_len = sizeof(out->sparse_repr) - 1;
    }

    memcpy(out->sparse_repr, in->route, base_len);
    out->sparse_repr[base_len] = '\0';

    return 0;
}

static int filter_state_classify(const polycall_state_node_t *in,
                                 polycall_filter_result_t *out) {
    static const struct {
        polycall_state_type_t type;
        const char *symbol;
    } map[] = {
        {STATE_REQUEST_RECEIVED, "REQ"},
        {STATE_AUTH_CHECK, "AUTH"},
        {STATE_DATA_PROCESSED, "DATA"},
        {STATE_RESPONSE_SENT, "RESP"},
        {STATE_ERROR, "ERR"},
        {STATE_ROUTE_TRANSITION, "ROUTE"},
        {STATE_BINDING_INVOKED, "BIND"},
        {STATE_EDGE_CACHE_HIT, "CAHIT"},
        {STATE_EDGE_CACHE_MISS, "CAMIS"},
        {0, NULL},
    };

    if (!in || !out) {
        return -1;
    }

    for (size_t i = 0; map[i].symbol; i++) {
        if (map[i].type == in->type) {
            strncpy(out->symbol.name, map[i].symbol, POLYCALL_SYMBOL_MAX_LEN - 1);
            out->symbol.code = (uint32_t)in->type;
            break;
        }
    }

    out->is_terminal = (in->type == STATE_ERROR || in->next == NULL);
    return 0;
}

static int flash_bidirectional(const polycall_filter_result_t *in,
                               polycall_symbol_t *out_forward,
                               polycall_symbol_t *out_inverse) {
    if (!in || !out_forward) {
        return -1;
    }

    snprintf(out_forward->name, POLYCALL_SYMBOL_MAX_LEN, "%s::%s", in->sparse_repr, in->symbol.name);
    out_forward->code = in->symbol.code;
    out_forward->is_bidirectional = 1;
    SHA256((const unsigned char *)out_forward->name, strlen(out_forward->name), out_forward->hash);

    if (out_inverse) {
        snprintf(out_inverse->name, POLYCALL_SYMBOL_MAX_LEN, "INV[%s::%s]", in->symbol.name, in->sparse_repr);
        out_inverse->code = ~in->symbol.code;
        out_inverse->is_bidirectional = 1;
        SHA256((const unsigned char *)out_inverse->name, strlen(out_inverse->name), out_inverse->hash);
    }

    return 0;
}

int polycall_symbolic_interpret_trace(const polycall_state_trace_t *trace,
                                      polycall_symbolic_pipeline_t *pipeline,
                                      polycall_symbol_t *symbols_out,
                                      uint32_t *symbols_count_out,
                                      uint32_t max_symbols) {
    polycall_state_node_t *cursor;
    uint32_t count = 0;

    if (!trace || !pipeline || !symbols_out || !symbols_count_out) {
        return -1;
    }

    cursor = trace->head;
    while (cursor && count < max_symbols) {
        polycall_filter_result_t fres;
        memset(&fres, 0, sizeof(fres));

        for (uint32_t f = 0; f < pipeline->filter_count; f++) {
            if (!pipeline->filters[f] || pipeline->filters[f](cursor, &fres) != 0) {
                return -2;
            }
        }

        if (!pipeline->flash) {
            return -3;
        }
        if (pipeline->bidirectional) {
            polycall_symbol_t inverse_symbol;
            if (pipeline->flash(&fres, &symbols_out[count], &inverse_symbol) != 0) {
                return -3;
            }
        } else if (pipeline->flash(&fres, &symbols_out[count], NULL) != 0) {
            return -3;
        }

        count++;
        cursor = cursor->next;
    }

    *symbols_count_out = count;
    return 0;
}

int polycall_telemetry_build_default_pipeline(polycall_symbolic_pipeline_t *pipeline) {
    if (!pipeline) {
        return -1;
    }

    memset(pipeline, 0, sizeof(*pipeline));
    pipeline->filters[0] = filter_route_normalize;
    pipeline->filters[1] = filter_state_classify;
    pipeline->filter_count = 2;
    pipeline->flash = flash_bidirectional;
    pipeline->bidirectional = 1;

    return 0;
}
