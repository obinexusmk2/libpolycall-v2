/**
 * @file polycall_error.h
 * @brief Error management for LibPolyCall core
 *
 * Provides error codes, error context, and error handling utilities
 * for the polycall protocol stack.
 */

#ifndef POLYCALL_CORE_POLYCALL_ERROR_H
#define POLYCALL_CORE_POLYCALL_ERROR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Error codes extend the base polycall_status_t */
typedef enum {
    POLYCALL_CORE_SUCCESS          =  0,
    POLYCALL_CORE_ERROR_GENERAL    = -1,
    POLYCALL_CORE_ERROR_MEMORY     = -2,
    POLYCALL_CORE_ERROR_INVALID    = -3,
    POLYCALL_CORE_ERROR_STATE      = -4,
    POLYCALL_CORE_ERROR_NETWORK    = -5,
    POLYCALL_CORE_ERROR_AUTH       = -6,
    POLYCALL_CORE_ERROR_PROTOCOL   = -7,
    POLYCALL_CORE_ERROR_TIMEOUT    = -8,
    POLYCALL_CORE_ERROR_NOT_FOUND          = -9,
    POLYCALL_CORE_ERROR_EXISTS             = -10,
    POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED  = -11,
    POLYCALL_CORE_ERROR_CONNECTION_CLOSED  = -12
} polycall_core_error_t;

/* Aliases used by socket module source files */
#define POLYCALL_CORE_ERROR_INVALID_PARAMETERS  POLYCALL_CORE_ERROR_INVALID
#define POLYCALL_CORE_ERROR_OUT_OF_MEMORY       POLYCALL_CORE_ERROR_MEMORY

/* Error context for detailed error reporting */
typedef struct polycall_error_context {
    polycall_core_error_t code;
    const char* message;
    const char* file;
    int line;
    const char* function;
} polycall_error_context_t;

/* Error handling functions */
static inline const char* polycall_error_string(polycall_core_error_t err) {
    switch (err) {
        case POLYCALL_CORE_SUCCESS:         return "success";
        case POLYCALL_CORE_ERROR_GENERAL:   return "general error";
        case POLYCALL_CORE_ERROR_MEMORY:    return "memory allocation failed";
        case POLYCALL_CORE_ERROR_INVALID:   return "invalid parameter";
        case POLYCALL_CORE_ERROR_STATE:     return "invalid state";
        case POLYCALL_CORE_ERROR_NETWORK:   return "network error";
        case POLYCALL_CORE_ERROR_AUTH:      return "authentication error";
        case POLYCALL_CORE_ERROR_PROTOCOL:  return "protocol error";
        case POLYCALL_CORE_ERROR_TIMEOUT:   return "operation timed out";
        case POLYCALL_CORE_ERROR_NOT_FOUND: return "not found";
        case POLYCALL_CORE_ERROR_EXISTS:    return "already exists";
        default:                            return "unknown error";
    }
}

#define POLYCALL_SET_ERROR(ctx, err_code, msg) do { \
    if (ctx) { \
        (ctx)->code = (err_code); \
        (ctx)->message = (msg); \
        (ctx)->file = __FILE__; \
        (ctx)->line = __LINE__; \
        (ctx)->function = __func__; \
    } \
} while(0)

#endif /* POLYCALL_CORE_POLYCALL_ERROR_H */
