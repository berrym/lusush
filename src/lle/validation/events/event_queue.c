/*
 * Lusush Line Editor (LLE) - Phase 0 Validation
 * Event Queue Implementation - Week 4 Day 16
 * 
 * Copyright (C) 2021-2025  Michael Berry
 */

#include "event_queue.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get current timestamp in nanoseconds
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int lle_event_queue_init(lle_event_queue_t *queue,
                         size_t capacity,
                         bool thread_safe) {
    if (!queue || capacity == 0) {
        return -1;
    }
    
    queue->events = calloc(capacity, sizeof(lle_event_t));
    if (!queue->events) {
        return -1;
    }
    
    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->total_enqueued = 0;
    queue->total_dequeued = 0;
    queue->dropped_events = 0;
    queue->thread_safe = thread_safe;
    queue->lock_contentions = 0;
    
    return 0;
}

int lle_event_enqueue(lle_event_queue_t *queue, const lle_event_t *event) {
    if (!queue || !event) {
        return -1;
    }
    
    // Check if queue is full
    if (queue->count >= queue->capacity) {
        queue->dropped_events++;
        return -1;
    }
    
    // Copy event to queue
    queue->events[queue->tail] = *event;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    queue->total_enqueued++;
    
    return 0;
}

int lle_event_dequeue(lle_event_queue_t *queue, lle_event_t *event) {
    if (!queue || !event) {
        return -1;
    }
    
    // Check if queue is empty
    if (queue->count == 0) {
        return -1;
    }
    
    // Copy event from queue
    *event = queue->events[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    queue->total_dequeued++;
    
    return 0;
}

bool lle_event_queue_is_empty(const lle_event_queue_t *queue) {
    return (queue && queue->count == 0);
}

bool lle_event_queue_is_full(const lle_event_queue_t *queue) {
    return (queue && queue->count >= queue->capacity);
}

size_t lle_event_queue_size(const lle_event_queue_t *queue) {
    return queue ? queue->count : 0;
}

int lle_event_queue_get_stats(const lle_event_queue_t *queue,
                               uint64_t *total_enqueued,
                               uint64_t *total_dequeued,
                               uint64_t *dropped) {
    if (!queue) {
        return -1;
    }
    
    if (total_enqueued) *total_enqueued = queue->total_enqueued;
    if (total_dequeued) *total_dequeued = queue->total_dequeued;
    if (dropped) *dropped = queue->dropped_events;
    
    return 0;
}

void lle_event_queue_reset_stats(lle_event_queue_t *queue) {
    if (queue) {
        queue->total_enqueued = 0;
        queue->total_dequeued = 0;
        queue->dropped_events = 0;
    }
}

void lle_event_queue_clear(lle_event_queue_t *queue) {
    if (queue) {
        queue->head = 0;
        queue->tail = 0;
        queue->count = 0;
    }
}

void lle_event_queue_cleanup(lle_event_queue_t *queue) {
    if (queue && queue->events) {
        free(queue->events);
        queue->events = NULL;
        queue->capacity = 0;
        queue->head = 0;
        queue->tail = 0;
        queue->count = 0;
    }
}

void lle_event_create_input(lle_event_t *event,
                            uint32_t codepoint,
                            uint8_t modifiers,
                            uint32_t sequence) {
    if (!event) return;
    
    memset(event, 0, sizeof(lle_event_t));
    event->type = LLE_EVENT_INPUT;
    event->timestamp_ns = get_timestamp_ns();
    event->sequence = sequence;
    event->data.input.codepoint = codepoint;
    event->data.input.modifiers = modifiers;
}

void lle_event_create_signal(lle_event_t *event,
                             lle_signal_type_t signal,
                             uint32_t sequence) {
    if (!event) return;
    
    memset(event, 0, sizeof(lle_event_t));
    event->type = LLE_EVENT_SIGNAL;
    event->timestamp_ns = get_timestamp_ns();
    event->sequence = sequence;
    event->data.signal.signal = signal;
}

void lle_event_create_resize(lle_event_t *event,
                             uint16_t rows,
                             uint16_t cols,
                             uint32_t sequence) {
    if (!event) return;
    
    memset(event, 0, sizeof(lle_event_t));
    event->type = LLE_EVENT_RESIZE;
    event->timestamp_ns = get_timestamp_ns();
    event->sequence = sequence;
    event->data.resize.rows = rows;
    event->data.resize.cols = cols;
}
