# Spec 04 Phase 2 Implementation Plan

**Specification**: 04_event_system_complete.md  
**Phase**: Phase 2 - Advanced Event Infrastructure  
**Status**: PLANNING  
**Estimated**: ~2,900 lines across 4 new files, 6-8 weeks

---

## Overview

Phase 1 provided minimum event system functionality for Spec 06 and Spec 08. Phase 2 completes the full event system specification with advanced features including priority queues, event processing engine, filtering, state machines, and timer events.

---

## Phase 1 Recap (Completed)

### What Phase 1 Delivered
- **Basic Event Types**: 9 essential event types
- **Single FIFO Queue**: Circular buffer implementation
- **Handler Registry**: Register/unregister event handlers
- **Basic Lifecycle**: Create, enqueue, dispatch, destroy
- **Thread Safety**: Mutex-protected operations
- **Files**: event_system.c (7KB), event_queue.c (4KB), event_handlers.c (7KB)
- **Tests**: 35/35 passing

### What Phase 1 Deferred
- Priority queues
- Event filtering
- Advanced statistics
- Event processor subsystem
- State machine support
- Timer events
- Asynchronous processing
- 40+ additional event types

---

## Phase 2 Sub-Phases

### Phase 2A: Priority Queue + Expanded Event Types (Weeks 1-2)

**Goal**: Add priority-based event processing and complete event type coverage

**Work Items**:
1. **Expand Event Types** (header updates)
   - Add 40+ event types from spec (12 categories)
   - Add event priority enum
   - Add event source enum
   - Update event structure with priority field

2. **Priority Queue Implementation**
   - Modify event_queue.c to support priority levels
   - Add second priority queue to event_system
   - Implement priority-based dequeue strategy
   - Update enqueue to route by priority

3. **Update Event System**
   - Initialize both queues (priority + main)
   - Route events by priority level
   - Update processing to check priority queue first
   - Update statistics for dual queues

**Estimated**: ~600 lines, 15 functions  
**Files Modified**: include/lle/event_system.h, src/lle/event_queue.c, src/lle/event_system.c  
**New Tests**: Priority queue tests, event routing tests

---

### Phase 2B: Event Processor + Enhanced Stats (Weeks 3-4)

**Goal**: Add dedicated event processing engine with comprehensive statistics

**Work Items**:
1. **Event Processor Subsystem** (NEW: event_processor.c)
   - Dedicated event processing engine
   - Asynchronous processing support
   - Batch processing optimization
   - Processing control (start/stop/pause)
   - Integration with event queues

2. **Enhanced Statistics** (event_stats.c updates)
   - Per-type event counters
   - Processing latency histograms
   - Queue depth monitoring over time
   - Handler execution time tracking
   - Statistics query API

3. **Integration**
   - Wire processor into event_system
   - Update event_process_queue to use processor
   - Add statistics collection hooks
   - Performance monitoring integration

**Estimated**: ~900 lines, 25 functions  
**Files Created**: src/lle/event_processor.c  
**Files Modified**: src/lle/event_system.c, include/lle/event_system.h  
**New Tests**: Processor tests, statistics tests, async processing tests

---

### Phase 2C: Event Filter + State Machine (Weeks 5-6)

**Goal**: Add event filtering and state machine support

**Work Items**:
1. **Event Filter Subsystem** (NEW: event_filter.c)
   - Filter rule registration
   - Pre-queue filtering (performance)
   - Conditional event blocking
   - Event transformation
   - Filter statistics

2. **State Machine Support** (NEW: event_state_machine.c)
   - State machine definition structures
   - State-based event handling
   - State transition on events
   - State-specific handler registration
   - State history tracking

3. **Integration**
   - Wire filter into enqueue path
   - Add state machine to event_system
   - State machine event routing
   - Filter performance monitoring

**Estimated**: ~1,000 lines, 30 functions  
**Files Created**: src/lle/event_filter.c, src/lle/event_state_machine.c  
**Files Modified**: src/lle/event_system.c, include/lle/event_system.h  
**New Tests**: Filter tests, state machine tests, integration tests

---

### Phase 2D: Timer Events + Testing (Weeks 7-8)

**Goal**: Add timer-based event delivery and comprehensive testing

**Work Items**:
1. **Timer Event Subsystem** (NEW: event_timer.c)
   - Timer registration/cancellation
   - Delayed event delivery
   - Periodic event generation
   - Timer heap implementation (efficient)
   - Timer expiration checking

2. **Comprehensive Testing**
   - Phase 2A tests (priority, routing)
   - Phase 2B tests (processor, stats)
   - Phase 2C tests (filter, state machine)
   - Phase 2D tests (timers)
   - Integration tests (all subsystems)
   - Performance benchmarks
   - Stress tests

3. **Documentation**
   - Update API documentation
   - Phase 2 lessons learned
   - Performance analysis
   - Usage examples

**Estimated**: ~400 lines (timer) + testing infrastructure  
**Files Created**: src/lle/event_timer.c, tests for all Phase 2 components  
**Files Modified**: All Phase 2 files for bug fixes  
**New Tests**: 40+ new tests covering all Phase 2 features

---

## Success Criteria

### Functionality
- ✅ All 40+ event types defined and usable
- ✅ Priority queue correctly routes events by priority
- ✅ Event processor handles async processing
- ✅ Event filter blocks/transforms events correctly
- ✅ State machine handles state-based events
- ✅ Timer events delivered at correct times

### Performance
- ✅ Event creation: < 1μs (same as Phase 1)
- ✅ Priority queue overhead: < 0.5μs additional
- ✅ Filter overhead: < 0.2μs per event
- ✅ Timer check overhead: < 0.1μs per process cycle
- ✅ Overall event processing: < 500μs end-to-end

### Testing
- ✅ All Phase 1 tests still passing (35/35)
- ✅ All Phase 2 tests passing (40+ new tests)
- ✅ Integration tests passing
- ✅ Performance benchmarks meet targets
- ✅ Stress tests pass (10,000+ events/sec)

### Code Quality
- ✅ Zero compiler warnings (-Werror)
- ✅ Zero memory leaks (valgrind)
- ✅ 100% spec compliance
- ✅ Complete function documentation
- ✅ Living documents updated

---

## Dependencies

**Required (All Complete)**:
- ✅ Spec 16: Error Handling
- ✅ Spec 15: Memory Management
- ✅ Spec 14: Performance Monitoring
- ✅ Spec 17: Testing Framework
- ✅ Spec 04 Phase 1: Basic Event System

**Optional (Not Required)**:
- ❌ Spec 07: Extensibility (for plugin events - deferred)
- ❌ Spec 09: History (for history events - just types)
- ❌ Spec 12: Completion (for completion events - just types)

---

## Implementation Notes

### Event Type Philosophy
- Event types for unimplemented systems (history, completion, plugins) are defined but unused
- These events can be created and queued, but no handlers exist yet
- When those systems are implemented, they simply register handlers for their event types
- This forward compatibility is intentional and correct

### Thread Safety
- All Phase 2 components must be thread-safe
- Use existing pthread_mutex from event_system
- Add rwlock for read-heavy operations (stats, filter rules)
- Document locking order to prevent deadlocks

### Performance First
- Filter runs BEFORE enqueue (saves queue space)
- Priority queue uses efficient data structure
- Timer heap for O(log n) operations
- Statistics use lock-free counters where possible

### Zero Tolerance Compliance
- NO stubs or TODOs
- Complete implementation of all functions
- Comprehensive error handling
- Full test coverage

---

## Next Steps

1. Begin Phase 2A: Update event types in header
2. Implement priority queue modifications
3. Test priority routing
4. Proceed to Phase 2B once 2A complete

---

**Status**: Ready to begin Phase 2A  
**Blockers**: None  
**Estimated Completion**: 6-8 weeks from start
