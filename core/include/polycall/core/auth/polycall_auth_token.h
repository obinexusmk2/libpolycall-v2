/**
 * @file polycall_auth_token.h
 * @brief Authentication token management for LibPolyCall
 *
 * Implements zero-trust authentication tokens for the polycall
 * protocol security layer.
 */

#ifndef POLYCALL_CORE_AUTH_TOKEN_H
#define POLYCALL_CORE_AUTH_TOKEN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "polycall/core/polycall/polycall_error.h"

/* Forward declare core context */
typedef struct polycall_core_context polycall_core_context_t;

/* Auth context (opaque forward declaration) */
typedef struct polycall_auth_context polycall_auth_context_t;

/* Security scope/role limits */
#define POLYCALL_MAX_SOCKET_SCOPES  32
#define POLYCALL_MAX_SOCKET_ROLES   16

/* Token types */
typedef enum {
    POLYCALL_TOKEN_NONE     = 0,
    POLYCALL_TOKEN_BEARER   = 1,
    POLYCALL_TOKEN_SESSION  = 2,
    POLYCALL_TOKEN_API_KEY  = 3
} polycall_token_type_t;

/* Authentication token */
typedef struct polycall_auth_token {
    polycall_token_type_t type;
    char     token_id[64];
    char     value[256];
    time_t   issued_at;
    time_t   expires_at;
    bool     revoked;
    uint32_t permissions;
} polycall_auth_token_t;

/* Token claims (extracted from validated token) */
typedef struct polycall_token_claims {
    const char*  subject;
    const char** scopes;
    size_t       scope_count;
    const char** roles;
    size_t       role_count;
    uint64_t     expires_at;
    uint32_t     permissions;
} polycall_token_claims_t;

/* Token validation result */
typedef struct token_validation_result {
    bool                      is_valid;
    const char*               error_message;
    uint32_t                  granted_permissions;
    polycall_token_claims_t*  claims;
} token_validation_result_t;

/* Token operations */
static inline bool polycall_auth_token_is_valid(const polycall_auth_token_t* token) {
    if (!token || token->revoked) return false;
    if (token->type == POLYCALL_TOKEN_NONE) return false;
    time_t now = time(NULL);
    if (token->expires_at > 0 && now > token->expires_at) return false;
    return true;
}

static inline bool polycall_auth_token_has_permission(
    const polycall_auth_token_t* token, uint32_t perm)
{
    if (!polycall_auth_token_is_valid(token)) return false;
    return (token->permissions & perm) == perm;
}

/**
 * Extended token validation (stub implementation)
 * Signature: (core_ctx, auth_ctx, token_string, &result_ptr) -> error_t
 */
static inline polycall_core_error_t polycall_auth_validate_token_ex(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    const char* token_string,
    token_validation_result_t** out_result)
{
    (void)core_ctx;
    (void)auth_ctx;

    if (!token_string || !out_result) {
        return POLYCALL_CORE_ERROR_INVALID;
    }

    /* Allocate result (caller must free via polycall_auth_free_token_validation_result) */
    token_validation_result_t* result = (token_validation_result_t*)calloc(1, sizeof(token_validation_result_t));
    if (!result) {
        return POLYCALL_CORE_ERROR_MEMORY;
    }

    /* Stub: accept any non-empty token */
    result->is_valid = (token_string[0] != '\0');
    result->error_message = result->is_valid ? NULL : "empty token";
    result->granted_permissions = 0xFFFFFFFF; /* all permissions for stub */
    result->claims = NULL; /* no claims parsing in stub */

    *out_result = result;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Free token validation result
 * Signature: (core_ctx, result_ptr)
 */
static inline void polycall_auth_free_token_validation_result(
    polycall_core_context_t* core_ctx,
    token_validation_result_t* result)
{
    (void)core_ctx;
    if (result) {
        free(result);
    }
}

#endif /* POLYCALL_CORE_AUTH_TOKEN_H */
