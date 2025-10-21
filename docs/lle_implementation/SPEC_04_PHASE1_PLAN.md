# Spec 04 Phase 1 Implementation Plan

**Specification**: 04_event_system_complete.md  
**Phase**: Phase 1 - Core Event Infrastructure  
**Status**: PLANNING  
**Estimated**: ~600-800 lines, 15-20 functions

---

## Analysis: Spec 04 Complexity

### Full Spec Scope (1,370 lines)

The complete Spec 04 describes an **extremely complex** event-driven architecture with:

**8 Major Subsystems**:
1. Event System (main coordinator)
2. Event Queue (circular buffer, priority scheduling)
3. Event Processor (processing engine)
4. Event Handler Registry (handler management)
5. Event Filter (filtering system)
6. Event Statistics (performance tracking)
7. Event Lifecycle Management
8. Lusush Display Integration

**40+ Event Types** across 12 categories
**Complex Data Structures** with union-based event data
**Thread Safety** with mutexes
**Memory Pool Integration** with dedicated pools
**Performance Monitoring** integration

**Estimated Full Implementation**: ~3,000-4,000 lines (too large for single phase)

---

## Phase 1 Strategy: Core Event Infrastructure Only

### What Phase 1 MUST Provide

**Minimum functionality for Spec 06 and Spec 08 to use**:
1. Create events with basic types
2. Queue events (simple FIFO queue)
3. Dispatch events to handlers
4. Register/unregister handlers
5. Basic event lifecycle

### What Phase 1 CAN Defer

**Complex features for Phase 2+**:
- Priority queues (can use single queue)
- Event filtering system
- Advanced statistics
- Performance monitoring integration
- Lusush display integration
- Plugin event support
- Asynchronous processing
- Complex error recovery
- State machines
- Timer events

---

## Phase 1 Scope Definition

### Core Event Types (Phase 1)

Focus on **essential input events** only:

```c
typedef enum {
    // Terminal Input Events
    LLE_EVENT_KEY_PRESS = 0x1000,
    LLE_EVENT_KEY_SEQUENCE,
    
    // Terminal State Events  
    LLE_EVENT_TERMINAL_RESIZE = 0x3000,
    
    // Buffer Events (for Spec 03)
    LLE_EVENT_BUFFER_CHANGED = 0x5000,
    LLE_EVENT_CURSOR_MOVED,
    
    // Display Events (for Spec 08)
    LLE_EVENT_DISPLAY_UPDATE = 0xC000,
    LLE_EVENT_DISPLAY_REFRESH,
    
    // System Events
    LLE_EVENT_SYSTEM_ERROR = 0x9000,
    LLE_EVENT_SYSTEM_SHUTDOWN,
} lle_event_type_t;  // Phase 1: 9 event types
```

### Simplified Event Structure (Phase 1)

```c
// Phase 1: Simplified event structure
typedef struct lle_event {
    lle_event_type_t type;
    uint64_t sequence_number;
    uint64_t timestamp;
    
    // Simplified data (no union, just pointer)
    void *data;
    size_t data_size;
    
    // Queue linkage
    struct lle_event *next;
} lle_event_t;
```

### Core Components (Phase 1)

**1. Event Queue** (simple circular buffer):
```c
typedef struct {
    lle_event_t **events;      // Array of event pointers
    size_t capacity;           // Queue capacity
    size_t head;               // Read position
    size_t tail;               // Write position  
    size_t count;              // Current count
    pthread_mutex_t mutex;     // Thread safety
} lle_event_queue_t;
```

**2. Event Handler**:
```c
typedef lle_result_t (*lle_event_handler_fn)(lle_event_t *event, void *user_data);

typedef struct {
    lle_event_type_t event_type;
    lle_event_handler_fn handler;
    void *user_data;
    char name[64];
} lle_event_handler_t;
```

**3. Event System** (simplified):
```c
typedef struct {
    lle_event_queue_t *queue;          // Single queue (no priority)
    lle_event_handler_t **handlers;    // Handler array
    size_t handler_count;
    size_t handler_capacity;
    
    lle_memory_pool_t *event_pool;     // Event memory pool
    pthread_mutex_t system_mutex;
    
    uint64_t sequence_counter;
    bool active;
} lle_event_system_t;
```

---

## Phase 1 Function List (15-20 functions)

### Event System Lifecycle (4 functions)

```c
lle_result_t lle_event_system_init(lle_event_system_t **system, lle_memory_pool_t *pool);
void lle_event_system_destroy(lle_event_system_t *system);
lle_result_t lle_event_system_start(lle_event_system_t *system);
lle_result_t lle_event_system_stop(lle_event_system_t *system);
```

### Event Creation and Destruction (3 functions)

```c
lle_result_t lle_event_create(lle_event_system_t *system, lle_event_type_t type, 
                               void *data, size_t data_size, lle_event_t **event);
void lle_event_destroy(lle_event_system_t *system, lle_event_t *event);
lle_result_t lle_event_clone(lle_event_t *source, lle_event_t **dest);
```

### Event Queue Operations (4 functions)

```c
lle_result_t lle_event_queue_init(lle_event_queue_t **queue, size_t capacity);
void lle_event_queue_destroy(lle_event_queue_t *queue);
lle_result_t lle_event_enqueue(lle_event_system_t *system, lle_event_t *event);
lle_result_t lle_event_dequeue(lle_event_system_t *system, lle_event_t **event);
```

### Handler Management (4 functions)

```c
lle_result_t lle_event_handler_register(lle_event_system_t *system,
                                         lle_event_type_t type,
                                         lle_event_handler_fn handler,
                                         void *user_data,
                                         const char *name);
lle_result_t lle_event_handler_unregister(lle_event_system_t *system,
                                           lle_event_type_t type,
                                           const char *name);
lle_result_t lle_event_handler_unregister_all(lle_event_system_t *system,
                                               lle_event_type_t type);
size_t lle_event_handler_count(lle_event_system_t *system, lle_event_type_t type);
```

### Event Processing (3 functions)

```c
lle_result_t lle_event_dispatch(lle_event_system_t *system, lle_event_t *event);
lle_result_t lle_event_process_queue(lle_event_system_t *system, uint32_t max_events);
lle_result_t lle_event_process_all(lle_event_system_t *system);
```

### Utility Functions (2 functions)

```c
uint64_t lle_event_get_timestamp_us(void);
const char *lle_event_type_name(lle_event_type_t type);
```

**Total Phase 1**: ~20 functions

---

## Phase 1 Implementation Files

```
include/lle/event_system.h          # Phase 1 header (~400 lines)
src/lle/event_system.c              # Main system (~300 lines)
src/lle/event_queue.c               # Queue operations (~200 lines)
src/lle/event_handlers.c            # Handler management (~250 lines)
```

**Total Phase 1 Estimate**: ~700-800 lines

---

## What Phase 1 Enables

**For Spec 06 (Input Parsing)**:
- Can create KEY_PRESS events
- Can dispatch input events to handlers
- Can register input handlers

**For Spec 08 (Display Integration)**:
- Can create DISPLAY_UPDATE events
- Can dispatch display events
- Can register display handlers

**For Spec 03 (Buffer Management)**:
- Can create BUFFER_CHANGED events
- Can notify buffer changes

**For Terminal (Spec 02)**:
- Can create TERMINAL_RESIZE events
- Can dispatch resize notifications

---

## Phase 2+ Deferred Features

### Phase 2: Advanced Features (~1,000 lines)
- Priority queues (separate high/low priority)
- Event filtering system
- Advanced event data types (mouse, paste, etc.)
- Event statistics and metrics
- Comprehensive error handling

### Phase 3: Integration (~800 lines)
- Lusush display integration
- Plugin event support
- Timer events
- Async processing
- State machine integration

### Phase 4: Optimization (~500 lines)
- Performance monitoring integration
- Lock-free queue optimization
- Event batching
- Advanced memory management

**Total Spec 04**: ~3,000-3,500 lines across all phases

---

## Success Criteria (Phase 1)

### Functionality
- ✅ Can create and destroy events
- ✅ Can queue and dequeue events (FIFO)
- ✅ Can register/unregister handlers
- ✅ Can dispatch events to all registered handlers
- ✅ Thread-safe queue operations

### Quality
- ✅ All functions complete (no stubs)
- ✅ Compiles cleanly
- ✅ Zero memory leaks
- ✅ Proper error handling

### Performance
- ✅ Event creation < 10μs
- ✅ Event dispatch < 100μs
- ✅ Queue operations < 5μs

### Testing
- ✅ Can create events
- ✅ Can queue/dequeue events
- ✅ Can dispatch to handlers
- ✅ Thread safety verified

---

## Dependencies

**Phase 1 Needs** (HAVE):
- ✅ Spec 16: Error handling
- ✅ Spec 15: Memory management (for event pool)
- Standard libraries: pthread.h, time.h

**Phase 1 Provides** (FOR FUTURE SPECS):
- Basic event infrastructure for Spec 06, 08, 03
- Handler registration system
- Event dispatch mechanism

---

## Implementation Order

1. **event_system.h** - Define Phase 1 structures and API
2. **event_queue.c** - Implement circular buffer queue
3. **event_handlers.c** - Implement handler registry
4. **event_system.c** - Implement main system + dispatch

---

## Estimated Effort

**Implementation**: 2-3 days  
**Testing**: 0.5 days  
**Total**: 2.5-3.5 days for Phase 1

**Phase 1 delivers**: Core event infrastructure enabling Specs 03, 06, 08 to proceed.

---

**Status**: Ready to begin Phase 1 implementation  
**Next**: Create event_system.h with Phase 1 API
