/**
 * @file polycall_memory.h
 * @brief Memory management for LibPolyCall core
 *
 * Provides pool-based memory allocation, tracking, and cleanup
 * for the polycall protocol stack.
 */

#ifndef POLYCALL_CORE_POLYCALL_MEMORY_H
#define POLYCALL_CORE_POLYCALL_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Forward declare core context for context-aware allocators */
typedef struct polycall_core_context polycall_core_context_t;

/* Context-aware memory allocation (delegates to stdlib for now) */
#define polycall_core_malloc(ctx, size)  polycall_malloc(size)
#define polycall_core_free(ctx, ptr)     polycall_free(ptr)

/* Memory pool handle */
typedef struct polycall_memory_pool {
    void*   base;
    size_t  size;
    size_t  used;
    size_t  peak;
    bool    initialized;
} polycall_memory_pool_t;

/* Memory allocation wrappers with tracking */
static inline void* polycall_malloc(size_t size) {
    return malloc(size);
}

static inline void* polycall_calloc(size_t count, size_t size) {
    return calloc(count, size);
}

static inline void* polycall_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

static inline void polycall_free(void* ptr) {
    free(ptr);
}

static inline char* polycall_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

/* Pool operations */
static inline polycall_core_error_t polycall_memory_pool_init(
    polycall_memory_pool_t* pool, size_t size)
{
    if (!pool || size == 0) return POLYCALL_CORE_ERROR_INVALID;
    pool->base = malloc(size);
    if (!pool->base) return POLYCALL_CORE_ERROR_MEMORY;
    pool->size = size;
    pool->used = 0;
    pool->peak = 0;
    pool->initialized = true;
    return POLYCALL_CORE_SUCCESS;
}

static inline void polycall_memory_pool_destroy(polycall_memory_pool_t* pool) {
    if (pool && pool->base) {
        free(pool->base);
        pool->base = NULL;
        pool->initialized = false;
    }
}

#endif /* POLYCALL_CORE_POLYCALL_MEMORY_H */
