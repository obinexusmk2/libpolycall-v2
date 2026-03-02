/**
 * @file polycall_telemetry.h
 * @brief Telemetry subsystem for LibPolyCall core
 *
 * Provides telemetry event recording, severity levels, and event types.
 * Currently implemented as stub/no-op for compilation; will be backed
 * by real telemetry infrastructure in a future phase.
 */

#ifndef POLYCALL_CORE_TELEMETRY_H
#define POLYCALL_CORE_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Forward declare core context */
typedef struct polycall_core_context polycall_core_context_t;

/* Telemetry context (opaque) */
typedef struct polycall_telemetry_context polycall_telemetry_context_t;

/* Telemetry severity levels */
typedef enum {
    POLYCALL_TELEMETRY_DEBUG   = 0,
    POLYCALL_TELEMETRY_INFO    = 1,
    POLYCALL_TELEMETRY_WARNING = 2,
    POLYCALL_TELEMETRY_ERROR   = 3
} polycall_telemetry_severity_t;

/* Telemetry event IDs */
typedef enum {
    POLYCALL_TELEMETRY_EVENT_COMPONENT_INIT         = 1,
    POLYCALL_TELEMETRY_EVENT_COMPONENT_CLEANUP       = 2,
    POLYCALL_TELEMETRY_EVENT_SOCKET_SERVER_CREATE     = 10,
    POLYCALL_TELEMETRY_EVENT_SOCKET_CONNECT           = 11,
    POLYCALL_TELEMETRY_EVENT_SOCKET_SEND              = 12,
    POLYCALL_TELEMETRY_EVENT_SOCKET_CLOSE             = 13,
    POLYCALL_TELEMETRY_EVENT_SOCKET_DISCONNECT        = 14
} polycall_telemetry_event_id_t;

/* Telemetry event structure */
typedef struct polycall_telemetry_event {
    polycall_telemetry_event_id_t  event_id;
    polycall_telemetry_severity_t  severity;
    const char*                    component;
    const char*                    source_module;
    char                           message[256];
    uint64_t                       timestamp;
} polycall_telemetry_event_t;

/* Record a telemetry event (no-op stub for now) */
static inline void polycall_telemetry_record_event(
    polycall_telemetry_context_t* ctx,
    const polycall_telemetry_event_t* event)
{
    (void)ctx;
    (void)event;
    /* No-op: telemetry infrastructure not yet wired */
}

#endif /* POLYCALL_CORE_TELEMETRY_H */
