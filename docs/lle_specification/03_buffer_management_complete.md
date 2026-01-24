# Buffer Management Complete Specification

**Document**: 03_buffer_management_complete.md  
**Version**: 1.0.0  
**Date**: 2025-01-07  
**Status**: Implementation-Ready Specification  
**Classification**: Core Architecture Component  

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Buffer Structure and Management](#3-buffer-structure-and-management)
4. [UTF-8 Unicode Support](#4-utf-8-unicode-support)
5. [Multiline Buffer Operations](#5-multiline-buffer-operations)
6. [Cursor Position Management](#6-cursor-position-management)
7. [Change Tracking and Undo System](#7-change-tracking-and-undo-system)
8. [Buffer Validation and Integrity](#8-buffer-validation-and-integrity)
9. [Memory Management Integration](#9-memory-management-integration)
10. [Performance Optimization](#10-performance-optimization)
11. [Integration with Lush Systems](#11-integration-with-lush-systems)
12. [Testing and Validation](#12-testing-and-validation)

---

## 1. Executive Summary

### 1.1 Purpose

The Buffer Management System provides the core foundation for LLE's revolutionary buffer-oriented command editing approach, treating shell commands as logical units rather than character streams. This system enables sophisticated multiline editing, complete UTF-8 Unicode support, and intelligent change tracking with comprehensive undo/redo capabilities.

### 1.2 Key Features

- **Buffer-Oriented Design**: Commands treated as logical text buffers with semantic structure
- **Complete UTF-8 Support**: Full Unicode handling with grapheme cluster awareness
- **Multiline Buffer Management**: Sophisticated support for complex multiline shell constructs
- **Intelligent Cursor Management**: Logical and visual cursor position synchronization
- **Change Tracking System**: Complete undo/redo with atomic operation support
- **Memory Safety**: Bounds checking and overflow protection throughout
- **Performance Optimization**: Sub-millisecond response times with intelligent caching

### 1.3 Critical Design Principles

1. **Logical vs Visual Separation**: Clear distinction between logical buffer positions and visual display coordinates
2. **UTF-8 Native**: All operations UTF-8 aware from the ground up
3. **Atomic Operations**: All buffer modifications are atomic and undoable
4. **Memory Safety**: Complete bounds checking and overflow protection
5. **Performance Focus**: Sub-millisecond operation times with intelligent optimization

---

## 2. Architecture Overview

### 2.1 Core Buffer Components

```c
// Primary buffer management system
typedef struct lle_buffer_system {
    lle_buffer_t *current_buffer;           // Active command buffer
    lle_buffer_pool_t *buffer_pool;         // Buffer memory pool
    lle_cursor_manager_t *cursor_mgr;       // Cursor position management
    lle_change_tracker_t *change_tracker;   // Undo/redo system
    lle_buffer_validator_t *validator;      // Buffer validation system
    lle_utf8_processor_t *utf8_processor;   // Unicode processing
    lle_multiline_manager_t *multiline_mgr; // Multiline structure management
    lle_performance_monitor_t *perf_monitor; // Performance monitoring
    lush_memory_pool_t *memory_pool;      // Lush memory integration
} lle_buffer_system_t;
```

### 2.2 Buffer System Initialization

```c
lle_result_t lle_buffer_system_init(lle_buffer_system_t **system, 
                                    lush_memory_pool_t *memory_pool) {
    lle_result_t result = LLE_SUCCESS;
    lle_buffer_system_t *buf_sys = NULL;
    
    // Step 1: Allocate system structure from memory pool
    buf_sys = lush_memory_pool_alloc(memory_pool, sizeof(lle_buffer_system_t));
    if (!buf_sys) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    memset(buf_sys, 0, sizeof(lle_buffer_system_t));
    buf_sys->memory_pool = memory_pool;
    
    // Step 2: Initialize performance monitoring first
    result = lle_performance_monitor_init(&buf_sys->perf_monitor, 
                                          "buffer_management");
    if (result != LLE_SUCCESS) {
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 3: Initialize UTF-8 processor
    result = lle_utf8_processor_init(&buf_sys->utf8_processor);
    if (result != LLE_SUCCESS) {
        lle_performance_monitor_destroy(buf_sys->perf_monitor);
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 4: Initialize buffer pool
    result = lle_buffer_pool_init(&buf_sys->buffer_pool, memory_pool);
    if (result != LLE_SUCCESS) {
        lle_utf8_processor_destroy(buf_sys->utf8_processor);
        lle_performance_monitor_destroy(buf_sys->perf_monitor);
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 5: Initialize cursor manager
    result = lle_cursor_manager_init(&buf_sys->cursor_mgr, buf_sys->utf8_processor);
    if (result != LLE_SUCCESS) {
        lle_buffer_pool_destroy(buf_sys->buffer_pool);
        lle_utf8_processor_destroy(buf_sys->utf8_processor);
        lle_performance_monitor_destroy(buf_sys->perf_monitor);
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 6: Initialize change tracker (undo/redo system)
    result = lle_change_tracker_init(&buf_sys->change_tracker, memory_pool);
    if (result != LLE_SUCCESS) {
        lle_cursor_manager_destroy(buf_sys->cursor_mgr);
        lle_buffer_pool_destroy(buf_sys->buffer_pool);
        lle_utf8_processor_destroy(buf_sys->utf8_processor);
        lle_performance_monitor_destroy(buf_sys->perf_monitor);
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 7: Initialize buffer validator
    result = lle_buffer_validator_init(&buf_sys->validator, buf_sys->utf8_processor);
    if (result != LLE_SUCCESS) {
        lle_change_tracker_destroy(buf_sys->change_tracker);
        lle_cursor_manager_destroy(buf_sys->cursor_mgr);
        lle_buffer_pool_destroy(buf_sys->buffer_pool);
        lle_utf8_processor_destroy(buf_sys->utf8_processor);
        lle_performance_monitor_destroy(buf_sys->perf_monitor);
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 8: Initialize multiline manager
    result = lle_multiline_manager_init(&buf_sys->multiline_mgr, 
                                        buf_sys->utf8_processor);
    if (result != LLE_SUCCESS) {
        lle_buffer_validator_destroy(buf_sys->validator);
        lle_change_tracker_destroy(buf_sys->change_tracker);
        lle_cursor_manager_destroy(buf_sys->cursor_mgr);
        lle_buffer_pool_destroy(buf_sys->buffer_pool);
        lle_utf8_processor_destroy(buf_sys->utf8_processor);
        lle_performance_monitor_destroy(buf_sys->perf_monitor);
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 9: Create initial buffer
    result = lle_buffer_create(&buf_sys->current_buffer, buf_sys->buffer_pool, 
                               LLE_BUFFER_DEFAULT_CAPACITY);
    if (result != LLE_SUCCESS) {
        lle_multiline_manager_destroy(buf_sys->multiline_mgr);
        lle_buffer_validator_destroy(buf_sys->validator);
        lle_change_tracker_destroy(buf_sys->change_tracker);
        lle_cursor_manager_destroy(buf_sys->cursor_mgr);
        lle_buffer_pool_destroy(buf_sys->buffer_pool);
        lle_utf8_processor_destroy(buf_sys->utf8_processor);
        lle_performance_monitor_destroy(buf_sys->perf_monitor);
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 10: Initialize cursor to beginning of buffer
    result = lle_cursor_manager_set_buffer(buf_sys->cursor_mgr, 
                                           buf_sys->current_buffer);
    if (result != LLE_SUCCESS) {
        lle_buffer_destroy(buf_sys->current_buffer);
        lle_multiline_manager_destroy(buf_sys->multiline_mgr);
        lle_buffer_validator_destroy(buf_sys->validator);
        lle_change_tracker_destroy(buf_sys->change_tracker);
        lle_cursor_manager_destroy(buf_sys->cursor_mgr);
        lle_buffer_pool_destroy(buf_sys->buffer_pool);
        lle_utf8_processor_destroy(buf_sys->utf8_processor);
        lle_performance_monitor_destroy(buf_sys->perf_monitor);
        lush_memory_pool_free(memory_pool, buf_sys);
        return result;
    }
    
    // Step 11: Perform system validation
    result = lle_buffer_system_validate(buf_sys);
    if (result != LLE_SUCCESS) {
        lle_buffer_system_destroy(buf_sys);
        return result;
    }
    
    *system = buf_sys;
    return LLE_SUCCESS;
}
```

---

## 3. Buffer Structure and Management

### 3.1 Core Buffer Structure

```c
typedef struct lle_buffer {
    // Buffer metadata
    uint32_t buffer_id;               // Unique buffer identifier
    char name[LLE_BUFFER_NAME_MAX];   // Buffer name/description
    uint64_t creation_time;           // Buffer creation timestamp
    uint64_t last_modified_time;      // Last modification timestamp
    uint32_t modification_count;      // Total modifications counter
    
    // Buffer content storage
    char *data;                       // UTF-8 encoded buffer data
    size_t capacity;                  // Allocated buffer capacity
    size_t length;                    // Current buffer length (bytes)
    size_t used;                      // Actually used buffer space
    
    // UTF-8 and Unicode metadata
    size_t codepoint_count;           // Number of Unicode codepoints
    size_t grapheme_count;            // Number of grapheme clusters
    lle_utf8_index_t *utf8_index;     // Fast UTF-8 position index
    bool utf8_index_valid;            // UTF-8 index validity flag
    
    // Line structure information
    lle_line_info_t *lines;           // Line structure array
    size_t line_count;                // Number of logical lines
    size_t line_capacity;             // Allocated line array capacity
    bool multiline_active;            // Multiline mode status
    lle_multiline_context_t *multiline_ctx; // Multiline parsing context
    
    // Cursor and selection
    lle_cursor_position_t cursor;     // Current cursor position
    lle_selection_range_t selection;  // Current selection range
    bool selection_active;            // Selection status flag
    
    // Change tracking integration
    lle_change_sequence_t *current_sequence; // Active change sequence
    uint32_t sequence_number;         // Current sequence number
    bool change_tracking_enabled;     // Change tracking status
    
    // Performance optimization
    lle_buffer_cache_t *cache;        // Rendering and operation cache
    uint32_t cache_version;           // Cache version counter
    bool cache_dirty;                 // Cache dirty flag
    
    // Validation and integrity
    uint32_t checksum;                // Buffer content checksum
    bool integrity_valid;             // Integrity validation status
    lle_buffer_flags_t flags;         // Buffer status flags
    
    // Memory management
    lle_buffer_pool_t *pool;          // Associated buffer pool
    lush_memory_pool_t *memory_pool; // Lush memory pool reference
    
} lle_buffer_t;
```

### 3.2 Line Structure Management

```c
typedef struct lle_line_info {
    size_t start_offset;              // Line start byte offset
    size_t end_offset;                // Line end byte offset
    size_t length;                    // Line length in bytes
    size_t codepoint_count;           // Number of codepoints in line
    size_t grapheme_count;            // Number of grapheme clusters
    size_t visual_width;              // Visual display width
    
    // Line type and characteristics
    lle_line_type_t type;             // Line type (command, continuation, etc.)
    lle_line_flags_t flags;           // Line status flags
    uint8_t indent_level;             // Indentation level
    
    // Multiline context
    lle_multiline_state_t ml_state;   // Multiline parser state
    char *ml_context;                 // Multiline context string
    
    // Performance optimization
    uint32_t render_cache_key;        // Render cache key
    bool needs_revalidation;          // Revalidation required flag
    
} lle_line_info_t;

lle_result_t lle_buffer_rebuild_line_structure(lle_buffer_t *buffer) {
    if (!buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    size_t line_count = 0;
    
    // Step 1: Count lines and estimate capacity
    const char *data = buffer->data;
    size_t data_len = buffer->length;
    
    for (size_t i = 0; i < data_len; i++) {
        if (data[i] == '\n') {
            line_count++;
        }
    }
    
    // Always have at least one line
    if (line_count == 0 || (data_len > 0 && data[data_len - 1] != '\n')) {
        line_count++;
    }
    
    // Step 2: Allocate or reallocate line array
    if (line_count > buffer->line_capacity) {
        size_t new_capacity = line_count * 2; // Growth strategy
        lle_line_info_t *new_lines = lush_memory_pool_realloc(
            buffer->memory_pool, 
            buffer->lines,
            new_capacity * sizeof(lle_line_info_t)
        );
        
        if (!new_lines) {
            return LLE_ERROR_MEMORY_ALLOCATION;
        }
        
        buffer->lines = new_lines;
        buffer->line_capacity = new_capacity;
    }
    
    // Step 3: Build line structure information
    size_t current_line = 0;
    size_t line_start = 0;
    
    for (size_t i = 0; i <= data_len; i++) {
        bool is_line_end = (i == data_len) || (data[i] == '\n');
        
        if (is_line_end) {
            lle_line_info_t *line = &buffer->lines[current_line];
            
            // Set basic line information
            line->start_offset = line_start;
            line->end_offset = i;
            line->length = i - line_start;
            
            // Analyze line content for UTF-8 and visual properties
            result = lle_analyze_line_content(buffer, line);
            if (result != LLE_SUCCESS) {
                return result;
            }
            
            // Determine line type and multiline context
            result = lle_determine_line_type(buffer, line);
            if (result != LLE_SUCCESS) {
                return result;
            }
            
            current_line++;
            line_start = i + 1;
        }
    }
    
    buffer->line_count = current_line;
    
    // Step 4: Update multiline parsing state
    if (buffer->multiline_active) {
        result = lle_update_multiline_context(buffer);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Step 5: Invalidate affected caches
    lle_buffer_invalidate_cache(buffer, LLE_CACHE_LINE_STRUCTURE);
    
    return LLE_SUCCESS;
}
```

### 3.3 Buffer Operations

```c
lle_result_t lle_buffer_insert_text(lle_buffer_t *buffer, 
                                    size_t position, 
                                    const char *text, 
                                    size_t text_length) {
    if (!buffer || !text || position > buffer->length) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    uint64_t operation_start_time = lle_get_current_time_us();
    
    // Step 1: Validate UTF-8 input
    if (!lle_utf8_is_valid(text, text_length)) {
        return LLE_ERROR_INVALID_UTF8;
    }
    
    // Step 2: Check if buffer needs expansion
    if (buffer->length + text_length > buffer->capacity) {
        result = lle_buffer_expand(buffer, buffer->length + text_length);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Step 3: Start change tracking sequence
    lle_change_operation_t *change_op = NULL;
    if (buffer->change_tracking_enabled) {
        result = lle_change_tracker_begin_operation(
            buffer->current_sequence,
            LLE_CHANGE_TYPE_INSERT,
            position,
            text_length,
            &change_op
        );
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Step 4: Make space for new text
    if (position < buffer->length) {
        memmove(buffer->data + position + text_length,
                buffer->data + position,
                buffer->length - position);
    }
    
    // Step 5: Insert new text
    memcpy(buffer->data + position, text, text_length);
    buffer->length += text_length;
    buffer->data[buffer->length] = '\0'; // Ensure null termination
    
    // Step 6: Update buffer metadata
    buffer->modification_count++;
    buffer->last_modified_time = lle_get_current_time_us();
    
    // Step 7: Update UTF-8 index
    result = lle_utf8_index_insert(buffer->utf8_index, position, text, text_length);
    if (result != LLE_SUCCESS) {
        // UTF-8 index update failed - mark for rebuild
        buffer->utf8_index_valid = false;
    } else {
        // Update codepoint and grapheme counts
        buffer->codepoint_count += lle_utf8_count_codepoints(text, text_length);
        buffer->grapheme_count += lle_utf8_count_graphemes(text, text_length);
    }
    
    // Step 8: Update line structure
    result = lle_buffer_update_line_structure_after_insert(buffer, position, 
                                                            text, text_length);
    if (result != LLE_SUCCESS) {
        // Line structure update failed - mark for rebuild
        lle_buffer_invalidate_cache(buffer, LLE_CACHE_LINE_STRUCTURE);
    }
    
    // Step 9: Update cursor position if necessary
    if (buffer->cursor.byte_offset >= position) {
        result = lle_cursor_manager_adjust_for_insert(buffer, position, text_length);
        if (result != LLE_SUCCESS) {
            // Cursor adjustment failed - reset cursor
            lle_cursor_manager_validate_and_correct(buffer);
        }
    }
    
    // Step 10: Complete change tracking
    if (buffer->change_tracking_enabled && change_op) {
        result = lle_change_tracker_complete_operation(change_op);
        if (result != LLE_SUCCESS) {
            // Change tracking failed - log warning but continue
            lle_log_warning("Change tracking completion failed for insert operation");
        }
    }
    
    // Step 11: Update buffer integrity
    result = lle_buffer_update_checksum(buffer);
    if (result != LLE_SUCCESS) {
        buffer->integrity_valid = false;
    }
    
    // Step 12: Update performance metrics
    uint64_t operation_time = lle_get_current_time_us() - operation_start_time;
    lle_performance_monitor_record_operation(buffer->pool->perf_monitor,
                                             "buffer_insert", operation_time);
    
    // Step 13: Invalidate relevant caches
    lle_buffer_invalidate_cache(buffer, LLE_CACHE_RENDER | LLE_CACHE_LAYOUT);
    
    return LLE_SUCCESS;
}

lle_result_t lle_buffer_delete_text(lle_buffer_t *buffer,
                                    size_t start_position,
                                    size_t delete_length) {
    if (!buffer || start_position > buffer->length || 
        start_position + delete_length > buffer->length) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (delete_length == 0) {
        return LLE_SUCCESS; // Nothing to delete
    }
    
    lle_result_t result = LLE_SUCCESS;
    uint64_t operation_start_time = lle_get_current_time_us();
    
    // Step 1: Start change tracking sequence
    lle_change_operation_t *change_op = NULL;
    if (buffer->change_tracking_enabled) {
        result = lle_change_tracker_begin_operation(
            buffer->current_sequence,
            LLE_CHANGE_TYPE_DELETE,
            start_position,
            delete_length,
            &change_op
        );
        if (result != LLE_SUCCESS) {
            return result;
        }
        
        // Save deleted text for undo
        result = lle_change_tracker_save_deleted_text(
            change_op, 
            buffer->data + start_position,
            delete_length
        );
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Step 2: Calculate UTF-8 statistics of deleted text
    size_t deleted_codepoints = 0;
    size_t deleted_graphemes = 0;
    
    if (buffer->utf8_index_valid) {
        deleted_codepoints = lle_utf8_count_codepoints(
            buffer->data + start_position, delete_length);
        deleted_graphemes = lle_utf8_count_graphemes(
            buffer->data + start_position, delete_length);
    }
    
    // Step 3: Remove text by shifting remaining data
    if (start_position + delete_length < buffer->length) {
        memmove(buffer->data + start_position,
                buffer->data + start_position + delete_length,
                buffer->length - start_position - delete_length);
    }
    
    // Step 4: Update buffer length and ensure null termination
    buffer->length -= delete_length;
    buffer->data[buffer->length] = '\0';
    
    // Step 5: Update buffer metadata
    buffer->modification_count++;
    buffer->last_modified_time = lle_get_current_time_us();
    
    // Step 6: Update UTF-8 index
    result = lle_utf8_index_delete(buffer->utf8_index, start_position, delete_length);
    if (result != LLE_SUCCESS) {
        buffer->utf8_index_valid = false;
    } else {
        buffer->codepoint_count -= deleted_codepoints;
        buffer->grapheme_count -= deleted_graphemes;
    }
    
    // Step 7: Update line structure
    result = lle_buffer_update_line_structure_after_delete(buffer, start_position, 
                                                           delete_length);
    if (result != LLE_SUCCESS) {
        lle_buffer_invalidate_cache(buffer, LLE_CACHE_LINE_STRUCTURE);
    }
    
    // Step 8: Update cursor position if necessary
    if (buffer->cursor.byte_offset >= start_position) {
        result = lle_cursor_manager_adjust_for_delete(buffer, start_position, 
                                                      delete_length);
        if (result != LLE_SUCCESS) {
            lle_cursor_manager_validate_and_correct(buffer);
        }
    }
    
    // Step 9: Complete change tracking
    if (buffer->change_tracking_enabled && change_op) {
        result = lle_change_tracker_complete_operation(change_op);
        if (result != LLE_SUCCESS) {
            lle_log_warning("Change tracking completion failed for delete operation");
        }
    }
    
    // Step 10: Update buffer integrity
    result = lle_buffer_update_checksum(buffer);
    if (result != LLE_SUCCESS) {
        buffer->integrity_valid = false;
    }
    
    // Step 11: Update performance metrics
    uint64_t operation_time = lle_get_current_time_us() - operation_start_time;
    lle_performance_monitor_record_operation(buffer->pool->perf_monitor,
                                             "buffer_delete", operation_time);
    
    // Step 12: Invalidate relevant caches
    lle_buffer_invalidate_cache(buffer, LLE_CACHE_RENDER | LLE_CACHE_LAYOUT);
    
    return LLE_SUCCESS;
}
```

---

## 4. UTF-8 Unicode Support

### 4.1 UTF-8 Processing System

```c
typedef struct lle_utf8_processor {
    // Unicode normalization context
    lle_unicode_normalizer_t *normalizer;
    
    // Grapheme cluster boundary detection
    lle_grapheme_detector_t *grapheme_detector;
    
    // Character width calculation (for display)
    lle_char_width_calculator_t *width_calculator;
    
    // UTF-8 validation and repair
    lle_utf8_validator_t *validator;
    
    // Performance optimization
    lle_utf8_cache_t *operation_cache;
    
} lle_utf8_processor_t;

typedef struct lle_utf8_index {
    // Fast position mapping
    size_t *byte_to_codepoint;     // Byte offset to codepoint index
    size_t *codepoint_to_byte;     // Codepoint index to byte offset
    size_t *grapheme_to_codepoint; // Grapheme cluster to codepoint index
    size_t *codepoint_to_grapheme; // Codepoint to grapheme cluster index
    
    // Index metadata
    size_t byte_count;             // Total bytes indexed
    size_t codepoint_count;        // Total codepoints indexed
    size_t grapheme_count;         // Total grapheme clusters indexed
    
    // Index validity and versioning
    bool index_valid;              // Index validity flag
    uint32_t buffer_version;       // Associated buffer version
    uint64_t last_update_time;     // Last index update time
    
    // Performance optimization
    size_t cache_hit_count;        // Cache hit statistics
    size_t cache_miss_count;       // Cache miss statistics
    
} lle_utf8_index_t;
```

### 4.2 UTF-8 Index Management

```c
lle_result_t lle_utf8_index_rebuild(lle_utf8_index_t *index, 
                                    const char *text, 
                                    size_t text_length) {
    if (!index || !text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    size_t codepoint_count = 0;
    size_t grapheme_count = 0;
    
    // Step 1: Count codepoints and grapheme clusters
    const char *ptr = text;
    const char *end = text + text_length;
    
    while (ptr < end) {
        // Validate UTF-8 sequence
        int sequence_length = lle_utf8_sequence_length(*ptr);
        if (sequence_length == 0 || ptr + sequence_length > end) {
            return LLE_ERROR_INVALID_UTF8;
        }
        
        // Validate complete UTF-8 sequence
        if (!lle_utf8_is_valid_sequence(ptr, sequence_length)) {
            return LLE_ERROR_INVALID_UTF8;
        }
        
        codepoint_count++;
        
        // Check if this starts a new grapheme cluster
        if (lle_is_grapheme_boundary(ptr, text, end)) {
            grapheme_count++;
        }
        
        ptr += sequence_length;
    }
    
    // Step 2: Allocate index arrays
    size_t *new_byte_to_codepoint = calloc(text_length + 1, sizeof(size_t));
    size_t *new_codepoint_to_byte = calloc(codepoint_count + 1, sizeof(size_t));
    size_t *new_grapheme_to_codepoint = calloc(grapheme_count + 1, sizeof(size_t));
    size_t *new_codepoint_to_grapheme = calloc(codepoint_count + 1, sizeof(size_t));
    
    if (!new_byte_to_codepoint || !new_codepoint_to_byte ||
        !new_grapheme_to_codepoint || !new_codepoint_to_grapheme) {
        free(new_byte_to_codepoint);
        free(new_codepoint_to_byte);
        free(new_grapheme_to_codepoint);
        free(new_codepoint_to_grapheme);
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Step 3: Build index mappings
    ptr = text;
    size_t byte_pos = 0;
    size_t codepoint_pos = 0;
    size_t grapheme_pos = 0;
    size_t current_grapheme_start = 0;
    
    while (ptr < end) {
        int sequence_length = lle_utf8_sequence_length(*ptr);
        
        // Update byte-to-codepoint mapping
        for (int i = 0; i < sequence_length; i++) {
            new_byte_to_codepoint[byte_pos + i] = codepoint_pos;
        }
        
        // Update codepoint-to-byte mapping
        new_codepoint_to_byte[codepoint_pos] = byte_pos;
        
        // Update grapheme cluster mappings
        if (lle_is_grapheme_boundary(ptr, text, end)) {
            if (grapheme_pos > 0) {
                // Complete previous grapheme cluster
                for (size_t i = current_grapheme_start; i < codepoint_pos; i++) {
                    new_codepoint_to_grapheme[i] = grapheme_pos - 1;
                }
            }
            new_grapheme_to_codepoint[grapheme_pos] = codepoint_pos;
            current_grapheme_start = codepoint_pos;
            grapheme_pos++;
        }
        
        codepoint_pos++;
        byte_pos += sequence_length;
        ptr += sequence_length;
    }
    
    // Step 4: Complete final grapheme cluster
    if (codepoint_count > 0 && current_grapheme_start < codepoint_count) {
        for (size_t i = current_grapheme_start; i < codepoint_count; i++) {
            new_codepoint_to_grapheme[i] = grapheme_pos - 1;
        }
    }
    
    // Step 5: Replace old index arrays with new ones
    free(index->byte_to_codepoint);
    free(index->codepoint_to_byte);
    free(index->grapheme_to_codepoint);
    free(index->codepoint_to_grapheme);
    
    index->byte_to_codepoint = new_byte_to_codepoint;
    index->codepoint_to_byte = new_codepoint_to_byte;
    index->grapheme_to_codepoint = new_grapheme_to_codepoint;
    index->codepoint_to_grapheme = new_codepoint_to_grapheme;
    
    // Step 6: Update index metadata
    index->byte_count = text_length;
    index->codepoint_count = codepoint_count;
    index->grapheme_count = grapheme_count;
    index->index_valid = true;
    index->last_update_time = lle_get_current_time_us();
    
    return LLE_SUCCESS;
}
```

---

## 5. Multiline Buffer Operations

### 5.1 Multiline Context Management

```c
typedef struct lle_multiline_context {
    // Multiline parsing state
    lle_shell_parser_state_t parser_state;    // Current shell parser state
    lle_bracket_stack_t *bracket_stack;       // Bracket matching stack
    lle_quote_state_t quote_state;            // Quote parsing state
    
    // Context tracking
    char *current_construct;                  // Current multiline construct
    size_t construct_start_line;              // Starting line of construct
    size_t construct_start_offset;            // Starting byte offset
    uint8_t nesting_level;                    // Current nesting level
    
    // Completion detection
    bool construct_complete;                  // Construction completion status
    bool needs_continuation;                  // Needs continuation line
    char *expected_terminator;                // Expected termination sequence
    
    // Performance optimization
    uint32_t cache_key;                       // Parsing cache key
    bool cache_valid;                         // Cache validity flag
    
} lle_multiline_context_t;

lle_result_t lle_multiline_manager_analyze_buffer(lle_multiline_manager_t *manager,
                                                  lle_buffer_t *buffer) {
    if (!manager || !buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    
    // Step 1: Initialize multiline context if needed
    if (!buffer->multiline_ctx) {
        result = lle_multiline_context_init(&buffer->multiline_ctx);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Step 2: Reset parsing state
    lle_multiline_context_reset(buffer->multiline_ctx);
    
    // Step 3: Parse buffer line by line
    for (size_t line_idx = 0; line_idx < buffer->line_count; line_idx++) {
        lle_line_info_t *line = &buffer->lines[line_idx];
        
        // Extract line content
        const char *line_content = buffer->data + line->start_offset;
        size_t line_length = line->length;
        
        // Parse line for shell constructs
        result = lle_parse_shell_line(buffer->multiline_ctx, 
                                      line_content, line_length);
        if (result != LLE_SUCCESS) {
            return result;
        }
        
        // Update line multiline state
        line->ml_state = buffer->multiline_ctx->parser_state;
        
        // Determine if this line needs continuation
        line->flags &= ~LLE_LINE_FLAG_NEEDS_CONTINUATION;
        if (buffer->multiline_ctx->needs_continuation) {
            line->flags |= LLE_LINE_FLAG_NEEDS_CONTINUATION;
        }
    }
    
    // Step 4: Determine overall buffer completion status
    buffer->multiline_active = !buffer->multiline_ctx->construct_complete;
    
    return LLE_SUCCESS;
}
```

---

## 6. Cursor Position Management

### 6.1 Cursor Position Structure

```c
typedef struct lle_cursor_position {
    // Byte-based position (primary)
    size_t byte_offset;                // Byte offset in buffer
    
    // UTF-8 based positions
    size_t codepoint_index;            // Unicode codepoint index
    size_t grapheme_index;             // Grapheme cluster index
    
    // Line-based position
    size_t line_number;                // Line number (0-based)
    size_t column_offset;              // Column offset in line (bytes)
    size_t column_codepoint;           // Column position (codepoints)
    size_t column_grapheme;            // Column position (graphemes)
    
    // Visual position
    size_t visual_line;                // Visual line (with wrapping)
    size_t visual_column;              // Visual column position
    
    // Position validity
    bool position_valid;               // Position validity flag
    uint32_t buffer_version;           // Associated buffer version
    
} lle_cursor_position_t;

typedef struct lle_cursor_manager {
    // Current cursor state
    lle_cursor_position_t position;    // Current cursor position
    lle_cursor_position_t target;      // Target cursor position
    
    // Movement preferences
    size_t preferred_visual_column;    // Preferred visual column
    bool sticky_column;                // Sticky column mode
    
    // UTF-8 processor reference
    lle_utf8_processor_t *utf8_processor;
    
    // Buffer reference
    lle_buffer_t *buffer;              // Associated buffer
    
    // Performance optimization
    lle_cursor_cache_t *position_cache; // Position calculation cache
    
} lle_cursor_manager_t;
```

### 6.2 Cursor Movement Operations

```c
lle_result_t lle_cursor_manager_move_to_byte_offset(lle_cursor_manager_t *manager,
                                                    size_t byte_offset) {
    if (!manager || !manager->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (byte_offset > manager->buffer->length) {
        return LLE_ERROR_INVALID_POSITION;
    }
    
    lle_result_t result = LLE_SUCCESS;
    
    // Step 1: Set byte offset
    manager->position.byte_offset = byte_offset;
    
    // Step 2: Calculate codepoint index
    if (manager->buffer->utf8_index_valid) {
        manager->position.codepoint_index = 
            manager->buffer->utf8_index->byte_to_codepoint[byte_offset];
    } else {
        result = lle_utf8_byte_to_codepoint_index(manager->buffer->data,
                                                  byte_offset,
                                                  &manager->position.codepoint_index);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Step 3: Calculate grapheme index
    if (manager->buffer->utf8_index_valid) {
        manager->position.grapheme_index = 
            manager->buffer->utf8_index->codepoint_to_grapheme[manager->position.codepoint_index];
    } else {
        result = lle_utf8_codepoint_to_grapheme_index(manager->buffer->data,
                                                      manager->position.codepoint_index,
                                                      &manager->position.grapheme_index);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Step 4: Calculate line and column positions
    result = lle_cursor_calculate_line_column(manager);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Step 5: Calculate visual position
    result = lle_cursor_calculate_visual_position(manager);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Step 6: Update position validity
    manager->position.position_valid = true;
    manager->position.buffer_version = manager->buffer->modification_count;
    
    return LLE_SUCCESS;
}

lle_result_t lle_cursor_manager_move_by_graphemes(lle_cursor_manager_t *manager,
                                                  int grapheme_delta) {
    if (!manager || !manager->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Calculate target grapheme index
    int target_grapheme = (int)manager->position.grapheme_index + grapheme_delta;
    
    // Bounds checking
    if (target_grapheme < 0) {
        target_grapheme = 0;
    } else if (target_grapheme > (int)manager->buffer->grapheme_count) {
        target_grapheme = (int)manager->buffer->grapheme_count;
    }
    
    // Convert back to byte offset
    size_t target_byte_offset;
    lle_result_t result = lle_grapheme_index_to_byte_offset(
        manager->buffer, 
        (size_t)target_grapheme,
        &target_byte_offset
    );
    
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    return lle_cursor_manager_move_to_byte_offset(manager, target_byte_offset);
}
```

---

## 7. Change Tracking and Undo System

### 7.1 Change Tracking Structure

```c
typedef enum {
    LLE_CHANGE_TYPE_INSERT = 1,        // Text insertion
    LLE_CHANGE_TYPE_DELETE,            // Text deletion
    LLE_CHANGE_TYPE_REPLACE,           // Text replacement
    LLE_CHANGE_TYPE_CURSOR_MOVE,       // Cursor movement
    LLE_CHANGE_TYPE_SELECTION,         // Selection change
    LLE_CHANGE_TYPE_COMPOSITE,         // Composite operation
} lle_change_type_t;

typedef struct lle_change_operation {
    // Operation metadata
    uint32_t operation_id;             // Unique operation identifier
    lle_change_type_t type;            // Change operation type
    uint64_t timestamp;                // Operation timestamp
    
    // Position information
    size_t start_position;             // Operation start position
    size_t end_position;               // Operation end position
    size_t affected_length;            // Length of affected text
    
    // Operation data
    char *inserted_text;               // Text that was inserted
    size_t inserted_length;            // Length of inserted text
    char *deleted_text;                // Text that was deleted
    size_t deleted_length;             // Length of deleted text
    
    // Cursor state
    lle_cursor_position_t cursor_before; // Cursor position before
    lle_cursor_position_t cursor_after;  // Cursor position after
    
    // Operation linking
    struct lle_change_operation *next; // Next operation in sequence
    struct lle_change_operation *prev; // Previous operation in sequence
    
} lle_change_operation_t;

typedef struct lle_change_sequence {
    // Sequence metadata
    uint32_t sequence_id;              // Unique sequence identifier
    char description[64];              // Human-readable description
    uint64_t start_time;               // Sequence start time
    uint64_t end_time;                 // Sequence end time
    
    // Operation chain
    lle_change_operation_t *first_op;  // First operation in sequence
    lle_change_operation_t *last_op;   // Last operation in sequence
    size_t operation_count;            // Number of operations
    
    // Sequence state
    bool sequence_complete;            // Sequence completion status
    bool can_undo;                     // Can be undone
    bool can_redo;                     // Can be redone
    
} lle_change_sequence_t;
```

### 7.2 Undo/Redo Implementation

```c
lle_result_t lle_change_tracker_undo(lle_change_tracker_t *tracker,
                                     lle_buffer_t *buffer) {
    if (!tracker || !buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Find the most recent sequence that can be undone
    lle_change_sequence_t *sequence = lle_find_last_undoable_sequence(tracker);
    if (!sequence) {
        return LLE_ERROR_NO_UNDO_AVAILABLE;
    }
    
    lle_result_t result = LLE_SUCCESS;
    
    // Step 1: Disable change tracking during undo
    bool tracking_was_enabled = buffer->change_tracking_enabled;
    buffer->change_tracking_enabled = false;
    
    // Step 2: Process operations in reverse order
    lle_change_operation_t *op = sequence->last_op;
    while (op) {
        switch (op->type) {
            case LLE_CHANGE_TYPE_INSERT:
                // Undo insert by deleting the inserted text
                result = lle_buffer_delete_text(buffer, op->start_position, 
                                                op->inserted_length);
                break;
                
            case LLE_CHANGE_TYPE_DELETE:
                // Undo delete by reinserting the deleted text
                result = lle_buffer_insert_text(buffer, op->start_position,
                                                op->deleted_text, op->deleted_length);
                break;
                
            case LLE_CHANGE_TYPE_REPLACE:
                // Undo replace by deleting new text and inserting old text
                result = lle_buffer_delete_text(buffer, op->start_position, 
                                                op->inserted_length);
                if (result == LLE_SUCCESS) {
                    result = lle_buffer_insert_text(buffer, op->start_position,
                                                    op->deleted_text, op->deleted_length);
                }
                break;
                
            default:
                // Other operation types don't modify buffer content
                break;
        }
        
        if (result != LLE_SUCCESS) {
            break;
        }
        
        op = op->prev;
    }
    
    // Step 3: Restore cursor position
    if (result == LLE_SUCCESS) {
        result = lle_cursor_manager_move_to_byte_offset(
            buffer->cursor_mgr, 
            sequence->first_op->cursor_before.byte_offset
        );
    }
    
    // Step 4: Mark sequence as undone
    if (result == LLE_SUCCESS) {
        sequence->can_undo = false;
        sequence->can_redo = true;
        tracker->undo_count++;
    }
    
    // Step 5: Re-enable change tracking
    buffer->change_tracking_enabled = tracking_was_enabled;
    
    return result;
}
```

---

## 8. Buffer Validation and Integrity

### 8.1 Validation System

```c
typedef struct lle_buffer_validator {
    // Validation configuration
    bool utf8_validation_enabled;      // UTF-8 validation
    bool line_structure_validation;    // Line structure validation
    bool cursor_validation_enabled;    // Cursor position validation
    bool checksum_validation_enabled;  // Content checksum validation
    
    // Validation statistics
    uint32_t validation_count;         // Total validations performed
    uint32_t validation_failures;      // Number of validation failures
    uint32_t corruption_detections;    // Buffer corruption detections
    
    // UTF-8 processor reference
    lle_utf8_processor_t *utf8_processor;
    
} lle_buffer_validator_t;

lle_result_t lle_buffer_validate_complete(lle_buffer_t *buffer,
                                          lle_buffer_validator_t *validator) {
    if (!buffer || !validator) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    validator->validation_count++;
    
    // Step 1: Validate UTF-8 encoding
    if (validator->utf8_validation_enabled) {
        if (!lle_utf8_is_valid(buffer->data, buffer->length)) {
            validator->validation_failures++;
            validator->corruption_detections++;
            return LLE_ERROR_INVALID_UTF8;
        }
    }
    
    // Step 2: Validate null termination
    if (buffer->data[buffer->length] != '\0') {
        validator->validation_failures++;
        validator->corruption_detections++;
        return LLE_ERROR_BUFFER_CORRUPTION;
    }
    
    // Step 3: Validate line structure
    if (validator->line_structure_validation) {
        result = lle_validate_line_structure(buffer);
        if (result != LLE_SUCCESS) {
            validator->validation_failures++;
            return result;
        }
    }
    
    // Step 4: Validate cursor position
    if (validator->cursor_validation_enabled) {
        result = lle_validate_cursor_position(buffer);
        if (result != LLE_SUCCESS) {
            validator->validation_failures++;
            return result;
        }
    }
    
    // Step 5: Validate buffer integrity checksum
    if (validator->checksum_validation_enabled) {
        uint32_t calculated_checksum = lle_calculate_buffer_checksum(buffer->data, 
                                                                     buffer->length);
        if (calculated_checksum != buffer->checksum) {
            validator->validation_failures++;
            validator->corruption_detections++;
            return LLE_ERROR_CHECKSUM_MISMATCH;
        }
    }
    
    // Step 6: Mark buffer as validated
    buffer->integrity_valid = true;
    
    return LLE_SUCCESS;
}
```

---

## 9. Performance Optimization

### 9.1 Performance Requirements

- **Buffer Operations**: < 0.5ms for insert/delete operations
- **UTF-8 Processing**: < 0.1ms for position calculations  
- **Memory Usage**: < 2MB for buffer management system
- **CPU Usage**: < 2% during active editing
- **Cache Hit Rate**: > 80% for position calculations

### 9.2 Performance Monitoring

```c
typedef struct lle_buffer_performance_metrics {
    // Operation timing
    uint64_t total_insert_time;        // Total insert operation time
    uint64_t total_delete_time;        // Total delete operation time
    uint64_t total_cursor_move_time;   // Total cursor movement time
    
    // Operation counts
    uint32_t insert_operations;        // Number of insert operations
    uint32_t delete_operations;        // Number of delete operations
    uint32_t cursor_movements;         // Number of cursor movements
    
    // Cache performance
    uint32_t cache_hits;               // Cache hit count
    uint32_t cache_misses;             // Cache miss count
    double cache_hit_rate;             // Cache hit rate percentage
    
    // Memory usage
    size_t memory_allocated;           // Total allocated memory
    size_t memory_used;                // Actually used memory
    size_t peak_memory_usage;          // Peak memory usage
    
} lle_buffer_performance_metrics_t;
```

---

## 10. Integration with Lush Systems

### 10.1 Memory Pool Integration

```c
lle_result_t lle_buffer_integrate_with_lush_memory(lle_buffer_system_t *buf_sys,
                                                     lush_memory_pool_t *memory_pool) {
    // Register buffer system with memory pool
    lle_result_t result = lush_memory_pool_register_subsystem(
        memory_pool, 
        "lle_buffer_management",
        buf_sys,
        lle_buffer_system_memory_callback
    );
    
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Configure memory pool preferences
    lush_memory_pool_set_alignment(memory_pool, 
                                     LLE_BUFFER_MEMORY_ALIGNMENT);
    lush_memory_pool_set_growth_strategy(memory_pool, 
                                           LUSH_MEMORY_GROWTH_EXPONENTIAL);
    
    return LLE_SUCCESS;
}
```

---

## 11. Testing and Validation

### 11.1 Comprehensive Test Framework

```c
typedef struct lle_buffer_test_suite {
    // Basic functionality tests
    bool (*test_buffer_creation)(void);
    bool (*test_text_insertion)(void);
    bool (*test_text_deletion)(void);
    bool (*test_cursor_movement)(void);
    
    // UTF-8 and Unicode tests
    bool (*test_utf8_handling)(void);
    bool (*test_grapheme_clusters)(void);
    bool (*test_unicode_normalization)(void);
    
    // Multiline buffer tests
    bool (*test_multiline_editing)(void);
    bool (*test_shell_construct_parsing)(void);
    
    // Change tracking tests
    bool (*test_undo_redo)(void);
    bool (*test_atomic_operations)(void);
    
    // Performance tests
    bool (*test_operation_timing)(void);
    bool (*test_memory_efficiency)(void);
    bool (*test_cache_performance)(void);
    
    // Integration tests
    bool (*test_lush_memory_integration)(void);
    bool (*test_display_system_integration)(void);
    
} lle_buffer_test_suite_t;
```

**Implementation Status**: Complete specification ready for development  
**Next Priority**: Event System Complete Specification (04_event_system_complete.md)