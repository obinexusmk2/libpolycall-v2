/**
 * @file polycall_socket_protocol.h
 * @brief WebSocket protocol handling for LibPolyCall
 */

#ifndef POLYCALL_CORE_SOCKET_PROTOCOL_H
#define POLYCALL_CORE_SOCKET_PROTOCOL_H

#include "polycall/core/socket/polycall_socket.h"

/* Protocol message types */
typedef enum {
    POLYCALL_PROTO_REQUEST  = 0,
    POLYCALL_PROTO_RESPONSE = 1,
    POLYCALL_PROTO_EVENT    = 2,
    POLYCALL_PROTO_ERROR    = 3
} polycall_proto_msg_type_t;

/* Protocol message */
typedef struct polycall_protocol_message {
    polycall_proto_msg_type_t type;
    uint32_t  sequence_id;
    char      method[64];
    void*     payload;
    size_t    payload_size;
} polycall_protocol_message_t;

/* Protocol operations */
polycall_core_error_t polycall_protocol_encode(
    const polycall_protocol_message_t* msg, void* buf, size_t* len);
polycall_core_error_t polycall_protocol_decode(
    const void* buf, size_t len, polycall_protocol_message_t* msg);

#endif /* POLYCALL_CORE_SOCKET_PROTOCOL_H */
