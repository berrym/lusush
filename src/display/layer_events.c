/*
 * Lusush Shell - Layered Display Architecture
 * Layer Events System - Inter-Layer Communication Implementation
 *
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ============================================================================
 *
 * LAYER EVENTS SYSTEM (Layer Communication Protocol)
 *
 * This implementation provides event-driven communication between display
 * layers without creating direct dependencies. It uses a publisher/subscriber
 * pattern with priority-based event processing and performance optimization.
 *
 * Key Responsibilities:
 * - Event queue management and processing
 * - Publisher/subscriber registration and notification
 * - Priority-based event ordering and delivery
 * - Memory-safe event data management
 * - Performance monitoring and optimization
 * - Event debugging and diagnostics
 *
 * Design Principles:
 * - Zero direct layer dependencies
 * - High-performance event processing
 * - Type-safe event handling
 * - Comprehensive error handling
 * - Memory-efficient operations
 * - Thread-safe design (future consideration)
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "layer_events.h"

// ============================================================================
// CONSTANTS AND CONFIGURATION
// ============================================================================

#define MAX_EVENT_TYPE_NAME_LENGTH 64
#define MAX_LAYER_NAME_LENGTH 32
#define EVENT_SYSTEM_MAGIC 0x4C415945 // "LAYE" in hex

// Default configuration values
#define DEFAULT_MAX_QUEUE_SIZE 256
#define DEFAULT_MAX_SUBSCRIBERS 32
#define DEFAULT_PROCESSING_TIMEOUT_MS 50

// Performance thresholds
#define PERFORMANCE_WARNING_QUEUE_SIZE 200
#define PERFORMANCE_WARNING_PROCESSING_TIME_MS 10

// ============================================================================
// INTERNAL DATA STRUCTURES
// ============================================================================

/**
 * Event queue entry
 */
typedef struct event_queue_entry {
    layer_event_t event;
    struct event_queue_entry *next;
    struct event_queue_entry *prev;
} event_queue_entry_t;

/**
 * Event queue structure
 */
typedef struct {
    event_queue_entry_t *head;
    event_queue_entry_t *tail;
    uint32_t size;
    uint32_t max_size;
} event_queue_t;

/**
 * Subscriber list entry
 */
typedef struct subscriber_entry {
    layer_event_subscriber_t subscriber;
    struct subscriber_entry *next;
} subscriber_entry_t;

/**
 * Event system internal structure
 */
struct layer_event_system {
    uint32_t magic;               // Magic number for validation
    bool initialized;             // Initialization status
    layer_events_config_t config; // System configuration

    // Event queues (one per priority level)
    event_queue_t queues[4]; // Critical, High, Normal, Low

    // Subscriber management
    subscriber_entry_t *subscribers; // Subscriber linked list
    uint32_t subscriber_count;       // Number of active subscribers

    // Event generation
    uint32_t next_sequence_number; // Global sequence counter
    uint32_t next_event_id;        // Unique event ID counter

    // Statistics and monitoring
    layer_event_stats_t stats; // Performance statistics
    bool debug_enabled;        // Debug logging enabled

    // Internal state
    layer_events_error_t last_error; // Last error that occurred
};

// ============================================================================
// STATIC VARIABLES
// ============================================================================

static bool layer_events_global_initialized = false;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static event_queue_entry_t *create_queue_entry(const layer_event_t *event);
static void destroy_queue_entry(event_queue_entry_t *entry);
static layer_events_error_t enqueue_event(layer_event_system_t *events,
                                          const layer_event_t *event);
static layer_event_t *dequeue_event(layer_event_system_t *events,
                                    layer_event_priority_t priority);
static layer_events_error_t notify_subscribers(layer_event_system_t *events,
                                               const layer_event_t *event);
static subscriber_entry_t *find_subscriber(layer_event_system_t *events,
                                           layer_event_type_t event_type,
                                           layer_id_t subscriber_id);
static void cleanup_event_data(layer_event_t *event);
static bool validate_event_system(const layer_event_system_t *events);
static uint64_t get_timestamp_ns(void);
static void debug_log(layer_event_system_t *events, const char *format, ...);

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

/**
 * Create a new layer events system
 */
layer_event_system_t *layer_events_create(const layer_events_config_t *config) {
    layer_event_system_t *events = calloc(1, sizeof(layer_event_system_t));
    if (!events) {
        return NULL;
    }

    events->magic = EVENT_SYSTEM_MAGIC;
    events->initialized = false;

    // Set configuration (use defaults if config is NULL)
    if (config) {
        events->config = *config;
    } else {
        events->config = layer_events_create_default_config();
    }

    // Initialize queues
    for (int i = 0; i < 4; i++) {
        events->queues[i].head = NULL;
        events->queues[i].tail = NULL;
        events->queues[i].size = 0;
        events->queues[i].max_size = events->config.max_queue_size;
    }

    // Initialize subscriber list
    events->subscribers = NULL;
    events->subscriber_count = 0;

    // Initialize counters
    events->next_sequence_number = 1;
    events->next_event_id = 1;

    // Initialize statistics
    memset(&events->stats, 0, sizeof(layer_event_stats_t));

    events->debug_enabled = events->config.enable_debugging;
    events->last_error = LAYER_EVENTS_SUCCESS;

    return events;
}

/**
 * Initialize the layer events system
 */
layer_events_error_t layer_events_init(layer_event_system_t *events) {
    if (!validate_event_system(events)) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    if (events->initialized) {
        return LAYER_EVENTS_SUCCESS; // Already initialized
    }

    debug_log(events, "Initializing layer events system");

    // Validate configuration
    if (events->config.max_queue_size == 0 ||
        events->config.max_subscribers == 0) {
        events->last_error = LAYER_EVENTS_ERROR_INVALID_PARAM;
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    events->initialized = true;
    layer_events_global_initialized = true;

    debug_log(events, "Layer events system initialized successfully");

    return LAYER_EVENTS_SUCCESS;
}

/**
 * Clean up event system resources
 */
layer_events_error_t layer_events_cleanup(layer_event_system_t *events) {
    if (!validate_event_system(events)) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    debug_log(events, "Cleaning up layer events system");

    // Clear all event queues
    for (int priority = 0; priority < 4; priority++) {
        event_queue_t *queue = &events->queues[priority];
        event_queue_entry_t *entry = queue->head;

        while (entry) {
            event_queue_entry_t *next = entry->next;
            cleanup_event_data(&entry->event);
            destroy_queue_entry(entry);
            entry = next;
        }

        queue->head = NULL;
        queue->tail = NULL;
        queue->size = 0;
    }

    // Clear subscriber list
    subscriber_entry_t *subscriber = events->subscribers;
    while (subscriber) {
        subscriber_entry_t *next = subscriber->next;
        free(subscriber);
        subscriber = next;
    }
    events->subscribers = NULL;
    events->subscriber_count = 0;

    events->initialized = false;

    debug_log(events, "Layer events system cleanup completed");

    return LAYER_EVENTS_SUCCESS;
}

/**
 * Destroy event system instance
 */
void layer_events_destroy(layer_event_system_t *events) {
    if (!events) {
        return;
    }

    if (events->magic == EVENT_SYSTEM_MAGIC) {
        layer_events_cleanup(events);
        events->magic = 0; // Clear magic number
    }

    free(events);
}

// ============================================================================
// EVENT PUBLISHING FUNCTIONS
// ============================================================================

/**
 * Publish an event to the system
 */
layer_events_error_t layer_events_publish(layer_event_system_t *events,
                                          const layer_event_t *event) {
    if (!validate_event_system(events) || !event) {
        if (events)
            events->last_error = LAYER_EVENTS_ERROR_INVALID_PARAM;
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    if (!events->initialized) {
        events->last_error = LAYER_EVENTS_ERROR_NOT_INITIALIZED;
        return LAYER_EVENTS_ERROR_NOT_INITIALIZED;
    }

    if (!layer_events_validate_event(event)) {
        events->last_error = LAYER_EVENTS_ERROR_INVALID_PARAM;
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    debug_log(events, "Publishing event type %d from layer %d", event->type,
              event->source_layer);

    events->stats.events_published++;

    // Create a copy of the event with system-generated fields
    layer_event_t event_copy = *event;
    event_copy.timestamp = get_timestamp_ns();
    event_copy.sequence_number = events->next_sequence_number++;
    event_copy.event_id = events->next_event_id++;
    event_copy.processing_attempts = 0;
    event_copy.last_processing_time = 0;
    event_copy.processed = false;

    // Handle immediate processing for NO_QUEUE flag
    if (event->flags & LAYER_EVENT_FLAG_NO_QUEUE) {
        layer_events_error_t result = notify_subscribers(events, &event_copy);
        if (result == LAYER_EVENTS_SUCCESS) {
            events->stats.events_processed++;
        } else {
            events->stats.events_failed++;
        }
        return result;
    }

    // Queue the event for later processing
    layer_events_error_t result = enqueue_event(events, &event_copy);
    if (result != LAYER_EVENTS_SUCCESS) {
        events->last_error = result;
        if (result == LAYER_EVENTS_ERROR_QUEUE_FULL) {
            events->stats.events_dropped++;
            events->stats.queue_overflows++;
        }
    }

    return result;
}

/**
 * Publish a simple event with minimal data
 */
layer_events_error_t
layer_events_publish_simple(layer_event_system_t *events,
                            layer_event_type_t type, layer_id_t source_layer,
                            layer_id_t target_layer,
                            layer_event_priority_t priority) {
    layer_event_t event = {0};
    event.type = type;
    event.source_layer = source_layer;
    event.target_layer = target_layer;
    event.priority = priority;
    event.flags = LAYER_EVENT_FLAG_NONE;

    return layer_events_publish(events, &event);
}

/**
 * Publish a content changed event
 */
layer_events_error_t layer_events_publish_content_changed(
    layer_event_system_t *events, layer_id_t source_layer, const char *content,
    size_t content_length, bool needs_reflow) {
    layer_event_t event = {0};
    event.type = LAYER_EVENT_CONTENT_CHANGED;
    event.source_layer = source_layer;
    event.target_layer = 0; // Broadcast
    event.priority = LAYER_EVENT_PRIORITY_NORMAL;
    event.flags = LAYER_EVENT_FLAG_BROADCAST;

    // Create a copy of the content string to avoid dangling pointers
    char *content_copy = NULL;
    if (content && content_length > 0) {
        content_copy = malloc(content_length + 1);
        if (content_copy) {
            memcpy(content_copy, content, content_length);
            content_copy[content_length] = '\0';
        }
    }

    event.data.content_changed.new_content = content_copy;
    event.data.content_changed.content_length = content_length;
    event.data.content_changed.needs_reflow = needs_reflow;

    return layer_events_publish(events, &event);
}

/**
 * Publish a size changed event
 */
layer_events_error_t layer_events_publish_size_changed(
    layer_event_system_t *events, layer_id_t source_layer, int old_width,
    int old_height, int new_width, int new_height) {
    layer_event_t event = {0};
    event.type = LAYER_EVENT_SIZE_CHANGED;
    event.source_layer = source_layer;
    event.target_layer = 0; // Broadcast
    event.priority = LAYER_EVENT_PRIORITY_HIGH;
    event.flags = LAYER_EVENT_FLAG_BROADCAST;

    event.data.size_changed.old_width = old_width;
    event.data.size_changed.old_height = old_height;
    event.data.size_changed.new_width = new_width;
    event.data.size_changed.new_height = new_height;

    return layer_events_publish(events, &event);
}

// ============================================================================
// EVENT SUBSCRIPTION FUNCTIONS
// ============================================================================

/**
 * Subscribe to events of a specific type
 */
layer_events_error_t
layer_events_subscribe(layer_event_system_t *events,
                       layer_event_type_t event_type, layer_id_t subscriber_id,
                       layer_event_callback_t callback, void *user_data,
                       layer_event_priority_t min_priority) {
    if (!validate_event_system(events) || !callback) {
        if (events)
            events->last_error = LAYER_EVENTS_ERROR_INVALID_PARAM;
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    if (!events->initialized) {
        events->last_error = LAYER_EVENTS_ERROR_NOT_INITIALIZED;
        return LAYER_EVENTS_ERROR_NOT_INITIALIZED;
    }

    if (events->subscriber_count >= events->config.max_subscribers) {
        events->last_error = LAYER_EVENTS_ERROR_MAX_SUBSCRIBERS;
        return LAYER_EVENTS_ERROR_MAX_SUBSCRIBERS;
    }

    // Check if already subscribed
    if (find_subscriber(events, event_type, subscriber_id)) {
        return LAYER_EVENTS_SUCCESS; // Already subscribed
    }

    debug_log(events, "Subscribing layer %d to event type %d", subscriber_id,
              event_type);

    // Create new subscriber entry
    subscriber_entry_t *entry = malloc(sizeof(subscriber_entry_t));
    if (!entry) {
        events->last_error = LAYER_EVENTS_ERROR_MEMORY_ALLOCATION;
        return LAYER_EVENTS_ERROR_MEMORY_ALLOCATION;
    }

    entry->subscriber.event_type = event_type;
    entry->subscriber.subscriber_id = subscriber_id;
    entry->subscriber.callback = callback;
    entry->subscriber.user_data = user_data;
    entry->subscriber.min_priority = min_priority;
    entry->subscriber.active = true;
    entry->subscriber.events_received = 0;
    entry->subscriber.events_processed = 0;
    entry->subscriber.last_event_time = 0;

    // Add to subscriber list
    entry->next = events->subscribers;
    events->subscribers = entry;
    events->subscriber_count++;
    events->stats.active_subscribers = events->subscriber_count;

    return LAYER_EVENTS_SUCCESS;
}

/**
 * Unsubscribe from events
 */
layer_events_error_t layer_events_unsubscribe(layer_event_system_t *events,
                                              layer_event_type_t event_type,
                                              layer_id_t subscriber_id) {
    if (!validate_event_system(events)) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    if (!events->initialized) {
        events->last_error = LAYER_EVENTS_ERROR_NOT_INITIALIZED;
        return LAYER_EVENTS_ERROR_NOT_INITIALIZED;
    }

    debug_log(events, "Unsubscribing layer %d from event type %d",
              subscriber_id, event_type);

    subscriber_entry_t **current = &events->subscribers;
    while (*current) {
        if ((*current)->subscriber.event_type == event_type &&
            (*current)->subscriber.subscriber_id == subscriber_id) {

            subscriber_entry_t *to_remove = *current;
            *current = (*current)->next;
            free(to_remove);
            events->subscriber_count--;
            events->stats.active_subscribers = events->subscriber_count;
            return LAYER_EVENTS_SUCCESS;
        }
        current = &(*current)->next;
    }

    events->last_error = LAYER_EVENTS_ERROR_SUBSCRIBER_NOT_FOUND;
    return LAYER_EVENTS_ERROR_SUBSCRIBER_NOT_FOUND;
}

/**
 * Unsubscribe from all events
 */
layer_events_error_t layer_events_unsubscribe_all(layer_event_system_t *events,
                                                  layer_id_t subscriber_id) {
    if (!validate_event_system(events)) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    if (!events->initialized) {
        events->last_error = LAYER_EVENTS_ERROR_NOT_INITIALIZED;
        return LAYER_EVENTS_ERROR_NOT_INITIALIZED;
    }

    debug_log(events, "Unsubscribing layer %d from all events", subscriber_id);

    subscriber_entry_t **current = &events->subscribers;
    int removed_count = 0;

    while (*current) {
        if ((*current)->subscriber.subscriber_id == subscriber_id) {
            subscriber_entry_t *to_remove = *current;
            *current = (*current)->next;
            free(to_remove);
            removed_count++;
        } else {
            current = &(*current)->next;
        }
    }

    events->subscriber_count -= removed_count;
    events->stats.active_subscribers = events->subscriber_count;

    return LAYER_EVENTS_SUCCESS;
}

// ============================================================================
// EVENT PROCESSING FUNCTIONS
// ============================================================================

/**
 * Process pending events
 */
int layer_events_process_pending(layer_event_system_t *events,
                                 uint32_t max_events, uint32_t timeout_ms) {
    if (!validate_event_system(events)) {
        return -1;
    }

    if (!events->initialized) {
        events->last_error = LAYER_EVENTS_ERROR_NOT_INITIALIZED;
        return -1;
    }

    uint64_t start_time = get_timestamp_ns();
    uint64_t timeout_ns = timeout_ms * 1000000ULL;
    int processed_count = 0;

    // Process events in priority order: Critical, High, Normal, Low
    for (int priority = LAYER_EVENT_PRIORITY_CRITICAL;
         priority >= LAYER_EVENT_PRIORITY_LOW; priority--) {

        while (true) {
            // Check timeout
            if (timeout_ms > 0) {
                uint64_t elapsed = get_timestamp_ns() - start_time;
                if (elapsed >= timeout_ns) {
                    return processed_count;
                }
            }

            // Check max events limit
            if (max_events > 0 && processed_count >= (int)max_events) {
                return processed_count;
            }

            // Get next event from this priority queue
            layer_event_t *event =
                dequeue_event(events, (layer_event_priority_t)priority);
            if (!event) {
                break; // No more events at this priority
            }

            // Process the event
            uint64_t processing_start = get_timestamp_ns();
            layer_events_error_t result = notify_subscribers(events, event);
            uint64_t processing_time = get_timestamp_ns() - processing_start;

            events->stats.total_processing_time_ns += processing_time;

            if (result == LAYER_EVENTS_SUCCESS) {
                events->stats.events_processed++;
                processed_count++;
            } else {
                events->stats.events_failed++;
            }

            // Check for performance warnings
            if (events->config.enable_performance_monitoring) {
                uint64_t processing_time_ms = processing_time / 1000000;
                if (processing_time_ms >
                    PERFORMANCE_WARNING_PROCESSING_TIME_MS) {
                    debug_log(
                        events,
                        "Performance warning: Event processing took %lu ms",
                        processing_time_ms);
                }
            }

            cleanup_event_data(event);
            free(event);
        }
    }

    return processed_count;
}

/**
 * Process events of specific priority
 */
int layer_events_process_priority(layer_event_system_t *events,
                                  layer_event_priority_t priority,
                                  uint32_t max_events) {
    if (!validate_event_system(events)) {
        return -1;
    }

    if (!events->initialized) {
        events->last_error = LAYER_EVENTS_ERROR_NOT_INITIALIZED;
        return -1;
    }

    int processed_count = 0;

    while (max_events == 0 || processed_count < (int)max_events) {
        layer_event_t *event = dequeue_event(events, priority);
        if (!event) {
            break; // No more events at this priority
        }

        layer_events_error_t result = notify_subscribers(events, event);

        if (result == LAYER_EVENTS_SUCCESS) {
            events->stats.events_processed++;
            processed_count++;
        } else {
            events->stats.events_failed++;
        }

        cleanup_event_data(event);
        free(event);
    }

    return processed_count;
}

/**
 * Check if there are pending events
 */
bool layer_events_has_pending(layer_event_system_t *events) {
    if (!validate_event_system(events) || !events->initialized) {
        return false;
    }

    for (int i = 0; i < 4; i++) {
        if (events->queues[i].size > 0) {
            return true;
        }
    }

    return false;
}

/**
 * Get number of pending events
 */
uint32_t layer_events_get_pending_count(layer_event_system_t *events) {
    if (!validate_event_system(events) || !events->initialized) {
        return 0;
    }

    uint32_t total = 0;
    for (int i = 0; i < 4; i++) {
        total += events->queues[i].size;
    }

    return total;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Get event type name
 */
const char *layer_events_get_type_name(layer_event_type_t event_type) {
    switch (event_type) {
    case LAYER_EVENT_NONE:
        return "NONE";
    case LAYER_EVENT_CONTENT_CHANGED:
        return "CONTENT_CHANGED";
    case LAYER_EVENT_TEXT_UPDATED:
        return "TEXT_UPDATED";
    case LAYER_EVENT_STYLE_UPDATED:
        return "STYLE_UPDATED";
    case LAYER_EVENT_SIZE_CHANGED:
        return "SIZE_CHANGED";
    case LAYER_EVENT_POSITION_CHANGED:
        return "POSITION_CHANGED";
    case LAYER_EVENT_CURSOR_MOVED:
        return "CURSOR_MOVED";
    case LAYER_EVENT_REDRAW_NEEDED:
        return "REDRAW_NEEDED";
    case LAYER_EVENT_REFRESH_REQUESTED:
        return "REFRESH_REQUESTED";
    case LAYER_EVENT_VISIBILITY_CHANGED:
        return "VISIBILITY_CHANGED";
    case LAYER_EVENT_THEME_CHANGED:
        return "THEME_CHANGED";
    case LAYER_EVENT_COLOR_SCHEME_UPDATED:
        return "COLOR_SCHEME_UPDATED";
    case LAYER_EVENT_FONT_CHANGED:
        return "FONT_CHANGED";
    case LAYER_EVENT_INPUT_RECEIVED:
        return "INPUT_RECEIVED";
    case LAYER_EVENT_FOCUS_GAINED:
        return "FOCUS_GAINED";
    case LAYER_EVENT_FOCUS_LOST:
        return "FOCUS_LOST";
    case LAYER_EVENT_TERMINAL_RESIZE:
        return "TERMINAL_RESIZE";
    case LAYER_EVENT_INITIALIZATION_COMPLETE:
        return "INITIALIZATION_COMPLETE";
    case LAYER_EVENT_CLEANUP_REQUESTED:
        return "CLEANUP_REQUESTED";
    case LAYER_EVENT_ERROR_OCCURRED:
        return "ERROR_OCCURRED";
    case LAYER_EVENT_PERFORMANCE_WARNING:
        return "PERFORMANCE_WARNING";
    case LAYER_EVENT_DEBUG_INFO:
        return "DEBUG_INFO";
    default:
        return "UNKNOWN";
    }
}

/**
 * Get layer name
 */
const char *layer_events_get_layer_name(layer_id_t layer_id) {
    switch (layer_id) {
    case LAYER_ID_UNKNOWN:
        return "UNKNOWN";
    case LAYER_ID_BASE_TERMINAL:
        return "BASE_TERMINAL";
    case LAYER_ID_TERMINAL_CONTROL:
        return "TERMINAL_CONTROL";
    case LAYER_ID_PROMPT_LAYER:
        return "PROMPT_LAYER";
    case LAYER_ID_COMMAND_LAYER:
        return "COMMAND_LAYER";
    case LAYER_ID_COMPOSITION_ENGINE:
        return "COMPOSITION_ENGINE";
    case LAYER_ID_DISPLAY_CONTROLLER:
        return "DISPLAY_CONTROLLER";
    case LAYER_ID_EXTERNAL:
        return "EXTERNAL";
    default:
        return "CUSTOM";
    }
}

/**
 * Create default configuration
 */
layer_events_config_t layer_events_create_default_config(void) {
    layer_events_config_t config = {0};
    config.max_queue_size = DEFAULT_MAX_QUEUE_SIZE;
    config.max_subscribers = DEFAULT_MAX_SUBSCRIBERS;
    config.processing_timeout_ms = DEFAULT_PROCESSING_TIMEOUT_MS;
    config.enable_debugging = false;
    config.enable_performance_monitoring = true;
    config.auto_cleanup_processed_events = true;
    return config;
}

/**
 * Validate event structure
 */
bool layer_events_validate_event(const layer_event_t *event) {
    if (!event) {
        return false;
    }

    // Check event type is valid
    if (event->type <= LAYER_EVENT_NONE) {
        return false;
    }

    // Check priority is valid
    if (event->priority < LAYER_EVENT_PRIORITY_LOW ||
        event->priority > LAYER_EVENT_PRIORITY_CRITICAL) {
        return false;
    }

    return true;
}

/**
 * Get event system statistics
 */
layer_event_stats_t layer_events_get_statistics(layer_event_system_t *events) {
    layer_event_stats_t empty_stats = {0};

    if (!validate_event_system(events) || !events->initialized) {
        return empty_stats;
    }

    // Update current queue size
    events->stats.current_queue_size = layer_events_get_pending_count(events);

    return events->stats;
}

/**
 * Clear event system statistics
 */
layer_events_error_t
layer_events_clear_statistics(layer_event_system_t *events) {
    if (!validate_event_system(events)) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    if (!events->initialized) {
        events->last_error = LAYER_EVENTS_ERROR_NOT_INITIALIZED;
        return LAYER_EVENTS_ERROR_NOT_INITIALIZED;
    }

    memset(&events->stats, 0, sizeof(layer_event_stats_t));
    events->stats.active_subscribers = events->subscriber_count;

    return LAYER_EVENTS_SUCCESS;
}

/**
 * Enable or disable debug logging
 */
layer_events_error_t
layer_events_set_debug_enabled(layer_event_system_t *events, bool enabled) {
    if (!validate_event_system(events)) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    events->debug_enabled = enabled;
    events->config.enable_debugging = enabled;

    return LAYER_EVENTS_SUCCESS;
}

/**
 * Get error description string
 */
const char *layer_events_error_string(layer_events_error_t error) {
    switch (error) {
    case LAYER_EVENTS_SUCCESS:
        return "Success";
    case LAYER_EVENTS_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case LAYER_EVENTS_ERROR_MEMORY_ALLOCATION:
        return "Memory allocation failed";
    case LAYER_EVENTS_ERROR_QUEUE_FULL:
        return "Event queue is full";
    case LAYER_EVENTS_ERROR_QUEUE_EMPTY:
        return "Event queue is empty";
    case LAYER_EVENTS_ERROR_SUBSCRIBER_NOT_FOUND:
        return "Subscriber not found";
    case LAYER_EVENTS_ERROR_MAX_SUBSCRIBERS:
        return "Maximum subscribers reached";
    case LAYER_EVENTS_ERROR_EVENT_TOO_LARGE:
        return "Event data too large";
    case LAYER_EVENTS_ERROR_TIMEOUT:
        return "Event processing timeout";
    case LAYER_EVENTS_ERROR_NOT_INITIALIZED:
        return "Events system not initialized";
    default:
        return "Unknown error";
    }
}

// ============================================================================
// STATIC HELPER FUNCTIONS
// ============================================================================

/**
 * Create queue entry
 */
static event_queue_entry_t *create_queue_entry(const layer_event_t *event) {
    event_queue_entry_t *entry = malloc(sizeof(event_queue_entry_t));
    if (!entry) {
        return NULL;
    }

    entry->event = *event;
    entry->next = NULL;
    entry->prev = NULL;

    return entry;
}

/**
 * Destroy queue entry
 */
static void destroy_queue_entry(event_queue_entry_t *entry) {
    if (entry) {
        cleanup_event_data(&entry->event);
        free(entry);
    }
}

/**
 * Enqueue event
 */
static layer_events_error_t enqueue_event(layer_event_system_t *events,
                                          const layer_event_t *event) {
    event_queue_t *queue = &events->queues[event->priority];

    if (queue->size >= queue->max_size) {
        return LAYER_EVENTS_ERROR_QUEUE_FULL;
    }

    event_queue_entry_t *entry = create_queue_entry(event);
    if (!entry) {
        return LAYER_EVENTS_ERROR_MEMORY_ALLOCATION;
    }

    // Add to tail of queue
    if (queue->tail) {
        queue->tail->next = entry;
        entry->prev = queue->tail;
        queue->tail = entry;
    } else {
        queue->head = queue->tail = entry;
    }

    queue->size++;
    events->stats.current_queue_size++;

    if (queue->size > events->stats.max_queue_size_reached) {
        events->stats.max_queue_size_reached = queue->size;
    }

    return LAYER_EVENTS_SUCCESS;
}

/**
 * Dequeue event
 */
static layer_event_t *dequeue_event(layer_event_system_t *events,
                                    layer_event_priority_t priority) {
    event_queue_t *queue = &events->queues[priority];

    if (queue->size == 0) {
        return NULL;
    }

    event_queue_entry_t *entry = queue->head;
    layer_event_t *event = malloc(sizeof(layer_event_t));
    if (!event) {
        return NULL;
    }

    *event = entry->event;

    // Remove from head of queue
    queue->head = entry->next;
    if (queue->head) {
        queue->head->prev = NULL;
    } else {
        queue->tail = NULL;
    }

    queue->size--;
    events->stats.current_queue_size--;

    free(entry);
    return event;
}

/**
 * Notify subscribers
 */
static layer_events_error_t notify_subscribers(layer_event_system_t *events,
                                               const layer_event_t *event) {
    subscriber_entry_t *subscriber = events->subscribers;
    bool found_subscriber = false;
    layer_events_error_t last_error = LAYER_EVENTS_SUCCESS;

    while (subscriber) {
        if (subscriber->subscriber.event_type == event->type &&
            subscriber->subscriber.active &&
            event->priority >= subscriber->subscriber.min_priority) {

            // Check if event is targeted to this subscriber
            if (event->target_layer != 0 &&
                event->target_layer != subscriber->subscriber.subscriber_id) {
                subscriber = subscriber->next;
                continue;
            }

            found_subscriber = true;
            subscriber->subscriber.events_received++;
            subscriber->subscriber.last_event_time = event->timestamp;

            debug_log(events, "Notifying subscriber %d for event %d",
                      subscriber->subscriber.subscriber_id, event->type);

            layer_events_error_t result = subscriber->subscriber.callback(
                event, subscriber->subscriber.user_data);

            if (result == LAYER_EVENTS_SUCCESS) {
                subscriber->subscriber.events_processed++;
            } else {
                last_error = result;
                debug_log(events, "Subscriber callback failed with error: %s",
                          layer_events_error_string(result));
            }
        }

        subscriber = subscriber->next;
    }

    return found_subscriber ? last_error : LAYER_EVENTS_SUCCESS;
}

/**
 * Find subscriber
 */
static subscriber_entry_t *find_subscriber(layer_event_system_t *events,
                                           layer_event_type_t event_type,
                                           layer_id_t subscriber_id) {
    subscriber_entry_t *subscriber = events->subscribers;

    while (subscriber) {
        if (subscriber->subscriber.event_type == event_type &&
            subscriber->subscriber.subscriber_id == subscriber_id) {
            return subscriber;
        }
        subscriber = subscriber->next;
    }

    return NULL;
}

/**
 * Cleanup event data
 */
static void cleanup_event_data(layer_event_t *event) {
    if (!event) {
        return;
    }

    // Clean up content changed data
    if (event->type == LAYER_EVENT_CONTENT_CHANGED &&
        event->data.content_changed.new_content) {
        free((void *)event->data.content_changed.new_content);
        event->data.content_changed.new_content = NULL;
    }

    // Clean up custom data if cleanup function is provided
    if (event->data.custom_data.cleanup_func && event->data.custom_data.data) {
        event->data.custom_data.cleanup_func(event->data.custom_data.data);
        event->data.custom_data.data = NULL;
        event->data.custom_data.cleanup_func = NULL;
    }
}

/**
 * Validate event system
 */
static bool validate_event_system(const layer_event_system_t *events) {
    return events && events->magic == EVENT_SYSTEM_MAGIC;
}

/**
 * Get timestamp in nanoseconds
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

/**
 * Debug logging function
 */
static void debug_log(layer_event_system_t *events, const char *format, ...) {
    if (!events || !events->debug_enabled) {
        return;
    }

    va_list args;
    va_start(args, format);

    printf("[LAYER_EVENTS] ");
    vprintf(format, args);
    printf("\n");

    va_end(args);
}