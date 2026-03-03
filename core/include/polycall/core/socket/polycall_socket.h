/**
 * @file polycall_socket.h
 * @brief WebSocket implementation for LibPolyCall
 */

#ifndef POLYCALL_CORE_SOCKET_H
#define POLYCALL_CORE_SOCKET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/telemetry/polycall_telemetry.h"
#include "polycall/core/auth/polycall_auth_token.h"

/* Forward declarations for opaque context types */
typedef struct polycall_socket_context polycall_socket_context_t;

/* Socket configuration */
typedef struct polycall_socket_config {
    size_t   max_connections;
    uint32_t connection_timeout_ms;
    bool     use_tls;
    uint32_t ping_interval_ms;
    size_t   max_message_size;
    uint32_t worker_threads;
    bool     enable_compression;
    bool     auto_reconnect;
    uint32_t reconnect_max_attempts;
    uint32_t reconnect_base_delay_ms;
} polycall_socket_config_t;

/* Connection options */
typedef struct polycall_socket_connect_options {
    const char* protocol;
    bool        use_tls;
    uint32_t    timeout_ms;
    bool        auto_reconnect;
} polycall_socket_connect_options_t;

/* Socket data types */
typedef enum {
    POLYCALL_SOCKET_DATA_TEXT   = 0,
    POLYCALL_SOCKET_DATA_BINARY = 1,
    POLYCALL_SOCKET_DATA_JSON   = 2
} polycall_socket_data_type_t;

/* Forward declare connection for callback types */
typedef struct polycall_socket_connection polycall_socket_connection_t;

/* Callback types */
typedef void (*polycall_socket_callback_t)(polycall_socket_connection_t* conn, void* user_data);
typedef void (*polycall_socket_message_handler_t)(
    polycall_socket_connection_t* conn,
    const void* data, size_t len,
    polycall_socket_data_type_t type,
    void* user_data);

/* Socket connection handle */
struct polycall_socket_connection {
    uint32_t        id;
    int             fd;
    bool            is_active;
    bool            is_authenticated;
    bool            is_connected;
    void*           user_data;
    char            url[512];
    uint64_t        created_time;
    uint16_t        close_code;
    char            close_reason[256];
    polycall_socket_connect_options_t options;
    polycall_socket_context_t*        socket_ctx;
    pthread_mutex_t connection_mutex;
    polycall_socket_message_handler_t message_handler;
    void*           handler_user_data;
};

/* Socket server handle */
typedef struct polycall_socket_server {
    int             fd;
    uint16_t        port;
    char            bind_address[256];
    bool            running;
    bool            is_running;
    polycall_socket_context_t* socket_ctx;
    pthread_mutex_t server_mutex;
    void*           user_data;
} polycall_socket_server_t;

/* WebSocket connection states */
typedef enum {
    POLYCALL_WS_CONNECTING = 0,
    POLYCALL_WS_OPEN       = 1,
    POLYCALL_WS_CLOSING    = 2,
    POLYCALL_WS_CLOSED     = 3
} polycall_ws_state_t;

/* WebSocket frame opcodes */
typedef enum {
    POLYCALL_WS_OP_CONTINUATION = 0x0,
    POLYCALL_WS_OP_TEXT         = 0x1,
    POLYCALL_WS_OP_BINARY       = 0x2,
    POLYCALL_WS_OP_CLOSE        = 0x8,
    POLYCALL_WS_OP_PING         = 0x9,
    POLYCALL_WS_OP_PONG         = 0xA
} polycall_ws_opcode_t;

/* WebSocket low-level connection handle */
typedef struct polycall_socket {
    uint32_t             magic;
    int                  fd;
    polycall_ws_state_t  state;
    pthread_mutex_t      lock;
    void*                user_data;
    char*                url;
    uint16_t             port;
    bool                 is_server;
    size_t               max_frame_size;
} polycall_socket_t;

/* High-level socket API (used by polycall_socket.c) */
polycall_core_error_t polycall_socket_init(
    polycall_core_context_t* core_ctx,
    polycall_socket_context_t** socket_ctx,
    const polycall_socket_config_t* config);

polycall_core_error_t polycall_socket_create_server(
    polycall_socket_context_t* socket_ctx,
    const char* bind_address, uint16_t port,
    polycall_socket_server_t** server);

polycall_core_error_t polycall_socket_connect(
    polycall_socket_context_t* socket_ctx,
    const char* url,
    const polycall_socket_connect_options_t* options,
    polycall_socket_connection_t** connection);

polycall_core_error_t polycall_socket_send(
    polycall_socket_connection_t* connection,
    const void* data, size_t data_size,
    polycall_socket_data_type_t data_type);

polycall_core_error_t polycall_socket_register_handler(
    polycall_socket_connection_t* connection,
    polycall_socket_message_handler_t handler,
    void* user_data);

polycall_core_error_t polycall_socket_close(
    polycall_socket_connection_t* connection,
    uint16_t close_code, const char* reason);

void polycall_socket_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_socket_context_t* socket_ctx);

polycall_socket_config_t polycall_socket_create_default_config(void);

/* Low-level socket operations */
polycall_core_error_t polycall_socket_create(polycall_socket_t** sock);
polycall_core_error_t polycall_socket_destroy(polycall_socket_t* sock);
polycall_core_error_t polycall_socket_listen(polycall_socket_t* sock,
                                              uint16_t port, int backlog);
polycall_core_error_t polycall_socket_recv(polycall_socket_t* sock,
                                            void* buf, size_t* len,
                                            polycall_ws_opcode_t* opcode);

#endif /* POLYCALL_CORE_SOCKET_H */
