#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Import core types (NetworkEndpoint, ClientState, constants) */
#include "libpolycall/core/types.h"

/* Network packet for send/receive operations */
typedef struct network_packet {
    void*    data;
    size_t   size;
    int      flags;
} NetworkPacket;

/* Network event handlers */
typedef struct network_handlers {
    void (*on_connect)(NetworkEndpoint* endpoint);
    void (*on_disconnect)(NetworkEndpoint* endpoint);
    void (*on_receive)(NetworkEndpoint* endpoint, NetworkPacket* packet);
} NetworkHandlers;

/* Network program - manages server endpoints + client pool */
typedef struct network_program {
    NetworkEndpoint* endpoints;      /* Dynamic array of endpoints */
    size_t           count;          /* Number of endpoints */
    ClientState      clients[NET_MAX_CLIENTS];
    pthread_mutex_t  clients_lock;
    bool             running;
    void*            phantom;        /* Opaque state pointer */
    NetworkHandlers  handlers;       /* Event callbacks */
} NetworkProgram;

/* Function declarations */
bool net_init(NetworkEndpoint* endpoint);
void net_close(NetworkEndpoint* endpoint);
bool net_release_port(uint16_t port);
NetworkEndpoint* network_endpoint_create(const char* address, int port);
void network_endpoint_destroy(NetworkEndpoint* endpoint);
void net_init_client_state(ClientState* state);
void net_cleanup_client_state(ClientState* state);
bool net_is_port_in_use(uint16_t port);
ssize_t net_send(NetworkEndpoint* endpoint, NetworkPacket* packet);
ssize_t net_receive(NetworkEndpoint* endpoint, NetworkPacket* packet);
bool net_add_client(NetworkProgram* program, int socket_fd, struct sockaddr_in addr);
void net_remove_client(NetworkProgram* program, int socket_fd);
void net_init_program(NetworkProgram* program);
void net_cleanup_program(NetworkProgram* program);
void net_run(NetworkProgram* program);

#endif
