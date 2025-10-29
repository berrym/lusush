/*
 * event_system.h - Event System Header (Phase 1)
 * 
 * Event-driven architecture for LLE.
 * Phase 1 provides core event infrastructure with essential functionality.
 * 
 * Phase 1 Scope:
 * - Basic event types (input, terminal, buffer, display, system)
 * - FIFO event queue
 * - Handler registration and dispatch
 * - Thread-safe queue operations
 * 
 * Future Phases:
 * - Priority queues
 * - Event filtering
 * - Advanced statistics
 * - Plugin events
 * - Timer events
 * 
 * Spec 04: Event System - Phase 1
 */

#ifndef LLE_EVENT_SYSTEM_H
#define LLE_EVENT_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include "error_handling.h"
#include "memory_management.h"

/* Forward declarations */
typedef struct lle_event lle_event_t;
typedef struct lle_event_queue lle_event_queue_t;
typedef struct lle_event_handler lle_event_handler_t;
typedef struct lle_event_system lle_event_system_t;

/*
 * Event Kinds (Phase 1 - Essential types only)
 * Note: Using 'kind' instead of 'type' to avoid conflict with memory pool event_type
 */
typedef enum {
    /* Terminal Input Events (0x1000 - 0x1FFF) */
    LLE_EVENT_KEY_PRESS = 0x1000,
    LLE_EVENT_KEY_SEQUENCE = 0x1001,
    LLE_EVENT_MOUSE_EVENT = 0x1002,
    
    /* Terminal State Events (0x3000 - 0x3FFF) */
    LLE_EVENT_TERMINAL_RESIZE = 0x3000,
    
    /* Buffer Events (0x5000 - 0x5FFF) */
    LLE_EVENT_BUFFER_CHANGED = 0x5000,
    LLE_EVENT_CURSOR_MOVED = 0x5001,
    
    /* Display Events (0xC000 - 0xCFFF) */
    LLE_EVENT_DISPLAY_UPDATE = 0xC000,
    LLE_EVENT_DISPLAY_REFRESH = 0xC001,
    
    /* System Events (0x9000 - 0x9FFF) */
    LLE_EVENT_SYSTEM_ERROR = 0x9000,
    LLE_EVENT_SYSTEM_SHUTDOWN = 0x9001,
} lle_event_kind_t;

/*
 * Event Structure (Phase 1)
 */
struct lle_event {
    lle_event_kind_t type;           /* Event kind/type */
    uint64_t sequence_number;        /* Global sequence number */
    uint64_t timestamp;              /* Event timestamp (microseconds) */
    
    /* Event data */
    void *data;                      /* Event-specific data */
    size_t data_size;                /* Size of data */
    
    /* Queue linkage */
    struct lle_event *next;          /* Next event in queue */
};

/*
 * Event Handler Function Type
 */
typedef lle_result_t (*lle_event_handler_fn)(lle_event_t *event, void *user_data);

/*
 * Event Handler Structure
 */
struct lle_event_handler {
    lle_event_kind_t event_type;     /* Event kind this handler handles */
    lle_event_handler_fn handler;    /* Handler function */
    void *user_data;                 /* User data passed to handler */
    char name[64];                   /* Handler name (for debugging) */
};

/*
 * Event Queue Structure (Phase 1 - circular buffer)
 */
struct lle_event_queue {
    lle_event_t **events;            /* Array of event pointers */
    size_t capacity;                 /* Queue capacity */
    size_t head;                     /* Read position */
    size_t tail;                     /* Write position */
    size_t count;                    /* Current event count */
    pthread_mutex_t mutex;           /* Thread safety */
};

/*
 * Event System Structure (Phase 1)
 */
struct lle_event_system {
    lle_event_queue_t *queue;        /* Single event queue */
    lle_event_handler_t **handlers;  /* Handler array */
    size_t handler_count;            /* Number of handlers */
    size_t handler_capacity;         /* Handler array capacity */
    
    lle_memory_pool_t *event_pool;   /* Memory pool for events */
    pthread_mutex_t system_mutex;    /* System-wide mutex */
    
    uint64_t sequence_counter;       /* Event sequence counter */
    bool active;                     /* System active flag */
    
    /* Statistics */
    uint64_t events_created;         /* Total events created */
    uint64_t events_dispatched;      /* Total events dispatched */
    uint64_t events_dropped;         /* Events dropped (queue full) */
};

/* ============================================================================
 * Event System Lifecycle Functions
 * ============================================================================ */

/*
 * Initialize event system
 */
lle_result_t lle_event_system_init(lle_event_system_t **system,
                                   lle_memory_pool_t *pool);

/*
 * Destroy event system
 */
void lle_event_system_destroy(lle_event_system_t *system);

/*
 * Start event system (begin processing)
 */
lle_result_t lle_event_system_start(lle_event_system_t *system);

/*
 * Stop event system (stop processing)
 */
lle_result_t lle_event_system_stop(lle_event_system_t *system);

/* ============================================================================
 * Event Creation and Destruction
 * ============================================================================ */

/*
 * Create event
 */
lle_result_t lle_event_create(lle_event_system_t *system,
                              lle_event_kind_t type,
                              void *data,
                              size_t data_size,
                              lle_event_t **event);

/*
 * Destroy event
 */
void lle_event_destroy(lle_event_system_t *system, lle_event_t *event);

/*
 * Clone event
 */
lle_result_t lle_event_clone(lle_event_system_t *system,
                             lle_event_t *source,
                             lle_event_t **dest);

/* ============================================================================
 * Event Queue Operations
 * ============================================================================ */

/*
 * Initialize event queue
 */
lle_result_t lle_event_queue_init(lle_event_queue_t **queue, size_t capacity);

/*
 * Destroy event queue
 */
void lle_event_queue_destroy(lle_event_queue_t *queue);

/*
 * Enqueue event
 */
lle_result_t lle_event_enqueue(lle_event_system_t *system, lle_event_t *event);

/*
 * Dequeue event
 */
lle_result_t lle_event_dequeue(lle_event_system_t *system, lle_event_t **event);

/*
 * Get queue size
 */
size_t lle_event_queue_size(lle_event_system_t *system);

/*
 * Check if queue is empty
 */
bool lle_event_queue_empty(lle_event_system_t *system);

/*
 * Check if queue is full
 */
bool lle_event_queue_full(lle_event_system_t *system);

/* ============================================================================
 * Handler Management
 * ============================================================================ */

/*
 * Register event handler
 */
lle_result_t lle_event_handler_register(lle_event_system_t *system,
                                        lle_event_kind_t type,
                                        lle_event_handler_fn handler,
                                        void *user_data,
                                        const char *name);

/*
 * Unregister specific handler
 */
lle_result_t lle_event_handler_unregister(lle_event_system_t *system,
                                          lle_event_kind_t type,
                                          const char *name);

/*
 * Unregister all handlers for event type
 */
lle_result_t lle_event_handler_unregister_all(lle_event_system_t *system,
                                              lle_event_kind_t type);

/*
 * Get handler count for event type
 */
size_t lle_event_handler_count(lle_event_system_t *system,
                               lle_event_kind_t type);

/* ============================================================================
 * Event Processing
 * ============================================================================ */

/*
 * Dispatch event to all registered handlers
 */
lle_result_t lle_event_dispatch(lle_event_system_t *system, lle_event_t *event);

/*
 * Process events from queue (up to max_events)
 */
lle_result_t lle_event_process_queue(lle_event_system_t *system,
                                     uint32_t max_events);

/*
 * Process all events in queue
 */
lle_result_t lle_event_process_all(lle_event_system_t *system);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/*
 * Get current timestamp in microseconds
 */
uint64_t lle_event_get_timestamp_us(void);

/*
 * Get event type name (for debugging)
 */
const char *lle_event_type_name(lle_event_kind_t type);

/*
 * Get event system statistics
 */
lle_result_t lle_event_system_get_stats(lle_event_system_t *system,
                                        uint64_t *created,
                                        uint64_t *dispatched,
                                        uint64_t *dropped);

#endif /* LLE_EVENT_SYSTEM_H */
