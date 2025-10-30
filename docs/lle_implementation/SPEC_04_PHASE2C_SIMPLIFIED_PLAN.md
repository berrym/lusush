# Spec 04 Phase 2C - Simplified Implementation Plan

**Phase:** Event System Phase 2C - Event Filtering + Basic State Support  
**Status:** Planning  
**Approach:** Simplified implementation compatible with current Phase 1/2A/2B infrastructure

## Background

The full Phase 2C specification includes:
- Complex filter rule engine with rule registration
- Full state machine framework with state definitions
- Dependencies on handler registry, error context, performance monitors

This simplified plan implements Phase 2C features using our current infrastructure:
- Callback-based event filtering (simple, flexible)
- Basic state tracking (no full state machine yet)
- Integrates with existing event system
- Maintains backward compatibility

## Simplified Phase 2C Goals

1. **Event Filtering** - Filter/block/transform events before dispatch
2. **Filter Statistics** - Track filtered/blocked/passed events  
3. **Basic State Tracking** - Track current system state
4. **Pre/Post Dispatch Hooks** - Callbacks before/after event dispatch

## Implementation Plan

### Part 1: Event Filter System (~250 lines)

Add callback-based event filtering.

**Filter Callback Types:**

```c
// Filter result
typedef enum {
    LLE_FILTER_PASS,        // Pass event through
    LLE_FILTER_BLOCK,       // Block event (don't dispatch)
    LLE_FILTER_TRANSFORM,   // Event was transformed (pass modified version)
    LLE_FILTER_ERROR        // Error in filter
} lle_filter_result_t;

// Filter callback function
typedef lle_filter_result_t (*lle_event_filter_fn)(lle_event_t *event, 
                                                    void *user_data);
```

**Filter Structure:**

```c
typedef struct {
    lle_event_filter_fn filter;  // Filter function
    void *user_data;             // User data for filter
    char name[64];               // Filter name (for debugging)
    bool enabled;                // Filter enabled/disabled
    
    // Statistics
    uint64_t events_filtered;    // Total events checked
    uint64_t events_passed;      // Events passed
    uint64_t events_blocked;     // Events blocked
    uint64_t events_transformed; // Events transformed
    uint64_t events_errored;     // Filter errors
} lle_event_filter_t;

typedef struct {
    lle_event_filter_t **filters;  // Array of filters
    size_t filter_count;            // Number of filters
    size_t filter_capacity;         // Array capacity
    pthread_mutex_t filter_mutex;   // Thread safety
    
    // Global statistics
    uint64_t total_events_filtered;
    uint64_t total_events_blocked;
} lle_event_filter_system_t;
```

**Functions:**
- `lle_event_filter_system_init()` - Initialize filter system
- `lle_event_filter_system_destroy()` - Cleanup filter system
- `lle_event_filter_add()` - Add filter to system
- `lle_event_filter_remove()` - Remove filter by name
- `lle_event_filter_enable()` - Enable specific filter
- `lle_event_filter_disable()` - Disable specific filter
- `lle_event_filter_apply()` - Apply all filters to event (internal)
- `lle_event_filter_get_stats()` - Get filter statistics

### Part 2: Pre/Post Dispatch Hooks (~100 lines)

Add hooks that run before/after event dispatch.

**Hook Callback Types:**

```c
// Pre-dispatch hook (before handlers called)
// Return LLE_SUCCESS to continue, error to skip dispatch
typedef lle_result_t (*lle_event_pre_dispatch_fn)(lle_event_t *event,
                                                   void *user_data);

// Post-dispatch hook (after handlers called)
typedef void (*lle_event_post_dispatch_fn)(lle_event_t *event,
                                           lle_result_t dispatch_result,
                                           void *user_data);
```

**Hook Storage in Event System:**

```c
struct lle_event_system {
    // ... existing fields ...
    
    /* Phase 2C: Event filtering and hooks */
    lle_event_filter_system_t *filter_system; // Event filter system (optional)
    
    lle_event_pre_dispatch_fn pre_dispatch_hook;  // Pre-dispatch callback
    void *pre_dispatch_data;
    
    lle_event_post_dispatch_fn post_dispatch_hook; // Post-dispatch callback  
    void *post_dispatch_data;
};
```

**Functions:**
- `lle_event_set_pre_dispatch_hook()` - Set pre-dispatch hook
- `lle_event_set_post_dispatch_hook()` - Set post-dispatch hook

### Part 3: Basic State Tracking (~50 lines)

Add simple state tracking (not full state machine).

**State Enum:**

```c
typedef enum {
    LLE_STATE_INITIALIZING,   // System initializing
    LLE_STATE_IDLE,           // No events processing
    LLE_STATE_PROCESSING,     // Processing events
    LLE_STATE_PAUSED,         // Processing paused
    LLE_STATE_SHUTTING_DOWN,  // System shutting down
    LLE_STATE_ERROR           // Error state
} lle_system_state_t;
```

**State Storage:**

```c
struct lle_event_system {
    // ... existing fields ...
    
    /* Phase 2C: System state tracking */
    lle_system_state_t current_state;  // Current system state
    lle_system_state_t previous_state; // Previous state (for recovery)
    uint64_t state_changed_time;       // When state last changed
};
```

**Functions:**
- `lle_event_system_set_state()` - Set system state
- `lle_event_system_get_state()` - Get current state
- `lle_event_system_get_previous_state()` - Get previous state

### Part 4: Integration with Event Dispatch (~100 lines)

Update `lle_event_dispatch()` to:
1. Apply filters before dispatch
2. Call pre-dispatch hook
3. Dispatch to handlers (existing logic)
4. Call post-dispatch hook
5. Update filter statistics

**Updated Dispatch Flow:**

```c
lle_result_t lle_event_dispatch(lle_event_system_t *system, lle_event_t *event) {
    // 1. Apply filters
    if (system->filter_system) {
        lle_filter_result_t filter_result = lle_event_filter_apply(system->filter_system, event);
        if (filter_result == LLE_FILTER_BLOCK) {
            return LLE_SUCCESS; // Blocked, but not an error
        }
    }
    
    // 2. Pre-dispatch hook
    if (system->pre_dispatch_hook) {
        lle_result_t hook_result = system->pre_dispatch_hook(event, system->pre_dispatch_data);
        if (hook_result != LLE_SUCCESS) {
            return hook_result; // Hook rejected event
        }
    }
    
    // 3. Dispatch to handlers (existing logic)
    lle_result_t dispatch_result = /* existing dispatch code */;
    
    // 4. Post-dispatch hook
    if (system->post_dispatch_hook) {
        system->post_dispatch_hook(event, dispatch_result, system->post_dispatch_data);
    }
    
    return dispatch_result;
}
```

## File Organization

### New Files
- `src/lle/event_filter.c` - Event filtering implementation (~300 lines)

### Modified Files
- `include/lle/event_system.h` - Add filter types, hooks, state (~100 lines added)
- `src/lle/event_handlers.c` - Update dispatch with filtering (~50 lines modified)
- `src/lle/event_system.c` - Initialize filter system, hooks, state (~30 lines added)

**Total Estimated:** ~480 lines

## Success Criteria

✅ Event filter system can add/remove/enable/disable filters  
✅ Filters can pass/block/transform events  
✅ Filter statistics track passed/blocked/transformed events  
✅ Pre-dispatch hooks work correctly  
✅ Post-dispatch hooks work correctly  
✅ System state tracking implemented  
✅ Event dispatch integrates filtering seamlessly  
✅ All existing tests still pass (backward compatibility)  
✅ Filter system is optional (NULL by default, zero overhead)  

## Implementation Order

1. **Event Filter Types** - Add structures and enums to header
2. **Event Filter Core** - Implement filter system (event_filter.c)
3. **Hooks** - Add pre/post dispatch hook storage and setters
4. **State Tracking** - Add state enum and state management
5. **Integration** - Update event_dispatch to use filters and hooks
6. **Statistics** - Wire filter statistics into enhanced stats
7. **Testing** - Verify all features work, no regressions

## Testing Strategy

**Unit Tests:**
- Filter add/remove/enable/disable
- Filter callback invocation
- Filter results (pass/block/transform)
- Filter statistics accuracy
- Hook invocation
- State transitions

**Integration Tests:**
- Filters integrated with event dispatch
- Multiple filters in sequence
- Hook + filter combinations
- State changes during processing

## Backward Compatibility

This implementation maintains 100% backward compatibility:
- Filter system is optional (NULL by default)
- Hooks are optional (NULL by default)
- Zero overhead when filters/hooks not used
- All Phase 1/2A/2B APIs unchanged
- Existing tests continue to pass

## Deferred Features (Not in Simplified Phase 2C)

These can be added in later phases:
- Filter rule engine (compile filters from rules)
- Full state machine framework (state definitions, transitions)
- State-specific handler registration
- Complex filter chaining/composition
- Filter rule persistence/loading

## Notes

This simplified approach:
- Uses flexible callback-based filtering
- Avoids complex rule engine implementation
- Provides immediate filtering capability
- Simple state tracking (not full state machine)
- Foundation for future enhancements
- Maintains zero-tolerance compliance
