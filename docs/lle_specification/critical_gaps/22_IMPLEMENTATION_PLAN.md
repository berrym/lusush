# Spec 22: History-Buffer Integration - Complete Implementation Plan

**Document Version**: 1.1.0  
**Date**: 2025-11-02  
**Based on**: Spec 22 History-Buffer Integration Complete Specification (1596 lines)  
**Status**: Phase 1-4 Complete (Performance Optimization Complete)

---

## Executive Summary

This implementation plan provides a complete roadmap for implementing Spec 22: History-Buffer Integration, the critical missing functionality from LLE_DESIGN_DOCUMENT.md. This specification implements seamless integration between the history system and buffer management for interactive editing of historical commands with complete multiline structure preservation.

**Key Deliverables**:
- Interactive history editing with callback framework
- Multiline command reconstruction with intelligent formatting
- Structure-aware shell construct handling
- Seamless buffer-history integration layer
- High-performance caching and optimization

**Total Estimated Effort**: 4-6 weeks  
**Total Estimated LOC**: ~4,500-5,500 lines of production code + tests

---

## Implementation Status

### Phase 1: Core Infrastructure ✅ COMPLETE
- System lifecycle (create/destroy/config)
- Thread synchronization
- Callback framework
- State management

### Phase 2: Multiline Reconstruction Engine ✅ COMPLETE
- Command structure analysis (command_structure.h/c)
- Shell construct detection (structure_analyzer.h/c)
- Multiline parsing (multiline_parser.h/c)
- Intelligent reconstruction (reconstruction_engine.h/c)
- Multiple formatting styles (formatting_engine.h/c)

### Phase 3: Interactive Editing System ✅ COMPLETE
- Edit session management (edit_session_manager.h/c)
- History-buffer bridge (history_buffer_bridge.h/c)
- Interactive editing API (lle_history_edit_entry, session_complete, session_cancel)
- Event callbacks integration

### Phase 4: Performance Optimization ✅ COMPLETE (2025-11-02)
- LRU cache system (edit_cache.h/c)
- Cache hit/miss tracking
- Cache invalidation on edits
- Performance monitoring API
- Statistics and maintenance functions

**Modules Implemented**: 74 total (11 new modules for Phases 2-4)  
**Production Code**: ~5,100 lines  
**Compilation Status**: All modules compiling successfully

---

## Table of Contents

1. [Core Data Structures](#1-core-data-structures)
2. [Core Functions](#2-core-functions)
3. [Dependencies Analysis](#3-dependencies-analysis)
4. [Phase 1: Core Integration Infrastructure](#4-phase-1-core-integration-infrastructure)
5. [Phase 2: Multiline Reconstruction Engine](#5-phase-2-multiline-reconstruction-engine)
6. [Phase 3: Interactive Editing System](#6-phase-3-interactive-editing-system)
7. [Phase 4: Performance Optimization](#7-phase-4-performance-optimization)
8. [Phase 5: Testing and Validation](#8-phase-5-testing-and-validation)
9. [Success Criteria](#9-success-criteria)
10. [Risk Analysis](#10-risk-analysis)
11. [Implementation Timeline](#11-implementation-timeline)

---

## 1. Core Data Structures

### 1.1 Primary Integration Structure

```c
// File: include/lle/history_buffer_integration.h
typedef struct lle_history_buffer_integration {
    // Core components
    lle_history_system_t *history_system;             // History system reference
    lle_buffer_t *editing_buffer;                     // Buffer system reference
    lle_reconstruction_engine_t *reconstruction;     // Command reconstruction engine
    lle_edit_session_manager_t *session_manager;     // Edit session management
    
    // Multiline support
    lle_multiline_parser_t *multiline_parser;        // Multiline structure parser
    lle_structure_analyzer_t *structure_analyzer;    // Shell construct analyzer
    lle_formatting_engine_t *formatter;              // Intelligent formatting engine
    
    // Callback system
    lle_history_edit_callbacks_t *edit_callbacks;    // Edit event callbacks
    lle_callback_registry_t *callback_registry;      // Callback management
    
    // Performance optimization
    lle_edit_cache_t *edit_cache;                     // Edit operation caching
    lle_memory_pool_t *memory_pool;                   // Memory pool integration
    lle_performance_metrics_t *metrics;               // Performance monitoring
    
    // Configuration and state
    lle_integration_config_t *config;                // Integration configuration
    lle_integration_state_t *current_state;          // Current integration state
    
    // Synchronization
    pthread_rwlock_t integration_lock;               // Thread-safe access
    bool system_active;                              // Integration system status
    uint64_t session_counter;                        // Edit session counter
} lle_history_buffer_integration_t;
```

**Size**: ~200 bytes + dynamically allocated subsystems  
**Alignment**: 8-byte aligned  
**Lifecycle**: Created once per shell instance

### 1.2 Enhanced History Entry Structure

```c
// File: include/lle/history.h (extension)
typedef struct lle_history_entry {
    // Core entry data (EXISTING in Spec 09)
    uint64_t entry_id;                                // Unique entry identifier
    char *command;                                    // Normalized command text
    char *original_multiline;                         // CRITICAL: Original multiline formatting preserved
    bool is_multiline;                                // Multiline structure flag
    size_t command_length;                            // Command text length
    size_t original_length;                           // Original multiline length
    
    // Structural information for reconstruction (NEW)
    lle_command_structure_t *structure_info;          // Shell construct structure
    lle_indentation_info_t *indentation;             // Original indentation patterns
    lle_line_mapping_t *line_mapping;                // Line boundary mapping
    
    // Execution context (EXISTING in Spec 09)
    uint64_t timestamp;                               // Command execution timestamp
    uint32_t duration_ms;                             // Execution duration
    int exit_code;                                    // Command exit status
    char *working_directory;                          // Execution directory
    session_id_t session_id;                          // Session identifier
    
    // Edit history tracking (NEW)
    uint32_t edit_count;                              // Number of times edited
    uint64_t last_edited;                             // Last edit timestamp
    bool has_been_edited;                             // Edit history flag
    
    // Classification and metadata (EXISTING in Spec 09)
    lle_command_type_t command_type;                  // Command classification
    bool is_private;                                  // Sensitive command flag
    uint32_t access_count;                            // Access frequency
    uint64_t last_accessed;                           // Last access time
} lle_history_entry_t;
```

**Size**: ~120-200 bytes + dynamic allocations  
**Dependencies**: Extends existing Spec 09 history entry  
**Backward Compatibility**: Fully compatible - adds optional fields

### 1.3 Command Structure Analysis

```c
// File: include/lle/command_structure.h
typedef enum lle_command_construct_type {
    LLE_CONSTRUCT_SIMPLE,                             // Simple single command
    LLE_CONSTRUCT_PIPELINE,                           // Command pipeline
    LLE_CONSTRUCT_FOR_LOOP,                           // For loop construct
    LLE_CONSTRUCT_WHILE_LOOP,                         // While loop construct
    LLE_CONSTRUCT_IF_STATEMENT,                       // If/then/else statement
    LLE_CONSTRUCT_CASE_STATEMENT,                     // Case statement
    LLE_CONSTRUCT_FUNCTION_DEF,                       // Function definition
    LLE_CONSTRUCT_SUBSHELL,                           // Subshell execution
    LLE_CONSTRUCT_COMMAND_SUBSTITUTION,               // Command substitution
    LLE_CONSTRUCT_HEREDOC,                            // Here document
    LLE_CONSTRUCT_COMPOUND,                           // Compound statement
    LLE_CONSTRUCT_CONDITIONAL                         // Conditional execution
} lle_command_construct_type_t;

typedef struct lle_command_structure {
    lle_command_construct_type_t construct_type;      // Type of shell construct
    
    // Structure boundaries
    size_t start_offset;                              // Start position in original text
    size_t end_offset;                                // End position in original text
    size_t line_start;                                // Starting line number
    size_t line_end;                                  // Ending line number
    
    // Indentation and formatting
    lle_indentation_pattern_t *indentation;          // Indentation pattern
    lle_line_continuation_t *continuations;          // Line continuation info
    size_t base_indent_level;                         // Base indentation level
    
    // Nested structures
    struct lle_command_structure **nested_constructs; // Nested command structures
    size_t nested_count;                              // Number of nested constructs
    
    // Keywords and syntax elements
    lle_keyword_position_t *keywords;                // Positions of shell keywords
    size_t keyword_count;                             // Number of keywords
    
    // Context information
    bool requires_completion;                         // Needs completion (like 'done', 'fi')
    char **completion_keywords;                       // Required completion keywords
    size_t completion_count;                          // Number of completion keywords needed
} lle_command_structure_t;
```

**Size**: ~150-300 bytes + nested structures  
**Complexity**: Recursive structure for nested constructs

### 1.4 Edit Session Management

```c
// File: include/lle/edit_session.h
typedef struct lle_edit_session {
    uint64_t session_id;                              // Unique session identifier
    lle_history_entry_t *original_entry;             // Original history entry
    lle_buffer_t *editing_buffer;                     // Buffer being edited
    lle_history_edit_callbacks_t *callbacks;         // Session callbacks
    void *user_data;                                  // User context data
    
    // Session state
    bool active;                                      // Session active flag
    uint64_t start_time;                              // Session start timestamp
    uint64_t last_activity;                           // Last edit activity
    
    // Edit tracking
    lle_change_list_t *changes;                       // List of edits made
    size_t change_count;                              // Number of changes
    bool has_modifications;                           // Modification flag
    
    // Multiline reconstruction context
    lle_multiline_context_t *multiline_context;      // Multiline editing context
    lle_structure_preservation_t *structure_state;   // Structure preservation state
    
    // Memory management
    lle_memory_pool_t *session_memory_pool;          // Session-specific memory pool
} lle_edit_session_t;
```

**Size**: ~120 bytes + session-specific allocations  
**Lifecycle**: Created per edit session, destroyed on completion/cancel

### 1.5 Callback Framework

```c
// File: include/lle/history_callbacks.h
typedef struct lle_history_edit_callbacks {
    lle_result_t (*on_edit_start)(lle_history_entry_t *entry, void *user_data);
    lle_result_t (*on_edit_complete)(lle_history_entry_t *entry, void *user_data);
    lle_result_t (*on_edit_cancel)(lle_history_entry_t *entry, void *user_data);
    
    // Extended callbacks for advanced editing
    lle_result_t (*on_buffer_loaded)(lle_buffer_t *buffer, lle_history_entry_t *entry, void *user_data);
    lle_result_t (*on_structure_reconstructed)(lle_multiline_info_t *multiline, void *user_data);
    lle_result_t (*on_edit_modified)(lle_buffer_t *buffer, lle_edit_change_t *change, void *user_data);
    lle_result_t (*on_save_requested)(lle_buffer_t *buffer, lle_history_entry_t *entry, void *user_data);
    
    void *user_data;                                  // User-provided context data
} lle_history_edit_callbacks_t;
```

**Size**: 64 bytes (8 function pointers + user_data pointer)  
**Usage**: Passed to `lle_history_edit_entry()` for event notifications

### 1.6 Performance Caching

```c
// File: include/lle/edit_cache.h
typedef struct lle_edit_cache {
    lle_hash_table_t *reconstruction_cache;          // Cache for reconstructed content
    lle_hash_table_t *structure_cache;               // Cache for analyzed structures
    lle_lru_cache_t *recent_edits_cache;             // LRU cache for recent edits
    
    // Cache statistics
    uint64_t reconstruction_hits;                     // Reconstruction cache hits
    uint64_t reconstruction_misses;                   // Reconstruction cache misses
    uint64_t structure_hits;                          // Structure cache hits  
    uint64_t structure_misses;                        // Structure cache misses
    
    // Memory management
    lle_memory_pool_t *cache_memory_pool;            // Dedicated cache memory pool
    size_t max_cache_size;                            // Maximum cache size
    size_t current_cache_size;                        // Current cache size
    
    // Performance tuning
    uint32_t reconstruction_ttl_seconds;             // Reconstruction TTL
    uint32_t structure_ttl_seconds;                   // Structure analysis TTL
    bool enable_aggressive_caching;                   // Aggressive caching flag
} lle_edit_cache_t;
```

**Size**: ~100 bytes + cache storage  
**Memory Overhead**: Configurable (default 1MB cache limit)

---

## 2. Core Functions

### 2.1 Integration Lifecycle

```c
// File: src/lle/history_buffer_integration.c

/**
 * Create and initialize history-buffer integration system
 * 
 * @param integration Output pointer for integration system
 * @param history_system History system instance
 * @param editing_buffer Buffer for editing operations
 * @param memory_pool Memory pool for allocations
 * @param config Configuration (NULL for defaults)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_buffer_integration_create(
    lle_history_buffer_integration_t **integration,
    lle_history_system_t *history_system,
    lle_buffer_t *editing_buffer,
    lle_memory_pool_t *memory_pool,
    const lle_integration_config_t *config);

/**
 * Destroy integration system and free all resources
 * 
 * @param integration Integration system to destroy
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_buffer_integration_destroy(
    lle_history_buffer_integration_t *integration);
```

**Estimated LOC**: 150-200 lines  
**Complexity**: Medium (resource management, initialization)

### 2.2 Interactive History Editing (PRIMARY FUNCTION)

```c
// File: src/lle/history_editing.c

/**
 * Edit history entry with interactive buffer loading and callbacks
 * 
 * CRITICAL IMPLEMENTATION from LLE_DESIGN_DOCUMENT.md
 * 
 * @param integration History-buffer integration system
 * @param entry_index Entry index to edit
 * @param callbacks Callback functions for edit lifecycle
 * @param user_data User context data passed to callbacks
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_edit_entry(
    lle_history_buffer_integration_t *integration,
    size_t entry_index,
    lle_history_edit_callbacks_t *callbacks,
    void *user_data);

/**
 * Complete active edit session and save/discard changes
 * 
 * @param integration History-buffer integration system
 * @param session Edit session to complete
 * @param save_changes true to save, false to discard
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_edit_session_complete(
    lle_history_buffer_integration_t *integration,
    lle_edit_session_t *session,
    bool save_changes);
```

**Estimated LOC**: 300-400 lines  
**Complexity**: High (session management, callbacks, error handling)  
**Critical Path**: This is THE core function from the design document

### 2.3 Multiline Reconstruction

```c
// File: src/lle/multiline_reconstruction.c

/**
 * Load history entry into buffer with complete multiline reconstruction
 * 
 * @param integration History-buffer integration system
 * @param entry History entry to load
 * @param target_buffer Buffer to load into
 * @param callbacks Optional callbacks for events
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_buffer_load_entry_with_reconstruction(
    lle_history_buffer_integration_t *integration,
    lle_history_entry_t *entry,
    lle_buffer_t *target_buffer,
    lle_history_edit_callbacks_t *callbacks);

/**
 * Reconstruct multiline command with intelligent formatting
 * 
 * @param engine Reconstruction engine
 * @param source_content Original content
 * @param source_length Content length
 * @param structure Command structure
 * @param reconstructed_content Output for reconstructed content
 * @param reconstructed_length Output for reconstructed length
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_reconstruction_engine_reconstruct_multiline(
    lle_reconstruction_engine_t *engine,
    const char *source_content,
    size_t source_length,
    lle_command_structure_t *structure,
    char **reconstructed_content,
    size_t *reconstructed_length);
```

**Estimated LOC**: 400-500 lines  
**Complexity**: High (parsing, indentation, structure preservation)

### 2.4 Structure Analysis

```c
// File: src/lle/structure_analyzer.c

/**
 * Analyze command structure for intelligent reconstruction
 * 
 * @param analyzer Structure analyzer
 * @param command_text Command text to analyze
 * @param text_length Text length
 * @param structure Output for command structure
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_structure_analyzer_analyze_command(
    lle_structure_analyzer_t *analyzer,
    const char *command_text,
    size_t text_length,
    lle_command_structure_t **structure);

/**
 * Calculate proper indentation for shell constructs
 * 
 * @param structure Command structure
 * @param line_number Line number to calculate for
 * @param current_level Current indentation level
 * @return Calculated indentation level
 */
size_t lle_calculate_target_indentation(
    lle_command_structure_t *structure,
    size_t line_number,
    size_t current_level);
```

**Estimated LOC**: 600-800 lines  
**Complexity**: Very High (shell syntax parsing, construct detection)

### 2.5 Buffer Integration Layer

```c
// File: src/lle/buffer_history_bridge.c

/**
 * Load history entry directly into buffer
 * 
 * @param buffer Buffer to load into
 * @param entry History entry
 * @param integration Integration system
 * @param preserve_multiline_structure Preserve multiline formatting
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_buffer_load_from_history_entry(
    lle_buffer_t *buffer,
    lle_history_entry_t *entry,
    lle_history_buffer_integration_t *integration,
    bool preserve_multiline_structure);

/**
 * Save buffer content back to history with multiline preservation
 * 
 * @param buffer Buffer with edited content
 * @param integration Integration system
 * @param original_entry Original history entry being replaced
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_buffer_save_to_history(
    lle_buffer_t *buffer,
    lle_history_buffer_integration_t *integration,
    lle_history_entry_t *original_entry);
```

**Estimated LOC**: 300-400 lines  
**Complexity**: Medium (buffer operations, content extraction)

### 2.6 Performance and Caching

```c
// File: src/lle/edit_cache.c

/**
 * Create edit cache system
 * 
 * @param cache Output for cache system
 * @param max_cache_size Maximum cache size in bytes
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_edit_cache_create(
    lle_edit_cache_t **cache,
    size_t max_cache_size,
    lle_memory_pool_t *memory_pool);

/**
 * Cache-aware multiline reconstruction
 * 
 * @param integration Integration system
 * @param entry History entry
 * @param reconstructed_content Output for reconstructed content
 * @param content_length Output for content length
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_cached_multiline_reconstruction(
    lle_history_buffer_integration_t *integration,
    lle_history_entry_t *entry,
    char **reconstructed_content,
    size_t *content_length);
```

**Estimated LOC**: 400-500 lines  
**Complexity**: High (caching algorithms, TTL management, LRU eviction)

---

## 3. Dependencies Analysis

### 3.1 Required Existing Systems

#### History System (Spec 09)
**Status**: Implemented (Phase 1-3 complete)  
**Required APIs**:
- `lle_history_core_t` - Core history engine
- `lle_history_entry_t` - History entry structure
- `lle_history_get_entry_by_index()` - Entry retrieval
- `lle_history_system_replace_entry()` - Entry replacement
- `lle_history_entry_create_modified()` - Modified entry creation

**Integration Points**:
- Read entries for editing
- Update entries after modification
- Preserve original_multiline field (NEW field to add)

#### Buffer Management (Spec 03)
**Status**: Phase 1 implemented, Phase 2-7 in progress  
**Required APIs**:
- `lle_buffer_t` - Core buffer structure
- `lle_buffer_create()` - Buffer creation
- `lle_buffer_clear()` - Buffer clearing
- `lle_buffer_set_content()` - Content loading
- `lle_buffer_get_complete_content()` - Content extraction
- `lle_buffer_is_multiline()` - Multiline detection
- `lle_buffer_move_cursor_to_buffer_end()` - Cursor positioning

**Integration Points**:
- Load historical commands into buffer
- Extract edited content from buffer
- Track buffer modifications

#### Event System (Spec 04)
**Status**: Phase 1, 2A, 2B, 2C, 2D complete  
**Required APIs**:
- `lle_event_system_t` - Event system
- `lle_event_create()` - Event creation
- `lle_event_system_publish()` - Event publishing
- `lle_event_system_register_handler()` - Handler registration

**Integration Points**:
- Publish edit start/complete/cancel events
- Notify of buffer load completion
- Signal structure reconstruction completion

#### Memory Management (Spec 02)
**Status**: Fully implemented  
**Required APIs**:
- `lle_memory_pool_t` - Memory pool
- `lle_memory_pool_alloc()` - Allocation
- `lle_memory_pool_free()` - Deallocation
- `lle_string_duplicate()` - String duplication

**Integration Points**:
- All dynamic allocations
- Session management
- Cache storage

### 3.2 New System Dependencies

#### Multiline Parser
**Status**: NEW - To be implemented  
**Purpose**: Parse shell constructs and detect multiline structures  
**Estimated LOC**: 800-1000 lines  
**Dependencies**: Existing Lusush continuation system

#### Structure Analyzer
**Status**: NEW - To be implemented  
**Purpose**: Analyze command structure for intelligent reconstruction  
**Estimated LOC**: 600-800 lines  
**Dependencies**: Shell tokenizer, keyword detection

#### Reconstruction Engine
**Status**: NEW - To be implemented  
**Purpose**: Reconstruct multiline commands with proper formatting  
**Estimated LOC**: 400-500 lines  
**Dependencies**: Structure analyzer, indentation calculator

#### Formatting Engine
**Status**: NEW - To be implemented  
**Purpose**: Apply intelligent indentation and formatting rules  
**Estimated LOC**: 300-400 lines  
**Dependencies**: Command structure, shell construct types

---

## 4. Phase 1: Core Integration Infrastructure

**Duration**: 1 week  
**Priority**: Critical  
**Estimated LOC**: 800-1000 lines

### 4.1 Deliverables

#### 4.1.1 Integration System Framework
- `lle_history_buffer_integration_t` structure definition
- System creation and destruction functions
- Configuration management
- Thread safety (rwlock implementation)

**Files**:
- `include/lle/history_buffer_integration.h` (200 lines)
- `src/lle/history_buffer_integration.c` (300 lines)

#### 4.1.2 Edit Session Management
- `lle_edit_session_t` structure definition
- Session creation and destruction
- Session lifecycle tracking
- Memory pool integration for sessions

**Files**:
- `include/lle/edit_session.h` (150 lines)
- `src/lle/edit_session.c` (250 lines)

#### 4.1.3 Callback Framework
- `lle_history_edit_callbacks_t` definition
- Callback invocation wrapper functions
- Error handling for callback failures
- User data context management

**Files**:
- `include/lle/history_callbacks.h` (100 lines)
- `src/lle/callback_framework.c` (200 lines)

### 4.2 Dependencies

**Required Before Starting**:
- ✓ Spec 09 History System (already implemented)
- ✓ Spec 03 Buffer Management Phase 1 (already implemented)
- ✓ Spec 02 Memory Management (already implemented)
- ✓ Spec 04 Event System (already implemented)

**Blocking Dependencies**: None

### 4.3 Test Strategy

#### Unit Tests
- Integration system creation/destruction
- Session lifecycle management
- Callback registration and invocation
- Thread safety (concurrent access)
- Memory leak detection

**Test File**: `tests/lle/unit/test_history_buffer_integration_phase1.c` (400 lines)

#### Integration Tests
- Integration with existing history system
- Integration with existing buffer system
- Event system integration
- Memory pool usage

**Test File**: `tests/lle/integration/test_integration_phase1.c` (300 lines)

### 4.4 Success Criteria

- [ ] Integration system can be created and destroyed without leaks
- [ ] Edit sessions can be created, tracked, and destroyed
- [ ] Callbacks can be registered and invoked correctly
- [ ] Thread-safe access verified with concurrent tests
- [ ] All Phase 1 unit tests pass (100% coverage)
- [ ] Integration tests demonstrate basic connectivity

### 4.5 Estimated Timeline

- Day 1-2: Integration system framework
- Day 3-4: Edit session management
- Day 5: Callback framework
- Day 6-7: Testing and documentation

---

## 5. Phase 2: Multiline Reconstruction Engine

**Duration**: 2 weeks  
**Priority**: Critical  
**Estimated LOC**: 1800-2200 lines

### 5.1 Deliverables

#### 5.1.1 Command Structure Analysis
- Shell construct detection (loops, conditionals, functions)
- Keyword position tracking
- Nested construct identification
- Indentation pattern extraction

**Files**:
- `include/lle/command_structure.h` (250 lines)
- `src/lle/structure_analyzer.c` (600-800 lines)

**Key Functions**:
```c
lle_result_t lle_structure_analyzer_analyze_command(
    lle_structure_analyzer_t *analyzer,
    const char *command_text,
    size_t text_length,
    lle_command_structure_t **structure);

lle_result_t lle_detect_primary_construct_type(
    lle_token_list_t *tokens,
    lle_command_construct_type_t *type);

lle_result_t lle_identify_nested_constructs(
    lle_token_list_t *tokens,
    lle_command_structure_t *structure);
```

#### 5.1.2 Reconstruction Engine
- Multiline content reconstruction
- Intelligent indentation application
- Line-by-line processing
- Structure preservation

**Files**:
- `include/lle/reconstruction_engine.h` (150 lines)
- `src/lle/reconstruction_engine.c` (400-500 lines)

**Key Functions**:
```c
lle_result_t lle_reconstruction_engine_reconstruct_multiline(
    lle_reconstruction_engine_t *engine,
    const char *source_content,
    size_t source_length,
    lle_command_structure_t *structure,
    char **reconstructed_content,
    size_t *reconstructed_length);

size_t lle_calculate_target_indentation(
    lle_command_structure_t *structure,
    size_t line_number,
    size_t current_level);

size_t lle_calculate_next_indent_level(
    lle_command_structure_t *structure,
    size_t next_line_number,
    size_t current_indent,
    const char *current_line_content,
    size_t content_length);
```

#### 5.1.3 Multiline Parser
- Shell construct parsing
- Quote and bracket tracking
- Line boundary detection
- Continuation state management

**Files**:
- `include/lle/multiline_parser.h` (200 lines)
- `src/lle/multiline_parser.c` (400-500 lines)

**Key Functions**:
```c
lle_result_t lle_multiline_parser_create(
    lle_multiline_parser_t **parser,
    lle_memory_pool_t *memory_pool);

lle_result_t lle_multiline_parser_preserve_structure(
    lle_multiline_parser_t *parser,
    lle_buffer_t *buffer,
    char **preserved_multiline);

lle_result_t lle_analyze_line_structure(
    const char *command_text,
    size_t text_length,
    lle_command_structure_t *structure);
```

### 5.2 Dependencies

**Required Before Starting**:
- ✓ Phase 1 completion
- ✓ Lusush continuation system (existing)
- ✓ Shell tokenizer (existing or new)

**Blocking Dependencies**:
- Phase 1 must be complete

### 5.3 Test Strategy

#### Unit Tests
- Structure analysis for each construct type
  - For loops
  - While loops
  - If/then/else statements
  - Case statements
  - Function definitions
  - Nested constructs
- Indentation calculation
- Line parsing
- Keyword extraction

**Test File**: `tests/lle/unit/test_multiline_reconstruction.c` (800 lines)

**Test Data**: `tests/lle/data/multiline_test_cases.txt` (1000+ lines of test commands)

#### Integration Tests
- Complex multiline command reconstruction
- Nested construct handling
- Edge cases (unclosed quotes, etc.)
- Performance benchmarks (<2ms for complex constructs)

**Test File**: `tests/lle/integration/test_reconstruction_integration.c` (500 lines)

### 5.4 Critical Algorithms

#### Indentation Calculator
```c
// Calculate indentation for shell constructs
size_t lle_calculate_case_indentation(
    lle_command_structure_t *structure, 
    size_t line_number);
```

**Complexity**: O(keywords) per line  
**Performance Target**: <10μs per line

#### Structure Parser
```c
// Parse shell construct structure
lle_result_t lle_parse_shell_construct(
    const char *text,
    size_t length,
    lle_command_structure_t *structure);
```

**Complexity**: O(n) where n = text length  
**Performance Target**: <500μs for 1KB command

### 5.5 Success Criteria

- [ ] All shell construct types correctly identified
- [ ] Proper indentation applied for all construct types
- [ ] Nested constructs handled correctly (3+ levels deep)
- [ ] Original multiline formatting preserved
- [ ] Performance targets met (<2ms for complex multiline)
- [ ] All Phase 2 unit tests pass (100% coverage)
- [ ] Integration tests demonstrate correct reconstruction

### 5.6 Estimated Timeline

- Days 1-3: Structure analyzer implementation
- Days 4-6: Reconstruction engine implementation
- Days 7-9: Multiline parser implementation
- Days 10-12: Testing and optimization
- Days 13-14: Documentation and edge case handling

---

## 6. Phase 3: Interactive Editing System

**Duration**: 1.5 weeks  
**Priority**: Critical  
**Estimated LOC**: 1200-1500 lines

### 6.1 Deliverables

#### 6.1.1 Primary Edit Function
- `lle_history_edit_entry()` implementation
- Complete callback lifecycle
- Session state management
- Error handling and recovery

**Files**:
- `src/lle/history_editing.c` (400-500 lines)

**Key Function**:
```c
lle_result_t lle_history_edit_entry(
    lle_history_buffer_integration_t *integration,
    size_t entry_index,
    lle_history_edit_callbacks_t *callbacks,
    void *user_data);
```

**Implementation Complexity**: Very High  
**Critical Path**: This is THE primary function from LLE_DESIGN_DOCUMENT.md

#### 6.1.2 Buffer Loading with Reconstruction
- Integration of reconstruction engine
- Buffer content loading
- Cursor positioning
- Multiline structure application

**Files**:
- `src/lle/buffer_history_bridge.c` (300-400 lines)

**Key Functions**:
```c
lle_result_t lle_history_buffer_load_entry_with_reconstruction(
    lle_history_buffer_integration_t *integration,
    lle_history_entry_t *entry,
    lle_buffer_t *target_buffer,
    lle_history_edit_callbacks_t *callbacks);

lle_result_t lle_buffer_load_from_history_entry(
    lle_buffer_t *buffer,
    lle_history_entry_t *entry,
    lle_history_buffer_integration_t *integration,
    bool preserve_multiline_structure);
```

#### 6.1.3 Session Completion
- Save/discard logic
- Content extraction from buffer
- Multiline structure preservation
- History entry replacement

**Files**:
- `src/lle/edit_session.c` (extension, +200 lines)

**Key Functions**:
```c
lle_result_t lle_edit_session_complete(
    lle_history_buffer_integration_t *integration,
    lle_edit_session_t *session,
    bool save_changes);

lle_result_t lle_buffer_save_to_history(
    lle_buffer_t *buffer,
    lle_history_buffer_integration_t *integration,
    lle_history_entry_t *original_entry);
```

#### 6.1.4 Event Integration
- Edit lifecycle events
- Event publishing
- Event handler registration
- Event data structures

**Files**:
- `include/lle/history_buffer_events.h` (150 lines)
- `src/lle/history_buffer_events.c` (250-300 lines)

### 6.2 Dependencies

**Required Before Starting**:
- ✓ Phase 1 completion
- ✓ Phase 2 completion
- ✓ Buffer management content APIs
- ✓ Event system

**Blocking Dependencies**:
- Phase 1 and Phase 2 must be complete

### 6.3 Test Strategy

#### Functional Tests
- Simple command editing
- Multiline command editing
- For loop editing
- If statement editing
- Function definition editing
- Nested construct editing
- Edit cancellation
- Save changes
- Discard changes

**Test File**: `tests/lle/functional/test_interactive_editing.c` (600 lines)

#### Callback Tests
- on_edit_start invocation
- on_edit_complete invocation
- on_edit_cancel invocation
- on_buffer_loaded invocation
- on_structure_reconstructed invocation
- Callback error handling
- User data passing

**Test File**: `tests/lle/functional/test_edit_callbacks.c` (400 lines)

#### Integration Tests
- Full edit workflow (start -> modify -> save)
- Full edit workflow (start -> modify -> cancel)
- Concurrent edit sessions (error case)
- Integration with Lusush shell
- Event system integration
- Memory leak testing

**Test File**: `tests/lle/integration/test_editing_integration.c` (500 lines)

### 6.4 Success Criteria

- [ ] `lle_history_edit_entry()` successfully loads entries into buffer
- [ ] All callbacks invoked at correct lifecycle points
- [ ] Multiline commands loaded with proper formatting
- [ ] Modified content correctly saved to history
- [ ] Original_multiline field preserved
- [ ] Edit cancellation restores original state
- [ ] All Phase 3 functional tests pass
- [ ] All callback tests pass
- [ ] Integration tests demonstrate complete workflow

### 6.5 Estimated Timeline

- Days 1-2: Primary edit function implementation
- Days 3-4: Buffer loading with reconstruction
- Days 5-6: Session completion and save/discard
- Days 7-8: Event integration
- Days 9-10: Testing and refinement
- Day 11: Documentation

---

## 7. Phase 4: Performance Optimization

**Duration**: 1 week  
**Priority**: High  
**Estimated LOC**: 600-800 lines

### 7.1 Deliverables

#### 7.1.1 Edit Cache System
- Reconstruction result caching
- Structure analysis caching
- LRU eviction policy
- Cache hit/miss statistics

**Files**:
- `include/lle/edit_cache.h` (150 lines)
- `src/lle/edit_cache.c` (400-500 lines)

**Key Functions**:
```c
lle_result_t lle_edit_cache_create(
    lle_edit_cache_t **cache,
    size_t max_cache_size,
    lle_memory_pool_t *memory_pool);

lle_result_t lle_cached_multiline_reconstruction(
    lle_history_buffer_integration_t *integration,
    lle_history_entry_t *entry,
    char **reconstructed_content,
    size_t *content_length);

uint64_t lle_generate_reconstruction_cache_key(
    lle_history_entry_t *entry);
```

#### 7.1.2 Performance Metrics
- Operation timing
- Cache efficiency tracking
- Memory usage monitoring
- Performance reporting

**Files**:
- `src/lle/performance_tracking.c` (200-250 lines)

#### 7.1.3 Memory Optimization
- Session memory pools
- String deduplication
- Structure reuse
- Memory pressure handling

**Files**:
- `src/lle/memory_optimization.c` (100-150 lines)

### 7.2 Performance Targets

| Operation | Target | Current (estimated) | Optimization Required |
|-----------|--------|---------------------|----------------------|
| Simple command load | <0.5ms | ~1ms | Cache + optimize |
| Complex multiline reconstruction | <2ms | ~5ms | Cache + algorithm |
| Structure analysis | <1ms | ~2ms | Cache |
| Session creation | <1ms | ~1.5ms | Pool optimization |
| Cache hit reconstruction | <0.1ms | N/A | New feature |
| Memory per session | <10KB | ~20KB | Pool reuse |

### 7.3 Test Strategy

#### Performance Tests
- Reconstruction speed benchmarks
- Cache hit rate measurement
- Memory usage profiling
- Concurrent access performance
- Large command handling (32KB limit)

**Test File**: `tests/lle/performance/test_edit_performance.c` (400 lines)

#### Stress Tests
- 1000 sequential edits
- 100 concurrent sessions (error handling)
- Cache thrashing scenarios
- Memory exhaustion recovery

**Test File**: `tests/lle/stress/test_edit_stress.c` (300 lines)

### 7.4 Success Criteria

- [ ] Simple command load <0.5ms (95th percentile)
- [ ] Complex multiline reconstruction <2ms (95th percentile)
- [ ] Cache hit rate >75% for repeated edits
- [ ] Cache hit reconstruction <0.1ms
- [ ] Memory per session <10KB average
- [ ] No memory leaks under continuous operation
- [ ] Performance tests pass all targets
- [ ] Stress tests demonstrate stability

### 7.5 Estimated Timeline

- Days 1-2: Cache system implementation
- Days 3-4: Performance metrics and tracking
- Days 5: Memory optimization
- Days 6-7: Testing and tuning

---

## 8. Phase 5: Testing and Validation

**Duration**: 1 week  
**Priority**: Critical  
**Estimated LOC**: 2000-2500 lines (test code)

### 8.1 Deliverables

#### 8.1.1 Comprehensive Test Suite
- Unit tests for all components
- Integration tests for workflows
- Functional tests for features
- Performance benchmarks
- Stress tests

**Test Files** (estimated):
- `tests/lle/unit/test_integration_core.c` (400 lines)
- `tests/lle/unit/test_structure_analysis.c` (500 lines)
- `tests/lle/unit/test_reconstruction.c` (400 lines)
- `tests/lle/functional/test_editing_workflows.c` (600 lines)
- `tests/lle/integration/test_full_integration.c` (400 lines)
- `tests/lle/performance/test_benchmarks.c` (300 lines)

#### 8.1.2 Compliance Testing
- Spec 22 requirement verification
- LLE_DESIGN_DOCUMENT.md compliance
- API contract validation
- Thread safety verification

**Test File**: `tests/lle/compliance/test_spec22_compliance.c` (400 lines)

#### 8.1.3 Error Handling Validation
- All error paths tested
- Recovery mechanisms verified
- Memory leak detection
- Corruption handling

**Test File**: `tests/lle/validation/test_error_handling.c` (300 lines)

### 8.2 Test Coverage Goals

| Component | Target Coverage | Minimum Coverage |
|-----------|----------------|------------------|
| Integration core | 95% | 90% |
| Structure analyzer | 90% | 85% |
| Reconstruction engine | 95% | 90% |
| Edit session management | 100% | 95% |
| Callback framework | 100% | 95% |
| Cache system | 90% | 85% |
| Overall | 95% | 90% |

### 8.3 Test Execution Strategy

#### Continuous Testing
- Run unit tests on every commit
- Run integration tests nightly
- Run performance tests weekly
- Run stress tests before release

#### Test Automation
```bash
# Run all Phase 5 tests
./tests/lle/run_spec22_tests.sh

# Expected output:
# Unit Tests: 150/150 passed
# Integration Tests: 45/45 passed
# Functional Tests: 60/60 passed
# Performance Tests: 20/20 passed
# Compliance Tests: 30/30 passed
# Total: 305/305 passed (100%)
```

### 8.4 Success Criteria

- [ ] All unit tests pass (100%)
- [ ] All integration tests pass (100%)
- [ ] All functional tests pass (100%)
- [ ] All performance tests meet targets
- [ ] Code coverage >90% overall, >95% for critical paths
- [ ] No memory leaks detected by Valgrind
- [ ] Thread safety verified with ThreadSanitizer
- [ ] Compliance tests verify all Spec 22 requirements
- [ ] Documentation complete and accurate

### 8.5 Estimated Timeline

- Days 1-2: Complete unit test suite
- Days 3-4: Integration and functional tests
- Day 5: Performance and stress tests
- Day 6: Compliance and error handling tests
- Day 7: Coverage analysis and refinement

---

## 9. Success Criteria

### 9.1 Functional Requirements

#### Core Functionality
- ✅ Interactive History Editing
  - [ ] `lle_history_edit_entry()` fully implemented
  - [ ] Callback framework operational
  - [ ] Session lifecycle management complete
  - [ ] Save/discard functionality working

- ✅ Multiline Preservation
  - [ ] `original_multiline` field preserved
  - [ ] Multiline structure restored correctly
  - [ ] All shell constructs supported (loops, conditionals, functions)
  - [ ] Indentation intelligently applied

- ✅ Structure Reconstruction
  - [ ] Complex shell constructs properly analyzed
  - [ ] Nested constructs handled (3+ levels)
  - [ ] Intelligent formatting applied
  - [ ] Keyword positions tracked

- ✅ Buffer Integration
  - [ ] Seamless loading into buffer
  - [ ] Content extraction working
  - [ ] Cursor positioning correct
  - [ ] Multiline state preserved

### 9.2 Performance Requirements

| Metric | Target | Must Have | Nice to Have |
|--------|--------|-----------|--------------|
| Simple command load | <0.5ms | <1ms | <0.3ms |
| Complex multiline reconstruction | <2ms | <5ms | <1ms |
| Structure analysis | <1ms | <2ms | <0.5ms |
| Cache hit reconstruction | <0.1ms | <0.2ms | <0.05ms |
| Cache hit rate | >75% | >60% | >85% |
| Memory per session | <10KB | <20KB | <5KB |
| Session creation | <1ms | <2ms | <0.5ms |

### 9.3 Quality Requirements

- **Memory Safety**
  - [ ] Zero memory leaks (Valgrind clean)
  - [ ] Proper cleanup of all resources
  - [ ] Memory pool integration correct
  - [ ] No use-after-free issues

- **Thread Safety**
  - [ ] Concurrent access supported
  - [ ] rwlock correctly implemented
  - [ ] No race conditions (ThreadSanitizer clean)
  - [ ] Deadlock prevention verified

- **Error Handling**
  - [ ] All error paths tested
  - [ ] Graceful degradation
  - [ ] Recovery mechanisms working
  - [ ] Error messages meaningful

- **Code Quality**
  - [ ] Code coverage >90%
  - [ ] Static analysis clean (clang-tidy, cppcheck)
  - [ ] Documentation complete
  - [ ] API contracts clear

### 9.4 Integration Requirements

- **System Integration**
  - [ ] History system integration verified
  - [ ] Buffer system integration verified
  - [ ] Event system integration verified
  - [ ] Memory pool integration verified

- **Backward Compatibility**
  - [ ] Existing history entries supported
  - [ ] No breaking changes to Spec 09
  - [ ] No breaking changes to Spec 03
  - [ ] Lusush compatibility maintained

### 9.5 Specification Compliance

- **LLE_DESIGN_DOCUMENT.md Requirements**
  - [ ] `lle_history_edit_entry()` matches design
  - [ ] `original_multiline` preservation implemented
  - [ ] Buffer-oriented multiline restoration working
  - [ ] Logical unit editing functional
  - [ ] Callback-based editing framework complete

- **Spec 22 Requirements**
  - [ ] All core structures implemented
  - [ ] All core functions implemented
  - [ ] All test scenarios passing
  - [ ] Performance targets met
  - [ ] Documentation complete

---

## 10. Risk Analysis

### 10.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Shell construct parsing complexity | High | High | Start with simple constructs, add incrementally |
| Performance targets not met | Medium | Medium | Implement caching early, profile continuously |
| Buffer API limitations | Low | High | Verify buffer APIs early, request extensions if needed |
| Thread safety issues | Medium | High | Design with locks from start, use sanitizers |
| Memory leaks | Medium | Medium | Use Valgrind from Phase 1, memory pool carefully |
| Callback complexity | Medium | Medium | Simple callback design, thorough testing |

### 10.2 Schedule Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Phase 2 overrun (complexity) | High | High | Allocate buffer time, prioritize core features |
| Testing phase inadequate | Medium | High | Start testing early, continuous integration |
| Integration issues | Medium | Medium | Early integration testing, incremental approach |
| Documentation delays | Low | Low | Document as you code, use templates |

### 10.3 Dependency Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Buffer APIs incomplete | Low | High | Verify APIs exist before Phase 3 |
| History system changes | Low | Medium | Lock Spec 09 API, use versioning |
| Event system changes | Low | Low | Event system stable, low risk |
| Lusush continuation system changes | Low | Medium | Understand current implementation early |

### 10.4 Mitigation Strategies

#### High Priority Mitigations
1. **Start with simple shell constructs** - Implement simple commands first, add complexity incrementally
2. **Early performance profiling** - Profile from Phase 1, don't wait until Phase 4
3. **Continuous testing** - Test each component as implemented, don't batch testing
4. **API verification** - Verify all required APIs exist before Phase 3
5. **Memory discipline** - Use Valgrind from day 1, fix leaks immediately

#### Medium Priority Mitigations
1. **Incremental integration** - Integrate with existing systems early and often
2. **Code reviews** - Review complex algorithms (parsing, indentation)
3. **Documentation as code** - Document each function as implemented
4. **Static analysis** - Run clang-tidy and cppcheck regularly
5. **Performance budgets** - Set performance budgets for each operation

---

## 11. Implementation Timeline

### 11.1 Overall Schedule

**Total Duration**: 6 weeks (30 working days)  
**Buffer**: 1 week for unforeseen issues

| Phase | Duration | Start | End | Dependencies |
|-------|----------|-------|-----|--------------|
| Phase 1: Core Integration | 1 week | Week 1 | Week 1 | None |
| Phase 2: Multiline Reconstruction | 2 weeks | Week 2 | Week 3 | Phase 1 |
| Phase 3: Interactive Editing | 1.5 weeks | Week 4 | Mid Week 5 | Phase 1, 2 |
| Phase 4: Performance Optimization | 1 week | Mid Week 5 | Week 6 | Phase 3 |
| Phase 5: Testing & Validation | 1 week | Week 6 | Week 6 | All phases |
| Buffer/Polish | 0.5 weeks | Week 6 | Week 6 | All phases |

### 11.2 Detailed Schedule

#### Week 1: Phase 1 - Core Integration Infrastructure
- **Day 1-2**: Integration system framework
  - Define structures
  - Implement lifecycle functions
  - Thread safety
- **Day 3-4**: Edit session management
  - Session creation/destruction
  - Session tracking
  - Memory pools
- **Day 5**: Callback framework
  - Callback structures
  - Invocation wrappers
  - Error handling
- **Day 6-7**: Testing
  - Unit tests
  - Integration tests
  - Memory leak testing

#### Week 2-3: Phase 2 - Multiline Reconstruction Engine
- **Days 1-3**: Structure analyzer (Week 2)
  - Construct detection
  - Keyword tracking
  - Nested constructs
- **Days 4-6**: Reconstruction engine (Week 2)
  - Line processing
  - Indentation calculation
  - Structure preservation
- **Days 7-9**: Multiline parser (Week 3)
  - Shell construct parsing
  - Line boundary detection
  - Quote/bracket tracking
- **Days 10-12**: Testing and optimization (Week 3)
  - Unit tests for all constructs
  - Performance benchmarking
  - Edge case handling
- **Days 13-14**: Documentation (Week 3)
  - API documentation
  - Algorithm documentation
  - Usage examples

#### Week 4 - Mid Week 5: Phase 3 - Interactive Editing System
- **Days 1-2**: Primary edit function
  - `lle_history_edit_entry()` implementation
  - Callback lifecycle
  - Session management
- **Days 3-4**: Buffer loading
  - Reconstruction integration
  - Content loading
  - Cursor positioning
- **Days 5-6**: Session completion
  - Save/discard logic
  - Content extraction
  - History replacement
- **Days 7-8**: Event integration
  - Event structures
  - Event publishing
  - Handler registration
- **Days 9-10**: Testing
  - Functional tests
  - Callback tests
  - Integration tests
- **Day 11**: Documentation

#### Mid Week 5 - Week 6: Phase 4 - Performance Optimization
- **Days 1-2**: Cache system
  - Cache structures
  - LRU implementation
  - Hit/miss tracking
- **Days 3-4**: Performance metrics
  - Operation timing
  - Cache efficiency
  - Memory monitoring
- **Day 5**: Memory optimization
  - Session pools
  - String deduplication
  - Pressure handling
- **Days 6-7**: Testing and tuning
  - Performance tests
  - Stress tests
  - Optimization tuning

#### Week 6: Phase 5 - Testing & Validation + Polish
- **Days 1-2**: Complete test suite
  - Unit tests
  - Integration tests
- **Days 3-4**: Advanced testing
  - Functional tests
  - Performance tests
  - Stress tests
- **Day 5**: Compliance testing
  - Spec 22 compliance
  - Design document compliance
  - API validation
- **Day 6**: Coverage and refinement
  - Coverage analysis
  - Gap filling
  - Bug fixes
- **Day 7**: Final polish
  - Documentation review
  - Code cleanup
  - Release preparation

### 11.3 Milestones

| Milestone | Date | Deliverable | Success Metric |
|-----------|------|-------------|----------------|
| M1: Core Integration Complete | End Week 1 | Phase 1 done | All Phase 1 tests pass |
| M2: Structure Analysis Working | End Week 2 | Structure analyzer | Can identify all constructs |
| M3: Reconstruction Working | End Week 3 | Reconstruction engine | Can reconstruct multiline |
| M4: Interactive Editing Working | Mid Week 5 | Edit function | Can edit and save entries |
| M5: Performance Optimized | End Week 5 | Cache system | Performance targets met |
| M6: Testing Complete | End Week 6 | Full test suite | 100% tests pass, >90% coverage |
| M7: Release Ready | End Week 6 | Complete system | All success criteria met |

### 11.4 Resource Requirements

**Developer**: 1 senior C developer with shell scripting knowledge  
**Review**: Code reviews for complex algorithms (parsing, indentation)  
**Testing**: Continuous integration environment  
**Documentation**: Technical writer (optional, for final polish)

**Tools Required**:
- GCC/Clang compiler
- Valgrind (memory leak detection)
- ThreadSanitizer (thread safety)
- clang-tidy (static analysis)
- cppcheck (static analysis)
- gcov/lcov (code coverage)
- git (version control)

---

## 12. Appendix

### 12.1 File Structure

```
lusush/
├── include/lle/
│   ├── history_buffer_integration.h    (NEW - 200 lines)
│   ├── edit_session.h                  (NEW - 150 lines)
│   ├── history_callbacks.h             (NEW - 100 lines)
│   ├── command_structure.h             (NEW - 250 lines)
│   ├── reconstruction_engine.h         (NEW - 150 lines)
│   ├── multiline_parser.h              (NEW - 200 lines)
│   ├── edit_cache.h                    (NEW - 150 lines)
│   └── history_buffer_events.h         (NEW - 150 lines)
│
├── src/lle/
│   ├── history_buffer_integration.c    (NEW - 300 lines)
│   ├── edit_session.c                  (NEW - 450 lines)
│   ├── callback_framework.c            (NEW - 200 lines)
│   ├── structure_analyzer.c            (NEW - 600-800 lines)
│   ├── reconstruction_engine.c         (NEW - 400-500 lines)
│   ├── multiline_parser.c              (NEW - 400-500 lines)
│   ├── history_editing.c               (NEW - 400-500 lines)
│   ├── buffer_history_bridge.c         (NEW - 300-400 lines)
│   ├── edit_cache.c                    (NEW - 400-500 lines)
│   ├── history_buffer_events.c         (NEW - 250-300 lines)
│   ├── performance_tracking.c          (NEW - 200-250 lines)
│   └── memory_optimization.c           (NEW - 100-150 lines)
│
└── tests/lle/
    ├── unit/
    │   ├── test_integration_core.c              (NEW - 400 lines)
    │   ├── test_structure_analysis.c            (NEW - 500 lines)
    │   ├── test_reconstruction.c                (NEW - 400 lines)
    │   └── test_multiline_reconstruction.c      (NEW - 800 lines)
    ├── functional/
    │   ├── test_interactive_editing.c           (NEW - 600 lines)
    │   ├── test_edit_callbacks.c                (NEW - 400 lines)
    │   └── test_editing_workflows.c             (NEW - 600 lines)
    ├── integration/
    │   ├── test_integration_phase1.c            (NEW - 300 lines)
    │   ├── test_reconstruction_integration.c    (NEW - 500 lines)
    │   ├── test_editing_integration.c           (NEW - 500 lines)
    │   └── test_full_integration.c              (NEW - 400 lines)
    ├── performance/
    │   ├── test_edit_performance.c              (NEW - 400 lines)
    │   └── test_benchmarks.c                    (NEW - 300 lines)
    ├── stress/
    │   └── test_edit_stress.c                   (NEW - 300 lines)
    ├── compliance/
    │   └── test_spec22_compliance.c             (NEW - 400 lines)
    ├── validation/
    │   └── test_error_handling.c                (NEW - 300 lines)
    └── data/
        └── multiline_test_cases.txt             (NEW - 1000+ lines)
```

### 12.2 LOC Summary

| Category | Estimated LOC | Files |
|----------|--------------|-------|
| Headers | 1,350 | 8 |
| Source Code | 4,500-5,500 | 12 |
| Unit Tests | 2,900 | 4 |
| Functional Tests | 1,600 | 3 |
| Integration Tests | 1,700 | 4 |
| Performance/Stress Tests | 1,000 | 3 |
| Compliance/Validation Tests | 700 | 2 |
| Test Data | 1,000+ | 1 |
| **Total Production** | **5,850-6,850** | **20** |
| **Total Test** | **8,900+** | **17** |
| **Grand Total** | **14,750-15,750+** | **37** |

### 12.3 Key Algorithms Reference

#### Indentation Calculator (Phase 2)
- **Purpose**: Calculate proper indentation for shell constructs
- **Input**: Command structure, line number, current level
- **Output**: Target indentation level
- **Complexity**: O(keywords) per line
- **Performance**: <10μs per line

#### Structure Parser (Phase 2)
- **Purpose**: Parse and analyze shell construct structure
- **Input**: Command text, length
- **Output**: Command structure tree
- **Complexity**: O(n) where n = text length
- **Performance**: <500μs for 1KB command

#### LRU Cache Eviction (Phase 4)
- **Purpose**: Evict least recently used cache entries
- **Input**: Cache, new entry
- **Output**: Updated cache
- **Complexity**: O(1) with hash table + linked list
- **Performance**: <10μs per eviction

---

## Conclusion

This implementation plan provides a complete roadmap for implementing Spec 22: History-Buffer Integration. The plan is structured into 5 well-defined phases with clear deliverables, dependencies, test strategies, and success criteria.

**Key Success Factors**:
1. **Phased approach** - Each phase builds on previous work
2. **Continuous testing** - Testing integrated into each phase
3. **Performance focus** - Performance considered from Phase 1
4. **Clear dependencies** - Dependencies identified and managed
5. **Risk mitigation** - Risks identified with mitigation strategies

**Next Steps**:
1. Review and approve this implementation plan
2. Allocate resources (developer, review time, testing environment)
3. Set up development environment and tools
4. Begin Phase 1: Core Integration Infrastructure
5. Follow phased approach with continuous integration and testing

**Estimated Total Effort**: 6 weeks (30 working days) with 1 week buffer  
**Estimated Total Code**: 14,750-15,750+ lines (production + tests)

The implementation of Spec 22 will complete the critical missing functionality from LLE_DESIGN_DOCUMENT.md and enable full interactive history editing with multiline command support in Lusush.
