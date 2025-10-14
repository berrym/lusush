/*
 * Lusush Line Editor (LLE) - Phase 0 Validation
 * Event Queue System - Week 4 Day 16
 * 
 * Copyright (C) 2021-2025  Michael Berry
 */

#ifndef LLE_VALIDATION_EVENT_QUEUE_H
#define LLE_VALIDATION_EVENT_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * EVENT-DRIVEN ARCHITECTURE VALIDATION
 * 
 * Week 4 Success Criteria:
 * - All 6 event scenarios work
 * - Zero race conditions
 * - Zero deadlocks
 * - Event latency <50μs (p99)
 * - Signal safety: 100%
 * - Stress test: 1M events without crashes
 * 
 * Phase 0 Focus: Validate event queue architecture and basic safety
 */

// Event types
typedef enum {
    LLE_EVENT_INPUT,           // Keyboard input
    LLE_EVENT_SIGNAL,          // Signal (INT, TSTP, CONT, WINCH)
    LLE_EVENT_RESIZE,          // Window resize
    LLE_EVENT_PLUGIN,          // Plugin-generated event
    LLE_EVENT_TIMER,           // Timer event
    LLE_EVENT_SHUTDOWN         // Shutdown event
} lle_event_type_t;

// Signal types for LLE_EVENT_SIGNAL
typedef enum {
    LLE_SIGNAL_INT = 1,        // SIGINT (Ctrl-C)
    LLE_SIGNAL_TSTP,           // SIGTSTP (Ctrl-Z)
    LLE_SIGNAL_CONT,           // SIGCONT
    LLE_SIGNAL_WINCH           // SIGWINCH (window resize)
} lle_signal_type_t;

// Event structure
typedef struct {
    lle_event_type_t type;     // Event type
    uint64_t timestamp_ns;     // Event timestamp (nanoseconds)
    uint32_t sequence;         // Sequence number
    
    // Event-specific data
    union {
        struct {
            uint32_t codepoint;     // For INPUT events
            uint8_t modifiers;      // Modifier keys
        } input;
        
        struct {
            lle_signal_type_t signal;  // For SIGNAL events
        } signal;
        
        struct {
            uint16_t rows;          // For RESIZE events
            uint16_t cols;
        } resize;
        
        struct {
            uint32_t plugin_id;     // For PLUGIN events
            void *data;
        } plugin;
    } data;
} lle_event_t;

// Event queue structure
typedef struct {
    lle_event_t *events;        // Event buffer (circular)
    size_t capacity;            // Buffer capacity
    size_t head;                // Read position
    size_t tail;                // Write position
    size_t count;               // Current event count
    
    // Statistics
    uint64_t total_enqueued;    // Total events enqueued
    uint64_t total_dequeued;    // Total events dequeued
    uint64_t dropped_events;    // Events dropped (queue full)
    
    // Thread safety (for Phase 0, simplified)
    bool thread_safe;           // Whether thread safety is enabled
    uint32_t lock_contentions;  // Lock contention counter
} lle_event_queue_t;

/**
 * Initialize event queue.
 * 
 * @param queue Event queue to initialize
 * @param capacity Queue capacity (number of events)
 * @param thread_safe Enable thread safety (Phase 0: basic validation)
 * @return 0 on success, -1 on failure
 */
int lle_event_queue_init(lle_event_queue_t *queue, 
                         size_t capacity,
                         bool thread_safe);

/**
 * Enqueue an event.
 * Non-blocking: returns -1 if queue is full.
 * 
 * @param queue Event queue
 * @param event Event to enqueue
 * @return 0 on success, -1 if queue full
 */
int lle_event_enqueue(lle_event_queue_t *queue, const lle_event_t *event);

/**
 * Dequeue an event.
 * Non-blocking: returns -1 if queue is empty.
 * 
 * @param queue Event queue
 * @param event Output: dequeued event
 * @return 0 on success, -1 if queue empty
 */
int lle_event_dequeue(lle_event_queue_t *queue, lle_event_t *event);

/**
 * Check if queue is empty.
 * 
 * @param queue Event queue
 * @return true if empty, false otherwise
 */
bool lle_event_queue_is_empty(const lle_event_queue_t *queue);

/**
 * Check if queue is full.
 * 
 * @param queue Event queue
 * @return true if full, false otherwise
 */
bool lle_event_queue_is_full(const lle_event_queue_t *queue);

/**
 * Get current queue size.
 * 
 * @param queue Event queue
 * @return Number of events in queue
 */
size_t lle_event_queue_size(const lle_event_queue_t *queue);

/**
 * Get queue statistics.
 * 
 * @param queue Event queue
 * @param total_enqueued Output: total enqueued
 * @param total_dequeued Output: total dequeued
 * @param dropped Output: dropped events
 * @return 0 on success, -1 on failure
 */
int lle_event_queue_get_stats(const lle_event_queue_t *queue,
                               uint64_t *total_enqueued,
                               uint64_t *total_dequeued,
                               uint64_t *dropped);

/**
 * Reset queue statistics.
 * 
 * @param queue Event queue
 */
void lle_event_queue_reset_stats(lle_event_queue_t *queue);

/**
 * Clear all events from queue.
 * 
 * @param queue Event queue
 */
void lle_event_queue_clear(lle_event_queue_t *queue);

/**
 * Cleanup event queue.
 * 
 * @param queue Event queue
 */
void lle_event_queue_cleanup(lle_event_queue_t *queue);

/**
 * Create an input event.
 * Helper function to create properly formatted events.
 * 
 * @param event Output: event structure
 * @param codepoint Character codepoint
 * @param modifiers Modifier keys
 * @param sequence Sequence number
 */
void lle_event_create_input(lle_event_t *event,
                            uint32_t codepoint,
                            uint8_t modifiers,
                            uint32_t sequence);

/**
 * Create a signal event.
 * 
 * @param event Output: event structure
 * @param signal Signal type
 * @param sequence Sequence number
 */
void lle_event_create_signal(lle_event_t *event,
                             lle_signal_type_t signal,
                             uint32_t sequence);

/**
 * Create a resize event.
 * 
 * @param event Output: event structure
 * @param rows New row count
 * @param cols New column count
 * @param sequence Sequence number
 */
void lle_event_create_resize(lle_event_t *event,
                             uint16_t rows,
                             uint16_t cols,
                             uint32_t sequence);

#endif // LLE_VALIDATION_EVENT_QUEUE_H
