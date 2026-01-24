# Event System Complete Specification

**Document**: 04_event_system_complete.md  
**Version**: 1.0.0  
**Date**: 2025-01-07  
**Status**: Implementation-Ready Specification  
**Classification**: Critical Core Component  

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Event Type Definitions](#3-event-type-definitions)
4. [Event Processing Pipeline](#4-event-processing-pipeline)
5. [Event Queue Management](#5-event-queue-management)
6. [Event Handler System](#6-event-handler-system)
7. [Priority and Filtering](#7-priority-and-filtering)
8. [Event Lifecycle Management](#8-event-lifecycle-management)
9. [Asynchronous Event Processing](#9-asynchronous-event-processing)
10. [State Machine Definitions](#10-state-machine-definitions)
11. [Error Handling and Recovery](#11-error-handling-and-recovery)
12. [Integration with Lush Systems](#12-integration-with-lush-systems)
13. [Performance Requirements](#13-performance-requirements)
14. [Memory Management Integration](#14-memory-management-integration)
15. [Testing and Validation](#15-testing-and-validation)

---

## 1. Executive Summary

### 1.1 Purpose

The Event System provides the foundational event-driven architecture for the Lush Line Editor (LLE), enabling high-performance asynchronous processing of user input, terminal events, and internal system events with priority-based scheduling and comprehensive error recovery.

### 1.2 Key Features

- **High-Performance Event Queue**: Lock-free circular buffer with priority scheduling
- **Asynchronous Processing**: Non-blocking event handling with microsecond precision
- **Extensible Handler System**: Plugin-capable event handler registration and management
- **Priority-Based Processing**: Critical events processed before low-priority events
- **Memory Pool Integration**: Seamless integration with Lush memory management
- **Comprehensive Error Recovery**: Graceful degradation and automatic error handling
- **Performance Monitoring**: Real-time event processing metrics and optimization

### 1.3 Critical Design Principles

1. **Sub-millisecond Response**: All critical events processed within 500 microseconds
2. **Zero Memory Allocation**: Event processing uses pre-allocated memory pools
3. **Priority-Based Scheduling**: System-critical events never blocked by user events
4. **Extensible Architecture**: Plugin events integrated as first-class citizens
5. **Graceful Degradation**: System continues functioning even with handler failures

---

## 2. Architecture Overview

### 2.1 Core Component Structure

```c
// Primary event system components
typedef struct lle_event_system {
    lle_event_queue_t *main_queue;           // Primary event queue
    lle_event_queue_t *priority_queue;       // High-priority event queue
    lle_event_handler_registry_t *handlers;  // Registered event handlers
    lle_event_processor_t *processor;        // Event processing engine
    lle_event_filter_t *filter;              // Event filtering system
    lle_event_stats_t *stats;                // Performance statistics
    lle_memory_pool_t *event_pool;           // Event memory pool
    lle_memory_pool_t *data_pool;            // Event data memory pool
    lle_error_context_t *error_ctx;          // Error handling context
    lle_performance_monitor_t *perf_monitor; // Performance monitoring
    pthread_mutex_t system_mutex;            // System-level mutex
    bool active;                             // System active state
    uint64_t sequence_counter;               // Event sequence counter
} lle_event_system_t;
```

### 2.2 Event System Initialization

```c
// Complete event system initialization with comprehensive error handling
lle_result_t lle_event_system_init(lle_event_system_t **system,
                                   lle_memory_pool_t *memory_pool) {
    lle_result_t result = LLE_SUCCESS;
    lle_event_system_t *sys = NULL;
    
    // Step 1: Validate input parameters
    if (!system || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Step 2: Allocate system structure from memory pool
    sys = lle_memory_pool_alloc(memory_pool, sizeof(lle_event_system_t));
    if (!sys) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    memset(sys, 0, sizeof(lle_event_system_t));
    
    // Step 3: Initialize system mutex first
    if (pthread_mutex_init(&sys->system_mutex, NULL) != 0) {
        lle_memory_pool_free(memory_pool, sys);
        return LLE_ERROR_MUTEX_INIT;
    }
    
    // Step 4: Create dedicated event memory pools
    result = lle_memory_pool_create(&sys->event_pool, 
                                   "LLE_Event_Pool", 
                                   LLE_EVENT_POOL_SIZE,
                                   sizeof(lle_event_t));
    if (result != LLE_SUCCESS) {
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    result = lle_memory_pool_create(&sys->data_pool,
                                   "LLE_EventData_Pool",
                                   LLE_EVENT_DATA_POOL_SIZE,
                                   LLE_MAX_EVENT_DATA_SIZE);
    if (result != LLE_SUCCESS) {
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 5: Initialize error handling context
    result = lle_error_context_init(&sys->error_ctx);
    if (result != LLE_SUCCESS) {
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 6: Initialize performance monitoring
    result = lle_performance_monitor_init(&sys->perf_monitor);
    if (result != LLE_SUCCESS) {
        lle_error_context_destroy(sys->error_ctx);
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 7: Initialize main event queue
    result = lle_event_queue_init(&sys->main_queue, 
                                 LLE_MAIN_QUEUE_SIZE,
                                 LLE_QUEUE_TYPE_CIRCULAR,
                                 sys->event_pool);
    if (result != LLE_SUCCESS) {
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 8: Initialize priority queue
    result = lle_event_queue_init(&sys->priority_queue,
                                 LLE_PRIORITY_QUEUE_SIZE,
                                 LLE_QUEUE_TYPE_PRIORITY,
                                 sys->event_pool);
    if (result != LLE_SUCCESS) {
        lle_event_queue_destroy(sys->main_queue);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 9: Initialize event handler registry
    result = lle_event_handler_registry_init(&sys->handlers, sys->event_pool);
    if (result != LLE_SUCCESS) {
        lle_event_queue_destroy(sys->priority_queue);
        lle_event_queue_destroy(sys->main_queue);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 10: Initialize event processor
    result = lle_event_processor_init(&sys->processor, 
                                     sys->handlers,
                                     sys->error_ctx,
                                     sys->perf_monitor);
    if (result != LLE_SUCCESS) {
        lle_event_handler_registry_destroy(sys->handlers);
        lle_event_queue_destroy(sys->priority_queue);
        lle_event_queue_destroy(sys->main_queue);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 11: Initialize event filter system
    result = lle_event_filter_init(&sys->filter, sys->event_pool);
    if (result != LLE_SUCCESS) {
        lle_event_processor_destroy(sys->processor);
        lle_event_handler_registry_destroy(sys->handlers);
        lle_event_queue_destroy(sys->priority_queue);
        lle_event_queue_destroy(sys->main_queue);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 12: Initialize performance statistics
    result = lle_event_stats_init(&sys->stats, sys->event_pool);
    if (result != LLE_SUCCESS) {
        lle_event_filter_destroy(sys->filter);
        lle_event_processor_destroy(sys->processor);
        lle_event_handler_registry_destroy(sys->handlers);
        lle_event_queue_destroy(sys->priority_queue);
        lle_event_queue_destroy(sys->main_queue);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    // Step 13: Initialize sequence counter and activate system
    sys->sequence_counter = 1;
    sys->active = true;
    
    // Step 14: Register core system event handlers
    result = lle_event_system_register_core_handlers(sys);
    if (result != LLE_SUCCESS) {
        lle_event_stats_destroy(sys->stats);
        lle_event_filter_destroy(sys->filter);
        lle_event_processor_destroy(sys->processor);
        lle_event_handler_registry_destroy(sys->handlers);
        lle_event_queue_destroy(sys->priority_queue);
        lle_event_queue_destroy(sys->main_queue);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        lle_memory_pool_destroy(sys->data_pool);
        lle_memory_pool_destroy(sys->event_pool);
        pthread_mutex_destroy(&sys->system_mutex);
        lle_memory_pool_free(memory_pool, sys);
        return result;
    }
    
    *system = sys;
    return LLE_SUCCESS;
}
```

---

## 3. Event Type Definitions

### 3.1 Core Event Types

```c
// Comprehensive event type enumeration
typedef enum {
    // Terminal Input Events (Priority: HIGH)
    LLE_EVENT_KEY_PRESS = 0x1000,           // Single key press
    LLE_EVENT_KEY_RELEASE,                  // Key release (if supported)
    LLE_EVENT_KEY_SEQUENCE,                 // Multi-key sequence (e.g., escape sequences)
    LLE_EVENT_KEY_CHORD,                    // Simultaneous key combination
    
    // Mouse Events (Priority: MEDIUM)
    LLE_EVENT_MOUSE_PRESS = 0x2000,         // Mouse button press
    LLE_EVENT_MOUSE_RELEASE,                // Mouse button release
    LLE_EVENT_MOUSE_MOVE,                   // Mouse movement
    LLE_EVENT_MOUSE_WHEEL,                  // Mouse wheel scroll
    LLE_EVENT_MOUSE_DRAG,                   // Mouse drag operation
    
    // Terminal State Events (Priority: CRITICAL)
    LLE_EVENT_TERMINAL_RESIZE = 0x3000,     // Terminal size change
    LLE_EVENT_FOCUS_IN,                     // Terminal gained focus
    LLE_EVENT_FOCUS_OUT,                    // Terminal lost focus
    LLE_EVENT_TERMINAL_DISCONNECT,          // Terminal disconnected
    
    // Paste Events (Priority: HIGH)
    LLE_EVENT_PASTE_START = 0x4000,         // Bracketed paste start
    LLE_EVENT_PASTE_DATA,                   // Paste data chunk
    LLE_EVENT_PASTE_END,                    // Bracketed paste end
    LLE_EVENT_PASTE_TIMEOUT,                // Paste operation timeout
    
    // Buffer Events (Priority: MEDIUM)
    LLE_EVENT_BUFFER_CHANGED = 0x5000,      // Buffer content changed
    LLE_EVENT_CURSOR_MOVED,                 // Cursor position changed
    LLE_EVENT_SELECTION_CHANGED,            // Text selection changed
    LLE_EVENT_BUFFER_VALIDATION,            // Buffer validation request
    
    // History Events (Priority: LOW)
    LLE_EVENT_HISTORY_CHANGED = 0x6000,     // History state changed
    LLE_EVENT_HISTORY_SEARCH,               // History search request
    LLE_EVENT_HISTORY_NAVIGATE,             // History navigation
    
    // Completion Events (Priority: MEDIUM)
    LLE_EVENT_COMPLETION_REQUESTED = 0x7000, // Tab completion requested
    LLE_EVENT_COMPLETION_UPDATED,           // Completion list updated
    LLE_EVENT_COMPLETION_ACCEPTED,          // Completion accepted
    LLE_EVENT_COMPLETION_CANCELLED,         // Completion cancelled
    
    // Suggestion Events (Priority: LOW)
    LLE_EVENT_SUGGESTION_UPDATED = 0x8000,  // Autosuggestion updated
    LLE_EVENT_SUGGESTION_ACCEPTED,          // Suggestion accepted
    LLE_EVENT_SUGGESTION_DISMISSED,         // Suggestion dismissed
    
    // System Events (Priority: CRITICAL)
    LLE_EVENT_SYSTEM_ERROR = 0x9000,        // System error occurred
    LLE_EVENT_SYSTEM_WARNING,               // System warning
    LLE_EVENT_SYSTEM_SHUTDOWN,              // System shutdown request
    LLE_EVENT_MEMORY_PRESSURE,              // Memory pressure warning
    
    // Timer Events (Priority: LOW)
    LLE_EVENT_TIMER_EXPIRED = 0xA000,       // Timer expired
    LLE_EVENT_TIMEOUT,                      // General timeout
    LLE_EVENT_PERIODIC_UPDATE,              // Periodic update trigger
    
    // Plugin Events (Priority: CONFIGURABLE)
    LLE_EVENT_PLUGIN_CUSTOM = 0xB000,       // Plugin-defined event
    LLE_EVENT_PLUGIN_MESSAGE,               // Inter-plugin message
    LLE_EVENT_PLUGIN_ERROR,                 // Plugin error
    
    // Display Integration Events (Priority: HIGH)
    LLE_EVENT_DISPLAY_UPDATE = 0xC000,      // Display update required
    LLE_EVENT_DISPLAY_REFRESH,              // Display refresh request
    LLE_EVENT_DISPLAY_INVALIDATE,           // Display invalidation
    
    // Testing and Debug Events (Priority: LOWEST)
    LLE_EVENT_DEBUG_MARKER = 0xF000,        // Debug marker event
    LLE_EVENT_TEST_TRIGGER,                 // Test trigger event
    LLE_EVENT_PROFILING_SAMPLE,             // Profiling sample
} lle_event_type_t;

// Event priority levels
typedef enum {
    LLE_PRIORITY_CRITICAL = 0,              // System-critical events
    LLE_PRIORITY_HIGH = 1,                  // High-priority user events
    LLE_PRIORITY_MEDIUM = 2,                // Normal priority events
    LLE_PRIORITY_LOW = 3,                   // Low priority events
    LLE_PRIORITY_LOWEST = 4,                // Lowest priority events
    LLE_PRIORITY_COUNT = 5                  // Total priority levels
} lle_event_priority_t;

// Event source identification
typedef enum {
    LLE_EVENT_SOURCE_TERMINAL,              // Terminal input
    LLE_EVENT_SOURCE_INTERNAL,              // Internal system event
    LLE_EVENT_SOURCE_PLUGIN,                // Plugin-generated event
    LLE_EVENT_SOURCE_TIMER,                 // Timer-generated event
    LLE_EVENT_SOURCE_DISPLAY,               // Display system event
    LLE_EVENT_SOURCE_BUFFER,                // Buffer system event
    LLE_EVENT_SOURCE_HISTORY,               // History system event
    LLE_EVENT_SOURCE_COMPLETION,            // Completion system event
    LLE_EVENT_SOURCE_TEST,                  // Test system event
} lle_event_source_t;

// Event processing flags
typedef enum {
    LLE_EVENT_FLAG_NONE = 0x00,            // No flags
    LLE_EVENT_FLAG_HANDLED = 0x01,         // Event has been handled
    LLE_EVENT_FLAG_CONSUMED = 0x02,        // Event consumed, stop propagation
    LLE_EVENT_FLAG_CANCELLED = 0x04,       // Event cancelled
    LLE_EVENT_FLAG_ASYNC = 0x08,           // Asynchronous processing
    LLE_EVENT_FLAG_URGENT = 0x10,          // Urgent processing required
    LLE_EVENT_FLAG_LOGGED = 0x20,          // Event has been logged
    LLE_EVENT_FLAG_PERSISTENT = 0x40,      // Event should be persisted
    LLE_EVENT_FLAG_SYNTHETIC = 0x80,       // Synthetically generated event
} lle_event_flags_t;
```

### 3.2 Event Data Structures

```c
// Key event data
typedef struct {
    uint32_t keycode;                      // Key code
    uint32_t modifiers;                    // Modifier keys (shift, ctrl, alt, etc.)
    char utf8_char[8];                     // UTF-8 character representation
    bool repeat;                           // Key repeat event
    uint16_t repeat_count;                 // Repeat count
    uint64_t key_timestamp;                // Key press timestamp
} lle_key_event_data_t;

// Mouse event data
typedef struct {
    uint16_t x, y;                         // Mouse coordinates
    uint8_t button;                        // Mouse button (left, right, middle)
    uint32_t modifiers;                    // Modifier keys
    int16_t wheel_delta;                   // Wheel scroll delta
    bool double_click;                     // Double-click detection
    uint64_t click_timestamp;              // Click timestamp
} lle_mouse_event_data_t;

// Terminal resize event data
typedef struct {
    uint16_t old_width, old_height;        // Previous dimensions
    uint16_t new_width, new_height;        // New dimensions
    bool size_changed;                     // Size actually changed
    bool position_changed;                 // Position changed
    uint64_t resize_timestamp;             // Resize timestamp
} lle_resize_event_data_t;

// Focus event data
typedef struct {
    bool gained_focus;                     // True if gained, false if lost
    uint64_t focus_timestamp;              // Focus change timestamp
    char window_id[64];                    // Window identifier (if available)
} lle_focus_event_data_t;

// Paste event data
typedef struct {
    char *data;                            // Paste data
    size_t data_len;                       // Data length
    size_t data_capacity;                  // Data buffer capacity
    bool is_complete;                      // Paste operation complete
    bool bracketed;                        // Bracketed paste mode
    uint64_t paste_timestamp;              // Paste timestamp
} lle_paste_event_data_t;

// Buffer change event data
typedef struct {
    size_t start_pos;                      // Change start position
    size_t end_pos;                        // Change end position
    size_t old_length;                     // Old buffer length
    size_t new_length;                     // New buffer length
    lle_change_type_t change_type;         // Type of change
    char *old_text;                        // Previous text (for undo)
    char *new_text;                        // New text
    uint64_t change_timestamp;             // Change timestamp
} lle_buffer_event_data_t;

// Error event data
typedef struct {
    lle_error_code_t error_code;           // Error code
    char error_message[256];               // Error message
    char context[128];                     // Error context
    void *error_data;                      // Additional error data
    size_t error_data_size;                // Error data size
    uint64_t error_timestamp;              // Error timestamp
    bool recoverable;                      // Error is recoverable
} lle_error_event_data_t;

// Custom/plugin event data
typedef struct {
    char event_name[64];                   // Custom event name
    void *data;                            // Custom data
    size_t data_size;                      // Data size
    char plugin_id[64];                    // Plugin identifier
    uint32_t custom_type;                  // Plugin-specific type
    lle_plugin_callback_t callback;        // Optional callback
    uint64_t custom_timestamp;             // Custom timestamp
} lle_custom_event_data_t;
```

### 3.3 Main Event Structure

```c
// Complete event structure
typedef struct lle_event {
    // Core event identification
    lle_event_type_t type;                 // Event type
    uint64_t sequence_number;              // Global sequence number
    uint64_t timestamp;                    // Event timestamp (microseconds)
    lle_event_source_t source;             // Event source
    lle_event_priority_t priority;         // Event priority
    
    // Event processing state
    lle_event_flags_t flags;               // Processing flags
    uint32_t handler_count;                // Number of handlers processed
    uint64_t processing_start_time;        // Processing start timestamp
    uint64_t processing_end_time;          // Processing end timestamp
    
    // Event context
    void *context;                         // Event context pointer
    lle_editor_t *editor;                  // Editor instance
    lle_buffer_t *buffer;                  // Associated buffer
    
    // Event data (union for memory efficiency)
    union {
        lle_key_event_data_t key;          // Key event data
        lle_mouse_event_data_t mouse;      // Mouse event data
        lle_resize_event_data_t resize;    // Resize event data
        lle_focus_event_data_t focus;      // Focus event data
        lle_paste_event_data_t paste;      // Paste event data
        lle_buffer_event_data_t buffer;    // Buffer event data
        lle_error_event_data_t error;      // Error event data
        lle_custom_event_data_t custom;    // Custom event data
        uint8_t raw_data[256];             // Raw data buffer
    } data;
    
    // Memory management
    lle_memory_pool_t *memory_pool;        // Memory pool for event
    struct lle_event *next;                // Next event in queue/chain
    struct lle_event *prev;                // Previous event in chain
} lle_event_t;
```

---

## 4. Event Processing Pipeline

### 4.1 Event Processing State Machine

```c
// Event processing states
typedef enum {
    LLE_EVENT_STATE_CREATED,               // Event created, not queued
    LLE_EVENT_STATE_QUEUED,                // Event queued for processing
    LLE_EVENT_STATE_FILTERING,             // Event being filtered
    LLE_EVENT_STATE_PROCESSING,            // Event being processed
    LLE_EVENT_STATE_HANDLED,               // Event handled successfully
    LLE_EVENT_STATE_ERROR,                 // Error during processing
    LLE_EVENT_STATE_CANCELLED,             // Event cancelled
    LLE_EVENT_STATE_COMPLETED,             // Event processing completed
    LLE_EVENT_STATE_DESTROYED,             // Event destroyed/freed
} lle_event_processing_state_t;

// Event processor structure
typedef struct {
    lle_event_handler_registry_t *handlers; // Handler registry
    lle_event_filter_t *filter;            // Event filter
    lle_error_context_t *error_ctx;        // Error context
    lle_performance_monitor_t *perf_monitor; // Performance monitor
    lle_event_stats_t *stats;              // Processing statistics
    
    // Processing control
    bool processing_active;                // Processing is active
    uint32_t max_events_per_cycle;         // Maximum events per processing cycle
    uint64_t cycle_time_limit_us;          // Time limit per cycle (microseconds)
    
    // Current processing state
    lle_event_t *current_event;            // Currently processing event
    uint32_t handlers_processed;           // Handlers processed for current event
    uint64_t cycle_start_time;             // Current cycle start time
    uint32_t events_processed_this_cycle;  // Events processed in current cycle
    
    // Error handling
    lle_event_t *error_event;              // Event that caused error
    uint32_t consecutive_errors;           // Consecutive error count
    uint64_t last_error_time;              // Last error timestamp
} lle_event_processor_t;
```

### 4.2 Main Event Processing Loop

```c
// Primary event processing function
lle_result_t lle_event_processor_process_events(lle_event_processor_t *processor,
                                               lle_event_queue_t *priority_queue,
                                               lle_event_queue_t *main_queue,
                                               uint32_t max_events) {
    if (!processor || !priority_queue || !main_queue) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    uint32_t events_processed = 0;
    uint64_t cycle_start = lle_get_timestamp_us();
    
    processor->cycle_start_time = cycle_start;
    processor->events_processed_this_cycle = 0;
    
    // Process priority queue first (critical events)
    while (events_processed < max_events && 
           processor->cycle_start_time + processor->cycle_time_limit_us > lle_get_timestamp_us()) {
        
        lle_event_t *event = NULL;
        result = lle_event_queue_dequeue(priority_queue, &event);
        
        if (result == LLE_SUCCESS && event) {
            result = lle_event_processor_process_single_event(processor, event);
            events_processed++;
            processor->events_processed_this_cycle++;
            
            // Handle processing errors
            if (result != LLE_SUCCESS) {
                lle_event_processor_handle_processing_error(processor, event, result);
                
                // If too many consecutive errors, stop processing
                if (processor->consecutive_errors >= LLE_MAX_CONSECUTIVE_ERRORS) {
                    lle_error_context_record_error(processor->error_ctx,
                                                  LLE_ERROR_TOO_MANY_EVENT_ERRORS,
                                                  "Too many consecutive event processing errors");
                    return LLE_ERROR_TOO_MANY_EVENT_ERRORS;
                }
            } else {
                processor->consecutive_errors = 0; // Reset error counter on success
            }
        } else if (result != LLE_ERROR_QUEUE_EMPTY) {
            // Actual error occurred (not just empty queue)
            return result;
        } else {
            // Priority queue empty, break to process main queue
            break;
        }
    }
    
    // Process main queue (normal priority events)
    while (events_processed < max_events &&
           processor->cycle_start_time + processor->cycle_time_limit_us > lle_get_timestamp_us()) {
        
        lle_event_t *event = NULL;
        result = lle_event_queue_dequeue(main_queue, &event);
        
        if (result == LLE_SUCCESS && event) {
            result = lle_event_processor_process_single_event(processor, event);
            events_processed++;
            processor->events_processed_this_cycle++;
            
            // Handle processing errors
            if (result != LLE_SUCCESS) {
                lle_event_processor_handle_processing_error(processor, event, result);
                
                if (processor->consecutive_errors >= LLE_MAX_CONSECUTIVE_ERRORS) {
                    lle_error_context_record_error(processor->error_ctx,
                                                  LLE_ERROR_TOO_MANY_EVENT_ERRORS,
                                                  "Too many consecutive event processing errors");
                    return LLE_ERROR_TOO_MANY_EVENT_ERRORS;
                }
            } else {
                processor->consecutive_errors = 0;
            }
        } else if (result != LLE_ERROR_QUEUE_EMPTY) {
            return result;
        } else {
            // Main queue empty, processing complete
            break;
        }
    }
    
    // Update processing statistics
    uint64_t cycle_end = lle_get_timestamp_us();
    uint64_t cycle_duration = cycle_end - cycle_start;
    
    lle_event_stats_update_cycle_stats(processor->stats,
                                      events_processed,
                                      cycle_duration);
    
    return LLE_SUCCESS;
}
```

### 4.3 Single Event Processing

```c
// Process a single event through the handler pipeline
lle_result_t lle_event_processor_process_single_event(lle_event_processor_t *processor,
                                                     lle_event_t *event) {
    if (!processor || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    uint64_t processing_start = lle_get_timestamp_us();
    
    // Set processing state
    processor->current_event = event;
    processor->handlers_processed = 0;
    event->processing_start_time = processing_start;
    event->flags |= LLE_EVENT_FLAG_HANDLED;
    
    // Step 1: Apply event filters
    result = lle_event_filter_apply(processor->filter, event);
    if (result != LLE_SUCCESS) {
        event->flags |= LLE_EVENT_FLAG_CANCELLED;
        goto cleanup;
    }
    
    // Step 2: Check if event was filtered out
    if (event->flags & LLE_EVENT_FLAG_CANCELLED) {
        result = LLE_SUCCESS; // Filtering is not an error
        goto cleanup;
    }
    
    // Step 3: Get handlers for this event type
    lle_event_handler_list_t *handlers = NULL;
    result = lle_event_handler_registry_get_handlers(processor->handlers,
                                                    event->type,
                                                    &handlers);
    if (result != LLE_SUCCESS) {
        goto cleanup;
    }
    
    // Step 4: Process each handler in priority order
    for (uint32_t i = 0; i < handlers->count && !(event->flags & LLE_EVENT_FLAG_CONSUMED); i++) {
        lle_event_handler_t *handler = &handlers->handlers[i];
        
        // Check handler prerequisites
        if (!lle_event_handler_can_process(handler, event)) {
            continue;
        }
        
        // Record handler processing start
        uint64_t handler_start = lle_get_timestamp_us();
        
        // Call the handler
        lle_result_t handler_result = handler->callback(event, handler->user_data);
        processor->handlers_processed++;
        
        // Record handler processing time
        uint64_t handler_end = lle_get_timestamp_us();
        uint64_t handler_duration = handler_end - handler_start;
        
        // Update handler statistics
        lle_event_stats_update_handler_stats(processor->stats,
                                            handler->name,
                                            handler_duration,
                                            handler_result);
        
        // Handle handler results
        if (handler_result == LLE_HANDLER_RESULT_CONSUMED) {
            event->flags |= LLE_EVENT_FLAG_CONSUMED;
        } else if (handler_result == LLE_HANDLER_RESULT_ERROR) {
            lle_error_context_record_error(processor->error_ctx,
                                          LLE_ERROR_HANDLER_FAILED,
                                          "Event handler failed: %s", handler->name);
            // Continue processing other handlers unless critical error
            if (handler->flags & LLE_HANDLER_FLAG_CRITICAL) {
                result = LLE_ERROR_HANDLER_FAILED;
                goto cleanup;
            }
        } else if (handler_result == LLE_HANDLER_RESULT_CANCEL) {
            event->flags |= LLE_EVENT_FLAG_CANCELLED;
            break;
        }
        
        // Check processing time limits
        uint64_t current_time = lle_get_timestamp_us();
        if (current_time - processing_start > LLE_MAX_EVENT_PROCESSING_TIME_US) {
            lle_error_context_record_error(processor->error_ctx,
                                          LLE_ERROR_EVENT_PROCESSING_TIMEOUT,
                                          "Event processing timeout exceeded");
            result = LLE_ERROR_EVENT_PROCESSING_TIMEOUT;
            goto cleanup;
        }
    }
    
cleanup:
    // Record processing end time
    event->processing_end_time = lle_get_timestamp_us();
    
    // Update event statistics
    uint64_t total_processing_time = event->processing_end_time - event->processing_start_time;
    lle_event_stats_update_event_stats(processor->stats,
                                      event->type,
                                      total_processing_time,
                                      processor->handlers_processed);
    
    // Clean up event resources
    lle_event_destroy(event);
    
    processor->current_event = NULL;
    processor->handlers_processed = 0;
    
    return result;
}
```

---

## 5. Event Queue Management

### 5.1 Event Queue Structure

```c
// Event queue types
typedef enum {
    LLE_QUEUE_TYPE_CIRCULAR,               // Circular buffer queue
    LLE_QUEUE_TYPE_PRIORITY,               // Priority-based queue
    LLE_QUEUE_TYPE_DYNAMIC,                // Dynamic resizing queue
} lle_queue_type_t;

// Event queue structure
typedef struct {
    lle_event_t **events;                  // Event array
    uint32_t capacity;                     // Queue capacity
    uint32_t size;                         // Current size
    uint32_t head;                         // Queue head position
    uint32_t tail;                         // Queue tail position
    lle_queue_type_t type;                 // Queue type
    lle_memory_pool_t *memory_pool;        // Memory pool for events
    
    // Thread synchronization
    pthread_mutex_t queue_mutex;           // Queue mutex
    pthread_cond_t not_empty;              // Not empty condition
    pthread_cond_t not_full;               // Not full condition
    
    // Queue statistics
    uint64_t enqueue_count;                // Total enqueues
    uint64_t dequeue_count;                // Total dequeues
    uint64_t drop_count;                   // Dropped events (overflow)
    uint32_t max_size_reached;             // Maximum size reached
    
    // Performance monitoring
    uint64_t total_wait_time;              // Total wait time
    uint64_t max_wait_time;                // Maximum wait time
    uint32_t current_waiters;              // Current waiting threads
} lle_event_queue_t;
```

### 5.2 Queue Operations

```c
// Enqueue event with priority handling
lle_result_t lle_event_queue_enqueue(lle_event_queue_t *queue,
                                    lle_event_t *event,
                                    bool block_on_full) {
    if (!queue || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    uint64_t wait_start = 0;
    
    // Lock queue
    if (pthread_mutex_lock(&queue->queue_mutex) != 0) {
        return LLE_ERROR_MUTEX_LOCK;
    }
    
    // Check if queue is full
    while (queue->size >= queue->capacity) {
        if (!block_on_full) {
            // Drop event if not blocking
            queue->drop_count++;
            result = LLE_ERROR_QUEUE_FULL;
            goto unlock_and_return;
        }
        
        // Wait for space if blocking
        wait_start = lle_get_timestamp_us();
        queue->current_waiters++;
        
        if (pthread_cond_wait(&queue->not_full, &queue->queue_mutex) != 0) {
            queue->current_waiters--;
            result = LLE_ERROR_CONDITION_WAIT;
            goto unlock_and_return;
        }
        
        queue->current_waiters--;
        
        // Update wait statistics
        uint64_t wait_time = lle_get_timestamp_us() - wait_start;
        queue->total_wait_time += wait_time;
        if (wait_time > queue->max_wait_time) {
            queue->max_wait_time = wait_time;
        }
    }
    
    // Insert event based on queue type
    if (queue->type == LLE_QUEUE_TYPE_PRIORITY) {
        result = lle_event_queue_insert_priority(queue, event);
    } else {
        result = lle_event_queue_insert_fifo(queue, event);
    }
    
    if (result == LLE_SUCCESS) {
        queue->size++;
        queue->enqueue_count++;
        
        if (queue->size > queue->max_size_reached) {
            queue->max_size_reached = queue->size;
        }
        
        // Signal waiting dequeuers
        pthread_cond_signal(&queue->not_empty);
    }
    
unlock_and_return:
    pthread_mutex_unlock(&queue->queue_mutex);
    return result;
}

// Dequeue event
lle_result_t lle_event_queue_dequeue(lle_event_queue_t *queue,
                                    lle_event_t **event) {
    if (!queue || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    *event = NULL;
    
    // Lock queue
    if (pthread_mutex_lock(&queue->queue_mutex) != 0) {
        return LLE_ERROR_MUTEX_LOCK;
    }
    
    // Check if queue is empty
    if (queue->size == 0) {
        result = LLE_ERROR_QUEUE_EMPTY;
        goto unlock_and_return;
    }
    
    // Remove event from head
    *event = queue->events[queue->head];
    queue->events[queue->head] = NULL;
    
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    queue->dequeue_count++;
    
    // Signal waiting enqueuers
    pthread_cond_signal(&queue->not_full);
    
unlock_and_return:
    pthread_mutex_unlock(&queue->queue_mutex);
    return result;
}
```

---

## 6. Event Handler System

### 6.1 Handler Structure

```c
// Event handler callback types
typedef enum {
    LLE_HANDLER_RESULT_CONTINUE,           // Continue processing other handlers
    LLE_HANDLER_RESULT_CONSUMED,           // Event consumed, stop processing
    LLE_HANDLER_RESULT_ERROR,              // Handler error occurred
    LLE_HANDLER_RESULT_CANCEL,             // Cancel event processing
} lle_handler_result_t;

// Event handler flags
typedef enum {
    LLE_HANDLER_FLAG_NONE = 0x00,          // No special flags
    LLE_HANDLER_FLAG_CRITICAL = 0x01,      // Critical handler (errors stop processing)
    LLE_HANDLER_FLAG_ASYNC = 0x02,         // Asynchronous handler
    LLE_HANDLER_FLAG_ONCE = 0x04,          // One-time handler
    LLE_HANDLER_FLAG_SYSTEM = 0x08,        // System handler (high priority)
    LLE_HANDLER_FLAG_PLUGIN = 0x10,        // Plugin handler
} lle_handler_flags_t;

// Event handler callback function
typedef lle_handler_result_t (*lle_event_handler_callback_t)(lle_event_t *event,
                                                           void *user_data);

// Event handler structure
typedef struct {
    char name[64];                         // Handler name
    lle_event_type_t event_type;           // Event type to handle
    lle_event_handler_callback_t callback; // Handler callback function
    void *user_data;                       // User data pointer
    uint32_t priority;                     // Handler priority (0 = highest)
    lle_handler_flags_t flags;             // Handler flags
    
    // Handler statistics
    uint64_t call_count;                   // Number of times called
    uint64_t total_processing_time;        // Total processing time
    uint64_t max_processing_time;          // Maximum processing time
    uint32_t error_count;                  // Number of errors
    uint64_t last_called_time;             // Last called timestamp
    
    // Handler lifecycle
    bool enabled;                          // Handler enabled
    uint64_t registration_time;            // Registration timestamp
    lle_plugin_t *owner_plugin;            // Owning plugin (if any)
} lle_event_handler_t;

// Handler list for a specific event type
typedef struct {
    lle_event_handler_t *handlers;         // Handler array
    uint32_t count;                        // Number of handlers
    uint32_t capacity;                     // Array capacity
    bool sorted;                           // Handlers sorted by priority
    uint64_t last_modified;                // Last modification time
} lle_event_handler_list_t;

// Handler registry structure
typedef struct {
    lle_hashtable_t *handler_map;          // Event type -> handler list mapping
    lle_memory_pool_t *memory_pool;        // Memory pool for handlers
    pthread_rwlock_t registry_lock;        // Reader-writer lock
    uint32_t total_handlers;               // Total registered handlers
    uint64_t modification_count;           // Registry modification counter
} lle_event_handler_registry_t;
```

### 6.2 Handler Registration

```c
// Register event handler
lle_result_t lle_event_handler_registry_register(lle_event_handler_registry_t *registry,
                                                const char *name,
                                                lle_event_type_t event_type,
                                                lle_event_handler_callback_t callback,
                                                void *user_data,
                                                uint32_t priority,
                                                lle_handler_flags_t flags,
                                                uint32_t *handler_id) {
    if (!registry || !name || !callback || !handler_id) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    lle_event_handler_list_t *handler_list = NULL;
    
    // Acquire write lock
    if (pthread_rwlock_wrlock(&registry->registry_lock) != 0) {
        return LLE_ERROR_RWLOCK_LOCK;
    }
    
    // Get or create handler list for this event type
    result = lle_hashtable_get(registry->handler_map, &event_type, sizeof(event_type),
                              (void**)&handler_list);
    
    if (result == LLE_ERROR_KEY_NOT_FOUND) {
        // Create new handler list
        handler_list = lle_memory_pool_alloc(registry->memory_pool,
                                           sizeof(lle_event_handler_list_t));
        if (!handler_list) {
            result = LLE_ERROR_MEMORY_ALLOCATION;
            goto unlock_and_return;
        }
        
        memset(handler_list, 0, sizeof(lle_event_handler_list_t));
        handler_list->capacity = LLE_INITIAL_HANDLER_CAPACITY;
        handler_list->handlers = lle_memory_pool_alloc(registry->memory_pool,
                                                      handler_list->capacity * sizeof(lle_event_handler_t));
        
        if (!handler_list->handlers) {
            lle_memory_pool_free(registry->memory_pool, handler_list);
            result = LLE_ERROR_MEMORY_ALLOCATION;
            goto unlock_and_return;
        }
        
        // Add to registry
        result = lle_hashtable_put(registry->handler_map, &event_type, sizeof(event_type),
                                  handler_list, sizeof(lle_event_handler_list_t*));
        if (result != LLE_SUCCESS) {
            lle_memory_pool_free(registry->memory_pool, handler_list->handlers);
            lle_memory_pool_free(registry->memory_pool, handler_list);
            goto unlock_and_return;
        }
    } else if (result != LLE_SUCCESS) {
        goto unlock_and_return;
    }
    
    // Check if handler list needs expansion
    if (handler_list->count >= handler_list->capacity) {
        uint32_t new_capacity = handler_list->capacity * 2;
        lle_event_handler_t *new_handlers = lle_memory_pool_alloc(registry->memory_pool,
                                                                 new_capacity * sizeof(lle_event_handler_t));
        if (!new_handlers) {
            result = LLE_ERROR_MEMORY_ALLOCATION;
            goto unlock_and_return;
        }
        
        memcpy(new_handlers, handler_list->handlers,
               handler_list->count * sizeof(lle_event_handler_t));
        
        lle_memory_pool_free(registry->memory_pool, handler_list->handlers);
        handler_list->handlers = new_handlers;
        handler_list->capacity = new_capacity;
    }
    
    // Initialize new handler
    lle_event_handler_t *handler = &handler_list->handlers[handler_list->count];
    memset(handler, 0, sizeof(lle_event_handler_t));
    
    strncpy(handler->name, name, sizeof(handler->name) - 1);
    handler->name[sizeof(handler->name) - 1] = '\0';
    handler->event_type = event_type;
    handler->callback = callback;
    handler->user_data = user_data;
    handler->priority = priority;
    handler->flags = flags;
    handler->enabled = true;
    handler->registration_time = lle_get_timestamp_us();
    
    *handler_id = handler_list->count;
    handler_list->count++;
    handler_list->sorted = false; // Mark as needing re-sort
    handler_list->last_modified = lle_get_timestamp_us();
    
    registry->total_handlers++;
    registry->modification_count++;
    
unlock_and_return:
    pthread_rwlock_unlock(&registry->registry_lock);
    return result;
}
```

---

## 7. Priority and Filtering

### 7.1 Event Filter System

```c
// Event filter types
typedef enum {
    LLE_FILTER_TYPE_ALLOW,                 // Allow specific events
    LLE_FILTER_TYPE_BLOCK,                 // Block specific events
    LLE_FILTER_TYPE_TRANSFORM,             // Transform events
    LLE_FILTER_TYPE_RATE_LIMIT,            // Rate limiting
    LLE_FILTER_TYPE_DUPLICATE,             // Duplicate detection
} lle_filter_type_t;

// Event filter structure
typedef struct {
    char name[64];                         // Filter name
    lle_filter_type_t type;                // Filter type
    lle_event_type_t event_type;           // Target event type
    uint32_t priority;                     // Filter priority
    bool enabled;                          // Filter enabled
    
    // Filter callback
    lle_result_t (*filter_callback)(lle_event_t *event, void *user_data);
    void *user_data;                       // User data
    
    // Rate limiting (if applicable)
    uint32_t max_events_per_second;        // Maximum events per second
    uint32_t current_count;                // Current event count
    uint64_t window_start;                 // Rate limiting window start
    
    // Duplicate detection (if applicable)
    lle_event_t *last_event;               // Last processed event
    uint64_t duplicate_threshold_us;       // Duplicate detection threshold
    
    // Filter statistics
    uint64_t events_processed;             // Events processed
    uint64_t events_filtered;              // Events filtered out
    uint64_t events_transformed;           // Events transformed
} lle_event_filter_t;
```

---

## 8. Integration with Lush Systems

### 8.1 Display System Integration

```c
// Integration with Lush layered display system
lle_result_t lle_event_system_integrate_display(lle_event_system_t *event_system,
                                               lle_display_controller_t *display) {
    if (!event_system || !display) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Register display update handlers
    lle_result_t result = LLE_SUCCESS;
    uint32_t handler_id;
    
    // Buffer change events trigger display updates
    result = lle_event_handler_registry_register(event_system->handlers,
                                               "display_buffer_change",
                                               LLE_EVENT_BUFFER_CHANGED,
                                               lle_event_handler_buffer_display_update,
                                               display,
                                               LLE_PRIORITY_HIGH,
                                               LLE_HANDLER_FLAG_SYSTEM,
                                               &handler_id);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Cursor movement events
    result = lle_event_handler_registry_register(event_system->handlers,
                                               "display_cursor_move",
                                               LLE_EVENT_CURSOR_MOVED,
                                               lle_event_handler_cursor_display_update,
                                               display,
                                               LLE_PRIORITY_HIGH,
                                               LLE_HANDLER_FLAG_SYSTEM,
                                               &handler_id);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Terminal resize events
    result = lle_event_handler_registry_register(event_system->handlers,
                                               "display_resize",
                                               LLE_EVENT_TERMINAL_RESIZE,
                                               lle_event_handler_terminal_resize,
                                               display,
                                               LLE_PRIORITY_CRITICAL,
                                               LLE_HANDLER_FLAG_SYSTEM | LLE_HANDLER_FLAG_CRITICAL,
                                               &handler_id);
    
    return result;
}
```

### 8.2 Memory Pool Integration

```c
// Memory pool integration for event management
typedef struct {
    lle_memory_pool_t *event_pool;         // Pool for event structures
    lle_memory_pool_t *data_pool;          // Pool for event data
    lle_memory_pool_t *handler_pool;       // Pool for handlers
    uint32_t pool_allocation_size;         // Standard allocation size
    uint32_t peak_usage;                   // Peak memory usage
    uint32_t current_usage;                // Current memory usage
} lle_event_memory_manager_t;

// Allocate event from memory pool
lle_event_t* lle_event_allocate(lle_event_memory_manager_t *mem_mgr,
                               lle_event_type_t type) {
    if (!mem_mgr) {
        return NULL;
    }
    
    lle_event_t *event = lle_memory_pool_alloc(mem_mgr->event_pool,
                                              sizeof(lle_event_t));
    if (!event) {
        return NULL;
    }
    
    memset(event, 0, sizeof(lle_event_t));
    event->type = type;
    event->timestamp = lle_get_timestamp_us();
    event->memory_pool = mem_mgr->event_pool;
    
    mem_mgr->current_usage += sizeof(lle_event_t);
    if (mem_mgr->current_usage > mem_mgr->peak_usage) {
        mem_mgr->peak_usage = mem_mgr->current_usage;
    }
    
    return event;
}
```

---

## 9. Performance Requirements

### 9.1 Performance Specifications

```c
// Performance constants
#define LLE_MAX_EVENT_PROCESSING_TIME_US     500     // 500 microseconds max
#define LLE_MAX_HANDLER_PROCESSING_TIME_US   100     // 100 microseconds per handler
#define LLE_MAX_QUEUE_WAIT_TIME_US          1000     // 1 millisecond max wait
#define LLE_TARGET_EVENTS_PER_SECOND       10000     // 10K events/second target
#define LLE_MAX_EVENTS_PER_CYCLE             100     // Max events per processing cycle
#define LLE_CYCLE_TIME_LIMIT_US             2000     // 2 milliseconds per cycle

// Performance monitoring structure
typedef struct {
    uint64_t total_events_processed;       // Total events processed
    uint64_t total_processing_time;        // Total processing time
    uint64_t max_processing_time;          // Maximum processing time
    uint64_t average_processing_time;      // Average processing time
    
    uint32_t events_per_second;            // Current events per second
    uint32_t peak_events_per_second;       // Peak events per second
    
    uint32_t queue_overflows;              // Queue overflow count
    uint32_t processing_timeouts;          // Processing timeout count
    uint32_t handler_errors;               // Handler error count
    
    uint64_t last_performance_check;       // Last performance check time
} lle_event_performance_stats_t;
```

---

## 10. Testing and Validation

### 10.1 Test Framework Integration

```c
// Test event generation
lle_result_t lle_event_system_generate_test_event(lle_event_system_t *system,
                                                 lle_event_type_t type,
                                                 void *test_data) {
    lle_event_t *event = lle_event_allocate(system->event_pool, type);
    if (!event) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    event->source = LLE_EVENT_SOURCE_TEST;
    event->flags = LLE_EVENT_FLAG_SYNTHETIC;
    
    // Copy test data
    if (test_data) {
        memcpy(&event->data, test_data, sizeof(event->data));
    }
    
    return lle_event_queue_enqueue(system->main_queue, event, false);
}

// Performance validation
lle_result_t lle_event_system_validate_performance(lle_event_system_t *system,
                                                  lle_event_performance_stats_t *stats) {
    if (!system || !stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Check processing time requirements
    if (stats->max_processing_time > LLE_MAX_EVENT_PROCESSING_TIME_US) {
        return LLE_ERROR_PERFORMANCE_VIOLATION;
    }
    
    // Check throughput requirements
    if (stats->events_per_second < LLE_TARGET_EVENTS_PER_SECOND) {
        return LLE_ERROR_PERFORMANCE_VIOLATION;
    }
    
    // Check error rates
    double error_rate = (double)stats->handler_errors / stats->total_events_processed;
    if (error_rate > 0.01) { // 1% error rate threshold
        return LLE_ERROR_PERFORMANCE_VIOLATION;
    }
    
    return LLE_SUCCESS;
}
```

---

## 11. Error Handling and Recovery

### 11.1 Comprehensive Error Recovery

```c
// Error recovery procedures
lle_result_t lle_event_system_recover_from_error(lle_event_system_t *system,
                                                lle_error_code_t error_code) {
    switch (error_code) {
        case LLE_ERROR_QUEUE_FULL:
            // Increase queue capacity or enable event dropping
            return lle_event_system_expand_queues(system);
            
        case LLE_ERROR_HANDLER_FAILED:
            // Disable failing handler temporarily
            return lle_event_system_disable_failing_handlers(system);
            
        case LLE_ERROR_MEMORY_ALLOCATION:
            // Trigger garbage collection and retry
            return lle_event_system_garbage_collect(system);
            
        case LLE_ERROR_EVENT_PROCESSING_TIMEOUT:
            // Reduce processing time limits
            return lle_event_system_adjust_timing(system);
            
        default:
            return LLE_ERROR_RECOVERY_FAILED;
    }
}
```

This completes the comprehensive Event System specification with implementation-ready pseudo-code, complete error handling, state machine definitions, integration points with Lush systems, performance requirements, and testing procedures.