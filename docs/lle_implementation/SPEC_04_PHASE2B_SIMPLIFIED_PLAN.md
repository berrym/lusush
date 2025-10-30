# Spec 04 Phase 2B - Simplified Implementation Plan

**Phase:** Event System Phase 2B - Event Processor + Enhanced Statistics  
**Status:** Planning  
**Approach:** Simplified implementation compatible with current Phase 1 infrastructure

## Background

The full Phase 2B specification includes dependencies on systems not yet implemented:
- Event filter subsystem (Phase 2C)
- Handler registry (future enhancement)
- Performance monitors (future enhancement)
- Error context system (partial - basic error handling exists)

This simplified plan implements Phase 2B features that work with our current Phase 1 infrastructure while providing a foundation for future enhancements.

## Simplified Phase 2B Goals

1. **Event Processing Control** - Add start/stop/pause controls
2. **Batch Processing** - Process multiple events efficiently
3. **Processing Limits** - Time and count limits per cycle
4. **Enhanced Statistics** - Per-event-type counters and timing
5. **Processing Callbacks** - Pre/post event processing hooks

## Implementation Plan

### Part 1: Enhanced Statistics (~200 lines)

Add detailed statistics tracking to the existing event system.

**Structures to Add:**

```c
// Per-event-type statistics
typedef struct {
    lle_event_kind_t event_type;
    uint64_t count;                  // Total events of this type
    uint64_t total_processing_time;  // Total microseconds spent
    uint64_t min_processing_time;    // Minimum processing time
    uint64_t max_processing_time;    // Maximum processing time
    uint64_t last_processed_time;    // Last processing timestamp
} lle_event_type_stats_t;

// Enhanced system statistics
typedef struct {
    // Cycle statistics
    uint64_t cycles_completed;
    uint64_t total_cycle_time;
    uint64_t min_cycle_time;
    uint64_t max_cycle_time;
    
    // Per-type statistics (dynamically sized array)
    lle_event_type_stats_t *type_stats;
    size_t type_stats_count;
    size_t type_stats_capacity;
    
    // Queue depth tracking
    uint64_t max_queue_depth_seen;
    uint64_t max_priority_queue_depth_seen;
    
    // Processing limits tracking
    uint64_t cycles_hit_time_limit;
    uint64_t cycles_hit_event_limit;
} lle_event_enhanced_stats_t;
```

**Functions to Add:**
- `lle_event_stats_init()` - Initialize enhanced statistics
- `lle_event_stats_destroy()` - Cleanup statistics
- `lle_event_stats_record_event()` - Record event processing
- `lle_event_stats_update_cycle()` - Update cycle statistics
- `lle_event_stats_get_type_stats()` - Query type-specific stats
- `lle_event_stats_reset()` - Reset statistics

**Integration:**
- Add `lle_event_enhanced_stats_t *enhanced_stats` to `lle_event_system_t`
- Update `event_dispatch()` to record per-event timing
- Update `event_process_queue()` to track cycles

### Part 2: Processing Control (~150 lines)

Add processing control state and configuration.

**Structures to Add:**

```c
// Processing configuration
typedef struct {
    uint32_t max_events_per_cycle;   // Maximum events per processing cycle
    uint64_t cycle_time_limit_us;    // Time limit per cycle (microseconds)
    bool auto_process;               // Automatically process when events queued
    bool record_detailed_stats;      // Enable detailed statistics
} lle_event_processing_config_t;

// Processing state
typedef enum {
    LLE_PROCESSING_STOPPED,
    LLE_PROCESSING_RUNNING,
    LLE_PROCESSING_PAUSED
} lle_processing_state_t;
```

**Functions to Add:**
- `lle_event_processing_config_init()` - Initialize with defaults
- `lle_event_processing_set_config()` - Update configuration
- `lle_event_processing_get_config()` - Query configuration
- `lle_event_processing_set_state()` - Set processing state
- `lle_event_processing_get_state()` - Query processing state

**Integration:**
- Add config and state fields to `lle_event_system_t`
- Update `event_process_queue()` to respect state and limits

### Part 3: Batch Processing API (~150 lines)

Add efficient batch processing with callbacks.

**Callback Types:**

```c
// Pre-processing callback (called before event dispatch)
typedef lle_result_t (*lle_event_pre_process_fn)(lle_event_t *event, void *user_data);

// Post-processing callback (called after event dispatch)
typedef lle_result_t (*lle_event_post_process_fn)(lle_event_t *event, 
                                                   lle_result_t dispatch_result,
                                                   void *user_data);

// Cycle callback (called at end of processing cycle)
typedef void (*lle_event_cycle_callback_fn)(uint32_t events_processed,
                                           uint64_t cycle_time_us,
                                           void *user_data);
```

**Functions to Add:**
- `lle_event_set_pre_process_callback()` - Set pre-processing hook
- `lle_event_set_post_process_callback()` - Set post-processing hook
- `lle_event_set_cycle_callback()` - Set cycle completion hook
- `lle_event_process_batch()` - Process events with callbacks
- `lle_event_process_until_empty()` - Process all queued events
- `lle_event_process_with_timeout()` - Process with time limit

**Integration:**
- Add callback function pointers to `lle_event_system_t`
- Update `event_process_queue()` to invoke callbacks
- Create new batch processing functions

### Part 4: Event Processor Wrapper (~300 lines)

Create a lightweight event processor that wraps our enhanced event system.

**Structure:**

```c
// Simplified event processor
typedef struct {
    lle_event_system_t *event_system;  // Underlying event system
    
    // Processing control
    lle_processing_state_t state;
    lle_event_processing_config_t config;
    
    // Callbacks
    lle_event_pre_process_fn pre_process;
    void *pre_process_data;
    lle_event_post_process_fn post_process;
    void *post_process_data;
    lle_event_cycle_callback_fn cycle_callback;
    void *cycle_callback_data;
    
    // Current processing cycle state
    lle_event_t *current_event;
    uint32_t events_processed_this_cycle;
    uint64_t cycle_start_time;
    
    // Error tracking
    uint32_t consecutive_errors;
    uint64_t last_error_time;
} lle_event_processor_t;
```

**Functions to Add:**
- `lle_event_processor_init()` - Initialize processor with event system
- `lle_event_processor_destroy()` - Cleanup processor
- `lle_event_processor_start()` - Start processing
- `lle_event_processor_stop()` - Stop processing
- `lle_event_processor_pause()` - Pause processing
- `lle_event_processor_resume()` - Resume processing
- `lle_event_processor_process_cycle()` - Process one cycle
- `lle_event_processor_process_single()` - Process single event
- `lle_event_processor_get_stats()` - Get processing statistics
- `lle_event_processor_reset_stats()` - Reset statistics

## File Organization

### New Files
- `include/lle/event_processor.h` - Event processor public API (~150 lines)
- `src/lle/event_processor.c` - Event processor implementation (~450 lines)

### Modified Files
- `include/lle/event_system.h` - Add statistics and config structures (~100 lines added)
- `src/lle/event_system.c` - Enhanced statistics integration (~50 lines added)

**Total Estimated:** ~750 lines

## Success Criteria

✅ Event processor can start/stop/pause processing  
✅ Batch processing with configurable limits (time, count)  
✅ Pre/post processing callbacks work correctly  
✅ Cycle callbacks invoked at appropriate times  
✅ Enhanced statistics track per-type counts and timing  
✅ Statistics track cycle timing (min/max/average)  
✅ Queue depth tracking works  
✅ Processing limits enforced (time and event count)  
✅ All existing tests still pass (backward compatibility)  
✅ New processor tests validate all features  

## Implementation Order

1. **Enhanced Statistics** - Add structures and tracking to event_system
2. **Processing Configuration** - Add config structures and state management
3. **Event Processor Header** - Define public API
4. **Event Processor Core** - Implement init/destroy/lifecycle
5. **Batch Processing** - Implement process_cycle with callbacks
6. **Statistics Integration** - Wire statistics into processing
7. **Testing** - Add comprehensive processor tests

## Testing Strategy

**Unit Tests:**
- Processor lifecycle (init/destroy)
- State transitions (stop/start/pause/resume)
- Configuration set/get
- Callback invocation
- Statistics accuracy
- Limit enforcement

**Integration Tests:**
- Process events through processor
- Verify statistics match actual processing
- Test with priority queue
- Test with various event types

## Backward Compatibility

This implementation maintains 100% backward compatibility:
- Event processor is optional (can use event_system directly)
- All Phase 1 APIs unchanged
- Statistics are opt-in (minimal overhead when disabled)
- Existing tests continue to pass

## Future Enhancements (Not in Phase 2B)

These can be added in later phases:
- Event filtering (Phase 2C)
- State machine support (Phase 2C)
- Handler registry with priorities
- Performance monitors
- Advanced error recovery
- Asynchronous processing

## Notes

This simplified approach:
- Works with current Phase 1 infrastructure
- Provides immediate value (statistics, batch processing, callbacks)
- Creates foundation for future phases
- Maintains zero-tolerance compliance (no TODOs or stubs)
- Keeps implementation manageable (~750 lines vs. ~900 in original plan)
