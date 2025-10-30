# LLE Event System - Phase 2D Simplified Implementation Plan

## Overview

Phase 2D adds timer event support to the LLE event system, allowing events to be scheduled for future execution with one-shot or repeating behavior.

## Design Philosophy

Following the proven pattern from Phase 2B and 2C:
- **Opt-in architecture**: Timer system is NULL by default (zero overhead when not used)
- **Simple, robust implementation**: No complex scheduling algorithms initially
- **Thread-safe**: Dedicated mutex for timer operations
- **Backward compatible**: All existing tests continue to pass
- **Separate file**: `event_timer.c` for clean organization

## Core Structures

### Timer Event
```c
typedef struct {
    uint64_t timer_id;                  /* Unique timer identifier */
    lle_event_t *event;                 /* Event to dispatch when timer fires */
    uint64_t trigger_time_us;           /* When to fire (absolute timestamp) */
    uint64_t interval_us;               /* Repeat interval (0 = one-shot) */
    bool repeating;                     /* Is this a repeating timer? */
    bool enabled;                       /* Is timer currently enabled? */
    uint64_t fire_count;                /* How many times has it fired? */
} lle_timer_event_t;

typedef struct {
    lle_timer_event_t **timers;         /* Array of timer pointers */
    size_t timer_count;                 /* Current number of timers */
    size_t timer_capacity;              /* Capacity of array */
    uint64_t next_timer_id;             /* Next ID to assign */
    pthread_mutex_t timer_mutex;        /* Thread safety */
    uint64_t total_timers_created;      /* Statistics */
    uint64_t total_timers_fired;
} lle_timer_system_t;
```

## API Functions (8 functions)

### Timer Management
```c
/* Create a one-shot timer (fires once) */
lle_result_t lle_event_timer_add_oneshot(
    lle_event_system_t *system,
    lle_event_t *event,
    uint64_t delay_us,
    uint64_t *timer_id_out
);

/* Create a repeating timer */
lle_result_t lle_event_timer_add_repeating(
    lle_event_system_t *system,
    lle_event_t *event,
    uint64_t initial_delay_us,
    uint64_t interval_us,
    uint64_t *timer_id_out
);

/* Cancel a timer */
lle_result_t lle_event_timer_cancel(
    lle_event_system_t *system,
    uint64_t timer_id
);

/* Enable/disable a timer (without destroying it) */
lle_result_t lle_event_timer_enable(
    lle_event_system_t *system,
    uint64_t timer_id
);

lle_result_t lle_event_timer_disable(
    lle_event_system_t *system,
    uint64_t timer_id
);

/* Get timer info */
lle_result_t lle_event_timer_get_info(
    lle_event_system_t *system,
    uint64_t timer_id,
    uint64_t *next_fire_time_us,
    uint64_t *fire_count,
    bool *is_repeating
);
```

### Timer Processing
```c
/* Process all timers that are ready to fire */
lle_result_t lle_event_timer_process(
    lle_event_system_t *system
);

/* Initialize timer system (called internally) */
lle_result_t lle_event_timer_system_init(
    lle_event_system_t *system
);

/* Destroy timer system (called internally) */
void lle_event_timer_system_destroy(
    lle_event_system_t *system
);
```

## Implementation Details

### Timer Storage
- Dynamic array of timer pointers
- Initial capacity: 16 timers
- Grows by 2x when needed
- Sorted by trigger_time_us for efficient processing

### Timer Processing
```c
lle_result_t lle_event_timer_process(lle_event_system_t *system) {
    // Get current time
    // Lock timer mutex
    // For each timer in order:
    //   If trigger_time > current_time, break (rest are future)
    //   If enabled:
    //     - Dispatch event
    //     - Increment fire_count
    //     - If repeating: update trigger_time, re-sort
    //     - If one-shot: remove from array
    // Unlock mutex
}
```

### Timer Sorting
- Keep array sorted by trigger_time_us
- Insert new timers in sorted position
- Re-sort after repeating timer fires
- Use simple insertion sort (small N, mostly sorted)

### Event Ownership
- Timer takes ownership of event when created
- Event is copied (deep copy) into timer
- When timer fires, event is dispatched (not destroyed)
- When timer is cancelled/destroyed, event is freed

## Integration Points

### event_system.h Changes
```c
/* Forward declaration */
typedef struct lle_timer_system lle_timer_system_t;

/* Add to lle_event_system_t */
struct lle_event_system {
    // ... existing fields ...
    
    /* Phase 2D: Timer events */
    lle_timer_system_t *timer_system;
};

/* Add Phase 2D API declarations */
```

### event_system.c Changes
```c
/* In lle_event_system_init() */
sys->timer_system = NULL;  // Created on demand

/* In lle_event_system_destroy() */
if (system->timer_system) {
    lle_event_timer_system_destroy(system);
}
```

### event_timer.c (NEW)
- Complete timer implementation
- ~400-500 lines estimated
- All 8 API functions
- Timer sorting and processing logic
- Thread-safe operations

## Usage Example

```c
/* Create event system */
lle_event_system_t *sys = lle_event_system_init();

/* Create event to fire in 1 second */
lle_event_t *evt = lle_event_create(sys, LLE_EVENT_USER_ACTION, NULL);
uint64_t timer_id;
lle_event_timer_add_oneshot(sys, evt, 1000000, &timer_id);  // 1 second

/* Create repeating timer (fires every 100ms) */
lle_event_t *repeat_evt = lle_event_create(sys, LLE_EVENT_RENDER_COMPLETE, NULL);
uint64_t repeat_id;
lle_event_timer_add_repeating(sys, repeat_evt, 100000, 100000, &repeat_id);

/* In main loop */
while (running) {
    lle_event_timer_process(sys);  // Check and fire ready timers
    lle_event_process(sys, 10);    // Process normal events
    usleep(1000);                  // Small sleep
}

/* Cancel timer */
lle_event_timer_cancel(sys, repeat_id);
```

## Design Decisions

### 1. Simple Array + Linear Search
- Optimal for small number of timers (< 100)
- No complex data structures needed
- Easy to debug and maintain
- Can upgrade to heap/tree later if needed

### 2. Absolute Timestamps
- Store absolute trigger time, not relative delay
- Avoids clock drift issues
- Simplifies comparison logic

### 3. Event Copying
- Timer makes deep copy of event
- Avoids ownership confusion
- Caller can free original event after timer_add

### 4. Manual Process Call
- Timer processing not automatic
- Application calls `lle_event_timer_process()` explicitly
- More control, predictable behavior
- Can integrate with event loop or separate thread

### 5. Thread Safety
- Dedicated timer_mutex
- All operations lock/unlock explicitly
- Dispatch happens with mutex held (brief)

## Zero-Tolerance Compliance

✅ **No TODOs**: Complete implementation, no stubs  
✅ **No placeholders**: All functions fully implemented  
✅ **Thread-safe**: Mutex protection for all shared data  
✅ **Error handling**: All edge cases covered  
✅ **Memory safety**: No leaks, proper cleanup  
✅ **Backward compatible**: Existing tests continue to pass  

## Estimated Implementation

- **event_system.h additions**: ~120 lines (structures, API declarations)
- **event_timer.c**: ~450 lines (complete implementation)
- **event_system.c updates**: ~10 lines (init/destroy)
- **meson.build**: ~4 lines

**Total**: ~584 lines

## Testing Strategy

Existing test suite should continue passing (24/24). Timer functionality can be tested via:
1. Integration tests (manual or future timer-specific tests)
2. Example programs demonstrating timer usage
3. Stress testing with many timers

## Success Criteria

✅ All 24 existing tests pass  
✅ No compilation errors or warnings  
✅ Timer system compiles and links successfully  
✅ API is usable and documented  
✅ Memory is properly managed (no leaks)  
✅ Thread-safe operations confirmed  

---

**Status**: Ready for implementation  
**Estimated effort**: 1-2 hours  
**Risk level**: Low (follows proven Phase 2B/2C pattern)
