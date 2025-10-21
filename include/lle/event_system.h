/**
 * @file event_system.h
 * @brief LLE Event System - Phase 1: Core Event Infrastructure
 * 
 * Spec Reference: 04_event_system_complete.md
 * Phase: 1 of 4
 * 
 * Phase 1 Scope:
 * - Basic event types (9 essential types)
 * - Simple FIFO event queue
 * - Event handler registration and dispatch
 * - Core event lifecycle management
 * 
 * Phase 2+ Scope (Deferred):
 * - Priority queues
 * - Event filtering system
 * - Advanced event types (mouse, paste, timer, plugin)
 * - Event statistics and monitoring
 * - Lusush display integration
 * - Asynchronous processing
 * 
 * Design Principles (Phase 1):
 * - Simple FIFO queue (no priority scheduling yet)
 * - Thread-safe operations (mutex-protected)
 * - Memory pool integration
 * - Comprehensive error handling
 * - Event creation < 10μs, dispatch < 100μs
 */

#ifndef LLE_EVENT_SYSTEM_H
#define LLE_EVENT_SYSTEM_H

#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>

/* ============================================================================
 * EVENT TYPE DEFINITIONS
 * ============================================================================
 */

/**
 * @brief System event type enumeration (Phase 1: Essential types only)
 * 
 * Phase 1 includes only core event types needed for Specs 03, 06, 08.
 * Phase 2+ will add mouse, paste, timer, plugin, and other advanced types.
 * 
 * Note: Renamed to lle_system_event_type_t to avoid conflict with
 * lle_event_type_t defined in memory_management.h
 */
typedef enum {
    /* Terminal Input Events */
    LLE_EVENT_KEY_PRESS = 0x1000,        /**< Single key press */
    LLE_EVENT_KEY_SEQUENCE,              /**< Multi-key sequence (e.g., escape codes) */
    
    /* Terminal State Events */
    LLE_EVENT_TERMINAL_RESIZE = 0x3000,  /**< Terminal size changed */
    
    /* Buffer Events (for Spec 03) */
    LLE_EVENT_BUFFER_CHANGED = 0x5000,   /**< Buffer content changed */
    LLE_EVENT_CURSOR_MOVED,              /**< Cursor position changed */
    
    /* Display Events (for Spec 08) */
    LLE_EVENT_DISPLAY_UPDATE = 0xC000,   /**< Display update required */
    LLE_EVENT_DISPLAY_REFRESH,           /**< Display refresh request */
    
    /* System Events */
    LLE_EVENT_SYSTEM_ERROR = 0x9000,     /**< System error occurred */
    LLE_EVENT_SYSTEM_SHUTDOWN,           /**< System shutdown request */
} lle_system_event_type_t;

/**
 * @brief Event priority levels (Phase 1: Not used, prepared for Phase 2)
 */
typedef enum {
    LLE_EVENT_PRIORITY_LOW = 0,
    LLE_EVENT_PRIORITY_NORMAL = 1,
    LLE_EVENT_PRIORITY_HIGH = 2,
    LLE_EVENT_PRIORITY_CRITICAL = 3,
} lle_event_priority_t;

/* ============================================================================
 * EVENT STRUCTURE
 * ============================================================================
 */

/**
 * @brief Event structure (Phase 1: Simplified version)
 * 
 * Phase 1 uses simple void* data pointer instead of complex union.
 * Phase 2+ will add union-based typed data, processing state, etc.
 */
typedef struct lle_event {
    /* Event identification */
    lle_system_event_type_t type;        /**< Event type */
    uint64_t sequence_number;            /**< Global sequence number */
    uint64_t timestamp;                  /**< Event creation time (microseconds) */
    
    /* Event data (Phase 1: Simple pointer) */
    void *data;                          /**< Event-specific data */
    size_t data_size;                    /**< Size of event data */
    
    /* Queue linkage */
    struct lle_event *next;              /**< Next event in queue */
} lle_event_t;

/* ============================================================================
 * EVENT HANDLER DEFINITION
 * ============================================================================
 */

/**
 * @brief Event handler function type
 * 
 * @param event The event to handle
 * @param user_data User-provided data passed during registration
 * @return LLE_SUCCESS if handled successfully, error code otherwise
 */
typedef lle_result_t (*lle_event_handler_fn)(lle_event_t *event, void *user_data);

/**
 * @brief Event handler registration entry
 */
typedef struct {
    lle_system_event_type_t event_type;  /**< Event type this handler processes */
    lle_event_handler_fn handler;        /**< Handler function pointer */
    void *user_data;                     /**< User data passed to handler */
    char name[64];                       /**< Handler name (for debugging) */
} lle_event_handler_t;

/* ============================================================================
 * EVENT QUEUE STRUCTURE
 * ============================================================================
 */

/**
 * @brief Event queue (Phase 1: Simple circular buffer FIFO)
 * 
 * Thread-safe circular buffer for event queueing.
 * Phase 2+ will add priority queue support.
 */
typedef struct {
    lle_event_t **events;                /**< Array of event pointers */
    size_t capacity;                     /**< Maximum queue capacity */
    size_t head;                         /**< Read position */
    size_t tail;                         /**< Write position */
    size_t count;                        /**< Current event count */
    pthread_mutex_t mutex;               /**< Thread safety mutex */
} lle_event_queue_t;

/* ============================================================================
 * EVENT SYSTEM STRUCTURE
 * ============================================================================
 */

/**
 * @brief Event system (Phase 1: Simplified version)
 * 
 * Main event system coordinator. Phase 1 uses single queue.
 * Phase 2+ will add priority queues, filtering, statistics, etc.
 */
typedef struct {
    /* Event queue */
    lle_event_queue_t *queue;            /**< Main event queue (FIFO) */
    
    /* Handler registry */
    lle_event_handler_t **handlers;      /**< Registered handlers array */
    size_t handler_count;                /**< Number of registered handlers */
    size_t handler_capacity;             /**< Handler array capacity */
    
    /* Memory management */
    lle_memory_pool_t *event_pool;       /**< Memory pool for events */
    
    /* Synchronization */
    pthread_mutex_t system_mutex;        /**< System-level mutex */
    
    /* State */
    uint64_t sequence_counter;           /**< Event sequence counter */
    bool active;                         /**< System is active */
} lle_event_system_t;

/* ============================================================================
 * EVENT SYSTEM LIFECYCLE
 * ============================================================================
 */

/**
 * @brief Initialize event system
 * 
 * Creates and initializes the event system with:
 * - Event queue (default capacity: 1024 events)
 * - Handler registry (initial capacity: 64 handlers)
 * - Memory pool for events
 * 
 * @param[out] system Pointer to receive initialized system
 * @param pool Memory pool to use for event allocations
 * @return LLE_SUCCESS on success, error code on failure
 * 
 * @note System starts in inactive state. Call lle_event_system_start().
 */
lle_result_t lle_event_system_init(
    lle_event_system_t **system,
    lle_memory_pool_t *pool
);

/**
 * @brief Destroy event system
 * 
 * Cleans up all resources:
 * - Destroys all queued events
 * - Unregisters all handlers
 * - Destroys event queue
 * - Frees memory pool
 * 
 * @param system Event system to destroy (can be NULL)
 */
void lle_event_system_destroy(
    lle_event_system_t *system
);

/**
 * @brief Start event system
 * 
 * Activates the event system for event processing.
 * Must be called after init before events can be processed.
 * 
 * @param system Event system to start
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_event_system_start(
    lle_event_system_t *system
);

/**
 * @brief Stop event system
 * 
 * Deactivates the event system. Queued events remain but
 * no new events will be processed until restarted.
 * 
 * @param system Event system to stop
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_event_system_stop(
    lle_event_system_t *system
);

/* ============================================================================
 * EVENT CREATION AND DESTRUCTION
 * ============================================================================
 */

/**
 * @brief Create new event
 * 
 * Allocates and initializes a new event from the event pool.
 * Event data is copied into pool-allocated memory.
 * 
 * @param system Event system
 * @param type Event type
 * @param data Event-specific data to copy (can be NULL)
 * @param data_size Size of event data (0 if data is NULL)
 * @param[out] event Pointer to receive created event
 * @return LLE_SUCCESS on success, error code on failure
 * 
 * @note Event must be destroyed with lle_event_destroy() when done
 * @note Event data is copied, caller retains ownership of original
 */
lle_result_t lle_event_create(
    lle_event_system_t *system,
    lle_system_event_type_t type,
    const void *data,
    size_t data_size,
    lle_event_t **event
);

/**
 * @brief Destroy event
 * 
 * Frees event and its data back to memory pool.
 * 
 * @param system Event system
 * @param event Event to destroy (can be NULL)
 */
void lle_event_destroy(
    lle_event_system_t *system,
    lle_event_t *event
);

/**
 * @brief Clone event
 * 
 * Creates a deep copy of an event, including its data.
 * 
 * @param system Event system
 * @param source Event to clone
 * @param[out] dest Pointer to receive cloned event
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_event_clone(
    lle_event_system_t *system,
    const lle_event_t *source,
    lle_event_t **dest
);

/* ============================================================================
 * EVENT QUEUE OPERATIONS
 * ============================================================================
 */

/**
 * @brief Initialize event queue
 * 
 * Creates circular buffer event queue with specified capacity.
 * 
 * @param[out] queue Pointer to receive initialized queue
 * @param capacity Maximum number of events queue can hold
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_event_queue_init(
    lle_event_queue_t **queue,
    size_t capacity
);

/**
 * @brief Destroy event queue
 * 
 * Destroys queue. Does NOT destroy queued events (caller's responsibility).
 * 
 * @param queue Queue to destroy (can be NULL)
 */
void lle_event_queue_destroy(
    lle_event_queue_t *queue
);

/**
 * @brief Enqueue event
 * 
 * Adds event to the event queue. Thread-safe.
 * 
 * @param system Event system
 * @param event Event to enqueue (system takes ownership)
 * @return LLE_SUCCESS on success
 * @return LLE_ERROR_QUEUE_FULL if queue is full
 * @return Other error codes on failure
 * 
 * @note Caller should not use event after successful enqueue
 */
lle_result_t lle_event_enqueue(
    lle_event_system_t *system,
    lle_event_t *event
);

/**
 * @brief Dequeue event
 * 
 * Removes and returns next event from queue. Thread-safe.
 * 
 * @param system Event system
 * @param[out] event Pointer to receive dequeued event (NULL if queue empty)
 * @return LLE_SUCCESS on success (event may be NULL if queue empty)
 * @return Error code on failure
 * 
 * @note Caller takes ownership of dequeued event
 */
lle_result_t lle_event_dequeue(
    lle_event_system_t *system,
    lle_event_t **event
);

/* ============================================================================
 * EVENT HANDLER MANAGEMENT
 * ============================================================================
 */

/**
 * @brief Register event handler
 * 
 * Registers a handler function for specific event type.
 * Multiple handlers can be registered for same event type.
 * 
 * @param system Event system
 * @param type Event type to handle
 * @param handler Handler function
 * @param user_data User data passed to handler (can be NULL)
 * @param name Handler name for debugging (can be NULL)
 * @return LLE_SUCCESS on success, error code on failure
 * 
 * @note Handlers called in registration order
 */
lle_result_t lle_event_handler_register(
    lle_event_system_t *system,
    lle_system_event_type_t type,
    lle_event_handler_fn handler,
    void *user_data,
    const char *name
);

/**
 * @brief Unregister specific event handler
 * 
 * Removes handler with matching type and name.
 * 
 * @param system Event system
 * @param type Event type
 * @param name Handler name (must match registration name)
 * @return LLE_SUCCESS on success, error code if not found
 */
lle_result_t lle_event_handler_unregister(
    lle_event_system_t *system,
    lle_system_event_type_t type,
    const char *name
);

/**
 * @brief Unregister all handlers for event type
 * 
 * Removes all handlers registered for specified event type.
 * 
 * @param system Event system
 * @param type Event type
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_event_handler_unregister_all(
    lle_event_system_t *system,
    lle_system_event_type_t type
);

/**
 * @brief Count handlers for event type
 * 
 * Returns number of handlers registered for event type.
 * 
 * @param system Event system
 * @param type Event type
 * @return Number of registered handlers (0 if none)
 */
size_t lle_event_handler_count(
    lle_event_system_t *system,
    lle_system_event_type_t type
);

/* ============================================================================
 * EVENT PROCESSING
 * ============================================================================
 */

/**
 * @brief Dispatch event to handlers
 * 
 * Calls all registered handlers for event's type in registration order.
 * Event is NOT queued - dispatched immediately.
 * 
 * @param system Event system
 * @param event Event to dispatch
 * @return LLE_SUCCESS if all handlers succeeded
 * @return First error code from any handler that failed
 * 
 * @note Event remains caller's responsibility after dispatch
 */
lle_result_t lle_event_dispatch(
    lle_event_system_t *system,
    lle_event_t *event
);

/**
 * @brief Process events from queue
 * 
 * Dequeues and dispatches up to max_events from the queue.
 * Stops early if queue becomes empty.
 * 
 * @param system Event system
 * @param max_events Maximum number of events to process
 * @return LLE_SUCCESS on success, error code on failure
 * 
 * @note Destroys events after successful dispatch
 */
lle_result_t lle_event_process_queue(
    lle_event_system_t *system,
    uint32_t max_events
);

/**
 * @brief Process all queued events
 * 
 * Processes all events currently in queue.
 * Equivalent to lle_event_process_queue(system, UINT32_MAX).
 * 
 * @param system Event system
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_event_process_all(
    lle_event_system_t *system
);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Get current timestamp in microseconds
 * 
 * Returns monotonic timestamp suitable for event timestamps.
 * Uses CLOCK_MONOTONIC for reliability.
 * 
 * @return Timestamp in microseconds
 */
uint64_t lle_event_get_timestamp_us(void);

/**
 * @brief Get human-readable event type name
 * 
 * @param type Event type
 * @return Static string with event type name (do not free)
 */
const char *lle_event_type_name(
    lle_system_event_type_t type
);

#endif /* LLE_EVENT_SYSTEM_H */
