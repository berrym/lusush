/**
 * @file event_queue.c
 * @brief LLE Event Queue Implementation
 * 
 * Thread-safe circular buffer FIFO queue for events.
 * Phase 1: Simple FIFO queue with mutex protection.
 * 
 * Spec: 04_event_system_complete.md - Phase 1
 */

#include "lle/event_system.h"
#include "lle/error_handling.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * EVENT QUEUE LIFECYCLE
 * ============================================================================
 */

lle_result_t lle_event_queue_init(lle_event_queue_t **queue, size_t capacity) {
    if (!queue || capacity == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate queue structure */
    lle_event_queue_t *q = calloc(1, sizeof(lle_event_queue_t));
    if (!q) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Allocate event pointer array */
    q->events = calloc(capacity, sizeof(lle_event_t *));
    if (!q->events) {
        free(q);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize queue state */
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    
    /* Initialize mutex */
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        free(q->events);
        free(q);
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    *queue = q;
    return LLE_SUCCESS;
}

void lle_event_queue_destroy(lle_event_queue_t *queue) {
    if (!queue) {
        return;
    }
    
    /* Destroy mutex */
    pthread_mutex_destroy(&queue->mutex);
    
    /* Free event array (does NOT destroy events themselves) */
    free(queue->events);
    
    /* Free queue structure */
    free(queue);
}

/* ============================================================================
 * QUEUE OPERATIONS (Called from event_system.c)
 * ============================================================================
 */

/**
 * @brief Enqueue event (internal - called with system mutex held)
 * 
 * This is an internal function called from lle_event_enqueue() in event_system.c
 * The caller holds the system mutex, so we still need our own mutex for queue ops.
 */
lle_result_t event_queue_enqueue_internal(lle_event_queue_t *queue, lle_event_t *event) {
    if (!queue || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Check if queue is full */
    if (queue->count >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        return LLE_ERROR_QUEUE_FULL;
    }
    
    /* Add event to tail */
    queue->events[queue->tail] = event;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    pthread_mutex_unlock(&queue->mutex);
    return LLE_SUCCESS;
}

/**
 * @brief Dequeue event (internal - called with system mutex held)
 * 
 * This is an internal function called from lle_event_dequeue() in event_system.c
 */
lle_result_t event_queue_dequeue_internal(lle_event_queue_t *queue, lle_event_t **event) {
    if (!queue || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Check if queue is empty */
    if (queue->count == 0) {
        *event = NULL;
        pthread_mutex_unlock(&queue->mutex);
        return LLE_SUCCESS; /* Not an error, just empty */
    }
    
    /* Remove event from head */
    *event = queue->events[queue->head];
    queue->events[queue->head] = NULL;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    pthread_mutex_unlock(&queue->mutex);
    return LLE_SUCCESS;
}

/**
 * @brief Get queue count (internal)
 */
size_t event_queue_count_internal(lle_event_queue_t *queue) {
    if (!queue) {
        return 0;
    }
    
    pthread_mutex_lock(&queue->mutex);
    size_t count = queue->count;
    pthread_mutex_unlock(&queue->mutex);
    
    return count;
}
