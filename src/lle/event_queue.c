/*
 * event_queue.c - Event Queue Implementation (Phase 1)
 * 
 * Simple circular buffer queue with thread safety.
 * 
 * Spec 04: Event System - Phase 1
 */

#include "../../include/lle/event_system.h"
#include <stdlib.h>
#include <string.h>

/*
 * Initialize event queue
 */
lle_result_t lle_event_queue_init(lle_event_queue_t **queue, size_t capacity) {
    if (!queue || capacity == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate queue structure */
    lle_event_queue_t *q = lle_pool_alloc(sizeof(lle_event_queue_t));
    if (!q) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(q, 0, sizeof(lle_event_queue_t));
    
    /* Allocate event array */
    q->events = lle_pool_alloc(sizeof(lle_event_t*) * capacity);
    if (!q->events) {
        lle_pool_free(q);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(q->events, 0, sizeof(lle_event_t*) * capacity);
    
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    
    /* Initialize mutex */
    pthread_mutex_init(&q->mutex, NULL);
    
    *queue = q;
    return LLE_SUCCESS;
}

/*
 * Destroy event queue
 */
void lle_event_queue_destroy(lle_event_queue_t *queue) {
    if (!queue) {
        return;
    }
    
    /* Note: We don't destroy events here - they should be dequeued first
     * or destroyed by the event system */
    
    /* Free event array */
    if (queue->events) {
        lle_pool_free(queue->events);
    }
    
    /* Destroy mutex */
    pthread_mutex_destroy(&queue->mutex);
    
    /* Free queue structure */
    lle_pool_free(queue);
}

/*
 * Enqueue event
 */
lle_result_t lle_event_enqueue(lle_event_system_t *system, lle_event_t *event) {
    if (!system || !system->queue || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_event_queue_t *queue = system->queue;
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Check if queue is full */
    if (queue->count >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        __atomic_fetch_add(&system->events_dropped, 1, __ATOMIC_SEQ_CST);
        return LLE_ERROR_QUEUE_FULL;
    }
    
    /* Add event to tail */
    queue->events[queue->tail] = event;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    pthread_mutex_unlock(&queue->mutex);
    
    return LLE_SUCCESS;
}

/*
 * Dequeue event
 */
lle_result_t lle_event_dequeue(lle_event_system_t *system, lle_event_t **event) {
    if (!system || !system->queue || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_event_queue_t *queue = system->queue;
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Check if queue is empty */
    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        *event = NULL;
        return LLE_ERROR_QUEUE_EMPTY;
    }
    
    /* Remove event from head */
    *event = queue->events[queue->head];
    queue->events[queue->head] = NULL;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    pthread_mutex_unlock(&queue->mutex);
    
    return LLE_SUCCESS;
}

/*
 * Get queue size
 */
size_t lle_event_queue_size(lle_event_system_t *system) {
    if (!system || !system->queue) {
        return 0;
    }
    
    lle_event_queue_t *queue = system->queue;
    
    pthread_mutex_lock(&queue->mutex);
    size_t count = queue->count;
    pthread_mutex_unlock(&queue->mutex);
    
    return count;
}

/*
 * Check if queue is empty
 */
bool lle_event_queue_empty(lle_event_system_t *system) {
    return lle_event_queue_size(system) == 0;
}

/*
 * Check if queue is full
 */
bool lle_event_queue_full(lle_event_system_t *system) {
    if (!system || !system->queue) {
        return false;
    }
    
    lle_event_queue_t *queue = system->queue;
    
    pthread_mutex_lock(&queue->mutex);
    bool full = (queue->count >= queue->capacity);
    pthread_mutex_unlock(&queue->mutex);
    
    return full;
}
