/**
 * @file polycall_socket_security.h
 * @brief WebSocket security layer for LibPolyCall
 */

#ifndef POLYCALL_CORE_SOCKET_SECURITY_H
#define POLYCALL_CORE_SOCKET_SECURITY_H

#include "polycall/core/socket/polycall_socket.h"
#include "polycall/core/auth/polycall_auth_token.h"

/* Security context is opaque - defined in polycall_socket_security.c */
typedef struct polycall_socket_security_context polycall_socket_security_context_t;

/* Authentication result with scopes and roles */
typedef struct polycall_socket_auth_result {
    bool        authenticated;
    uint32_t    granted_permissions;
    char        identity[256];
    char        error_message[256];
    uint64_t    token_expiry;
    char        scopes[POLYCALL_MAX_SOCKET_SCOPES][128];
    size_t      scope_count;
    char        roles[POLYCALL_MAX_SOCKET_ROLES][64];
    size_t      role_count;
} polycall_socket_auth_result_t;

/* Per-connection security state */
typedef struct polycall_socket_security {
    uint32_t                magic;
    polycall_auth_token_t*  token;
    bool                    authenticated;
    bool                    tls_enabled;
    char                    peer_identity[256];
} polycall_socket_security_t;

/* Security lifecycle */
polycall_core_error_t polycall_socket_security_init(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    polycall_socket_security_context_t** security_ctx);

polycall_core_error_t polycall_socket_security_configure(
    polycall_socket_security_context_t* security_ctx,
    bool enable_token_auth,
    bool require_secure_connection,
    uint32_t max_auth_failures,
    uint32_t token_refresh_interval);

polycall_core_error_t polycall_socket_authenticate(
    polycall_socket_security_context_t* security_ctx,
    const char* token,
    polycall_socket_auth_result_t* result);

bool polycall_socket_is_authorized(
    polycall_socket_security_context_t* security_ctx,
    const polycall_socket_auth_result_t* auth_result,
    const char* required_scope);

bool polycall_socket_has_role(
    polycall_socket_security_context_t* security_ctx,
    const polycall_socket_auth_result_t* auth_result,
    const char* required_role);

polycall_core_error_t polycall_socket_generate_challenge(
    polycall_socket_security_context_t* security_ctx,
    char* challenge_buffer, size_t buffer_size);

polycall_core_error_t polycall_socket_verify_challenge(
    polycall_socket_security_context_t* security_ctx,
    const char* challenge, const char* response,
    const char* token, bool* valid);

bool polycall_socket_token_needs_refresh(
    polycall_socket_security_context_t* security_ctx,
    const polycall_socket_auth_result_t* auth_result,
    uint64_t current_time);

void polycall_socket_security_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_socket_security_context_t* security_ctx);

/* Legacy per-connection security ops */
polycall_core_error_t polycall_socket_security_create(
    polycall_socket_security_t** sec);
polycall_core_error_t polycall_socket_security_destroy(
    polycall_socket_security_t* sec);

#endif /* POLYCALL_CORE_SOCKET_SECURITY_H */
