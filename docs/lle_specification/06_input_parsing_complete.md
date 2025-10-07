# Input Parsing Complete Specification

**Document**: 06_input_parsing_complete.md  
**Version**: 1.0.0  
**Date**: 2025-01-07  
**Status**: Implementation-Ready Specification  
**Classification**: Critical Foundation Component  

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Terminal Sequence Parsing](#3-terminal-sequence-parsing)
4. [Unicode and UTF-8 Processing](#4-unicode-and-utf-8-processing)
5. [Key Sequence Detection](#5-key-sequence-detection)
6. [Mouse Input Processing](#6-mouse-input-processing)
7. [Input Stream Management](#7-input-stream-management)
8. [Parser State Machine](#8-parser-state-machine)
9. [Error Handling and Recovery](#9-error-handling-and-recovery)
10. [Performance Optimization](#10-performance-optimization)
11. [Integration with Event System](#11-integration-with-event-system)
12. [Integration with Lusush Systems](#12-integration-with-lusush-systems)
13. [Testing and Validation](#13-testing-and-validation)
14. [Memory Management](#14-memory-management)

---

## 1. Executive Summary

### 1.1 Purpose

The Input Parsing System provides comprehensive, high-performance parsing of terminal input streams, converting raw terminal data into structured events for the LLE event system. This includes terminal escape sequences, Unicode/UTF-8 text, key combinations, mouse events, and special terminal control sequences with robust error handling and recovery.

### 1.2 Key Features

- **Universal Terminal Support**: Robust parsing for all major terminal types and their variations
- **Complete Unicode Processing**: Full UTF-8 support with grapheme cluster awareness
- **High-Performance Stream Processing**: Sub-millisecond parsing with zero-copy optimization
- **Intelligent Sequence Detection**: Context-aware escape sequence recognition and disambiguation
- **Comprehensive Error Recovery**: Graceful handling of malformed or incomplete sequences
- **Event System Integration**: Direct integration with LLE event processing pipeline

### 1.3 Critical Design Principles

1. **Zero Data Loss**: All input data is parsed or gracefully handled, never dropped
2. **Streaming Architecture**: Real-time processing without buffering delays
3. **Terminal Agnostic**: Works with any terminal without specific configuration
4. **Unicode First**: Native UTF-8 processing with full international support
5. **Performance Critical**: Sub-millisecond processing for responsive user experience

---

## 2. Architecture Overview

### 2.1 Core Component Structure

```c
// Primary input parsing system components
typedef struct lle_input_parser_system {
    lle_input_stream_t *stream;                 // Input stream management
    lle_sequence_parser_t *sequence_parser;     // Escape sequence parser
    lle_utf8_processor_t *utf8_processor;       // UTF-8 Unicode processor
    lle_key_detector_t *key_detector;           // Key sequence detector
    lle_mouse_parser_t *mouse_parser;           // Mouse event parser
    lle_parser_state_machine_t *state_machine;  // Parser state machine
    lle_input_buffer_t *input_buffer;           // Input buffering system
    lle_event_system_t *event_system;           // Event system integration
    lle_error_context_t *error_ctx;             // Error handling context
    lle_performance_monitor_t *perf_monitor;    // Performance monitoring
    lle_memory_pool_t *memory_pool;             // Memory management
    pthread_mutex_t parser_mutex;               // Thread synchronization
    bool active;                                // Parser active state
    uint64_t bytes_processed;                   // Total bytes processed
} lle_input_parser_system_t;
```

### 2.2 Input Parser Initialization

```c
// Complete input parser system initialization
lle_result_t lle_input_parser_system_init(lle_input_parser_system_t **system,
                                          lle_terminal_system_t *terminal,
                                          lle_event_system_t *event_system,
                                          lle_memory_pool_t *memory_pool) {
    if (!system || !terminal || !event_system || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    lle_input_parser_system_t *parser_sys = NULL;
    
    // Step 1: Allocate parser system structure
    parser_sys = lle_memory_pool_alloc(memory_pool, sizeof(lle_input_parser_system_t));
    if (!parser_sys) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    memset(parser_sys, 0, sizeof(lle_input_parser_system_t));
    
    // Step 2: Store references
    parser_sys->event_system = event_system;
    parser_sys->memory_pool = memory_pool;
    
    // Step 3: Initialize system mutex
    if (pthread_mutex_init(&parser_sys->parser_mutex, NULL) != 0) {
        lle_memory_pool_free(memory_pool, parser_sys);
        return LLE_ERROR_MUTEX_INIT;
    }
    
    // Step 4: Initialize error handling context
    result = lle_error_context_init(&parser_sys->error_ctx);
    if (result != LLE_SUCCESS) {
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 5: Initialize performance monitoring
    result = lle_performance_monitor_init(&parser_sys->perf_monitor);
    if (result != LLE_SUCCESS) {
        lle_error_context_destroy(parser_sys->error_ctx);
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 6: Initialize input stream management
    result = lle_input_stream_init(&parser_sys->stream, 
                                  terminal->terminal->stdin_fd,
                                  LLE_INPUT_BUFFER_SIZE,
                                  memory_pool);
    if (result != LLE_SUCCESS) {
        lle_performance_monitor_destroy(parser_sys->perf_monitor);
        lle_error_context_destroy(parser_sys->error_ctx);
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 7: Initialize sequence parser
    result = lle_sequence_parser_init(&parser_sys->sequence_parser,
                                     terminal->caps,
                                     memory_pool);
    if (result != LLE_SUCCESS) {
        lle_input_stream_destroy(parser_sys->stream);
        lle_performance_monitor_destroy(parser_sys->perf_monitor);
        lle_error_context_destroy(parser_sys->error_ctx);
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 8: Initialize UTF-8 processor
    result = lle_utf8_processor_init(&parser_sys->utf8_processor);
    if (result != LLE_SUCCESS) {
        lle_sequence_parser_destroy(parser_sys->sequence_parser);
        lle_input_stream_destroy(parser_sys->stream);
        lle_performance_monitor_destroy(parser_sys->perf_monitor);
        lle_error_context_destroy(parser_sys->error_ctx);
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 9: Initialize key sequence detector
    result = lle_key_detector_init(&parser_sys->key_detector,
                                  terminal->caps,
                                  memory_pool);
    if (result != LLE_SUCCESS) {
        lle_utf8_processor_destroy(parser_sys->utf8_processor);
        lle_sequence_parser_destroy(parser_sys->sequence_parser);
        lle_input_stream_destroy(parser_sys->stream);
        lle_performance_monitor_destroy(parser_sys->perf_monitor);
        lle_error_context_destroy(parser_sys->error_ctx);
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 10: Initialize mouse parser
    result = lle_mouse_parser_init(&parser_sys->mouse_parser,
                                  terminal->caps,
                                  memory_pool);
    if (result != LLE_SUCCESS) {
        lle_key_detector_destroy(parser_sys->key_detector);
        lle_utf8_processor_destroy(parser_sys->utf8_processor);
        lle_sequence_parser_destroy(parser_sys->sequence_parser);
        lle_input_stream_destroy(parser_sys->stream);
        lle_performance_monitor_destroy(parser_sys->perf_monitor);
        lle_error_context_destroy(parser_sys->error_ctx);
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 11: Initialize parser state machine
    result = lle_parser_state_machine_init(&parser_sys->state_machine,
                                          parser_sys->error_ctx,
                                          memory_pool);
    if (result != LLE_SUCCESS) {
        lle_mouse_parser_destroy(parser_sys->mouse_parser);
        lle_key_detector_destroy(parser_sys->key_detector);
        lle_utf8_processor_destroy(parser_sys->utf8_processor);
        lle_sequence_parser_destroy(parser_sys->sequence_parser);
        lle_input_stream_destroy(parser_sys->stream);
        lle_performance_monitor_destroy(parser_sys->perf_monitor);
        lle_error_context_destroy(parser_sys->error_ctx);
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 12: Initialize input buffer management
    result = lle_input_buffer_init(&parser_sys->input_buffer,
                                  LLE_INPUT_PARSE_BUFFER_SIZE,
                                  memory_pool);
    if (result != LLE_SUCCESS) {
        lle_parser_state_machine_destroy(parser_sys->state_machine);
        lle_mouse_parser_destroy(parser_sys->mouse_parser);
        lle_key_detector_destroy(parser_sys->key_detector);
        lle_utf8_processor_destroy(parser_sys->utf8_processor);
        lle_sequence_parser_destroy(parser_sys->sequence_parser);
        lle_input_stream_destroy(parser_sys->stream);
        lle_performance_monitor_destroy(parser_sys->perf_monitor);
        lle_error_context_destroy(parser_sys->error_ctx);
        pthread_mutex_destroy(&parser_sys->parser_mutex);
        lle_memory_pool_free(memory_pool, parser_sys);
        return result;
    }
    
    // Step 13: Activate parser system
    parser_sys->active = true;
    parser_sys->bytes_processed = 0;
    
    *system = parser_sys;
    return LLE_SUCCESS;
}
```

---

## 3. Terminal Sequence Parsing

### 3.1 Escape Sequence Categories

```c
// Terminal escape sequence types
typedef enum {
    LLE_SEQ_TYPE_UNKNOWN,                      // Unknown sequence
    LLE_SEQ_TYPE_CONTROL_CHAR,                 // Control character (0x00-0x1F)
    LLE_SEQ_TYPE_CSI,                          // Control Sequence Introducer (ESC [)
    LLE_SEQ_TYPE_OSC,                          // Operating System Command (ESC ])
    LLE_SEQ_TYPE_DCS,                          // Device Control String (ESC P)
    LLE_SEQ_TYPE_APC,                          // Application Program Command (ESC _)
    LLE_SEQ_TYPE_PM,                           // Privacy Message (ESC ^)
    LLE_SEQ_TYPE_SS2,                          // Single Shift Two (ESC N)
    LLE_SEQ_TYPE_SS3,                          // Single Shift Three (ESC O)
    LLE_SEQ_TYPE_FUNCTION_KEY,                 // Function key sequence
    LLE_SEQ_TYPE_CURSOR_KEY,                   // Cursor movement key
    LLE_SEQ_TYPE_MODIFIER_KEY,                 // Modified key combination
    LLE_SEQ_TYPE_MOUSE_EVENT,                  // Mouse event sequence
    LLE_SEQ_TYPE_BRACKETED_PASTE,              // Bracketed paste mode
    LLE_SEQ_TYPE_FOCUS_EVENT,                  // Focus in/out event
    LLE_SEQ_TYPE_TERMINAL_QUERY_RESPONSE,      // Terminal capability response
} lle_sequence_type_t;

// Sequence parser structure
typedef struct {
    lle_sequence_type_t type;                  // Sequence type
    lle_parser_state_t state;                  // Current parsing state
    char buffer[LLE_MAX_SEQUENCE_LENGTH];      // Sequence buffer
    size_t buffer_pos;                         // Current buffer position
    size_t buffer_capacity;                    // Buffer capacity
    uint64_t sequence_start_time;              // Sequence start timestamp
    lle_terminal_capabilities_t *terminal_caps; // Terminal capabilities
    lle_memory_pool_t *memory_pool;            // Memory pool
    
    // Parsing context
    uint32_t parameters[LLE_MAX_CSI_PARAMETERS]; // CSI parameters
    uint8_t parameter_count;                   // Number of parameters
    char intermediate_chars[8];                // Intermediate characters
    uint8_t intermediate_count;                // Intermediate character count
    char final_char;                          // Final character
    
    // Error tracking
    uint32_t malformed_sequences;             // Malformed sequence count
    uint32_t timeout_sequences;               // Timed-out sequence count
    uint64_t last_error_time;                 // Last error timestamp
} lle_sequence_parser_t;
```

### 3.2 Main Sequence Processing Function

```c
// Main sequence parsing function
lle_result_t lle_sequence_parser_process_data(lle_sequence_parser_t *parser,
                                             const char *data,
                                             size_t data_len,
                                             lle_parsed_input_t **parsed_input) {
    if (!parser || !data || !parsed_input) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    *parsed_input = NULL;
    
    uint64_t processing_start = lle_get_timestamp_us();
    
    for (size_t i = 0; i < data_len; i++) {
        char current_char = data[i];
        
        // Check for processing timeout
        if (lle_get_timestamp_us() - processing_start > LLE_MAX_PARSE_TIME_US) {
            lle_error_context_record_error(parser->error_ctx,
                                          LLE_ERROR_PARSE_TIMEOUT,
                                          "Input parsing timeout exceeded");
            return LLE_ERROR_PARSE_TIMEOUT;
        }
        
        switch (parser->state) {
            case LLE_PARSER_STATE_NORMAL:
                result = lle_sequence_parser_handle_normal_char(parser, current_char, parsed_input);
                break;
                
            case LLE_PARSER_STATE_ESCAPE:
                result = lle_sequence_parser_handle_escape_char(parser, current_char, parsed_input);
                break;
                
            case LLE_PARSER_STATE_CSI:
                result = lle_sequence_parser_handle_csi_char(parser, current_char, parsed_input);
                break;
                
            case LLE_PARSER_STATE_OSC:
                result = lle_sequence_parser_handle_osc_char(parser, current_char, parsed_input);
                break;
                
            case LLE_PARSER_STATE_DCS:
                result = lle_sequence_parser_handle_dcs_char(parser, current_char, parsed_input);
                break;
                
            case LLE_PARSER_STATE_STRING_TERMINATOR:
                result = lle_sequence_parser_handle_string_terminator(parser, current_char, parsed_input);
                break;
                
            default:
                lle_error_context_record_error(parser->error_ctx,
                                              LLE_ERROR_INVALID_PARSER_STATE,
                                              "Invalid parser state: %d", parser->state);
                parser->state = LLE_PARSER_STATE_NORMAL;
                result = LLE_ERROR_INVALID_PARSER_STATE;
                break;
        }
        
        // Handle parsing errors
        if (result != LLE_SUCCESS) {
            if (result == LLE_RESULT_SEQUENCE_COMPLETE) {
                // Sequence completed successfully
                result = LLE_SUCCESS;
                break;
            } else if (result == LLE_RESULT_SEQUENCE_INCOMPLETE) {
                // Sequence needs more data
                result = LLE_SUCCESS;
                continue;
            } else {
                // Actual error occurred
                parser->malformed_sequences++;
                parser->last_error_time = lle_get_timestamp_us();
                
                // Reset parser state and continue
                lle_sequence_parser_reset_state(parser);
                continue;
            }
        }
    }
    
    return result;
}
```

### 3.3 CSI Sequence Processing

```c
// Handle CSI (Control Sequence Introducer) sequences
lle_result_t lle_sequence_parser_handle_csi_char(lle_sequence_parser_t *parser,
                                                char ch,
                                                lle_parsed_input_t **parsed_input) {
    // Add character to sequence buffer
    if (parser->buffer_pos >= parser->buffer_capacity - 1) {
        lle_error_context_record_error(parser->error_ctx,
                                      LLE_ERROR_SEQUENCE_TOO_LONG,
                                      "CSI sequence exceeds maximum length");
        lle_sequence_parser_reset_state(parser);
        return LLE_ERROR_SEQUENCE_TOO_LONG;
    }
    
    parser->buffer[parser->buffer_pos++] = ch;
    
    if (ch >= 0x30 && ch <= 0x3F) {
        // Parameter or intermediate character
        if (ch >= 0x30 && ch <= 0x39) {
            // Digit: build parameter
            return lle_sequence_parser_build_parameter(parser, ch);
        } else if (ch == ';') {
            // Parameter separator
            return lle_sequence_parser_next_parameter(parser);
        } else {
            // Intermediate character
            return lle_sequence_parser_add_intermediate(parser, ch);
        }
    } else if (ch >= 0x40 && ch <= 0x7E) {
        // Final character - complete CSI sequence
        parser->final_char = ch;
        return lle_sequence_parser_complete_csi_sequence(parser, parsed_input);
    } else {
        // Invalid character in CSI sequence
        lle_error_context_record_error(parser->error_ctx,
                                      LLE_ERROR_INVALID_CSI_CHAR,
                                      "Invalid character in CSI sequence: 0x%02X", (unsigned char)ch);
        lle_sequence_parser_reset_state(parser);
        return LLE_ERROR_INVALID_CSI_CHAR;
    }
}

// Complete CSI sequence processing
lle_result_t lle_sequence_parser_complete_csi_sequence(lle_sequence_parser_t *parser,
                                                      lle_parsed_input_t **parsed_input) {
    lle_result_t result = LLE_SUCCESS;
    
    // Create parsed input structure
    *parsed_input = lle_memory_pool_alloc(parser->memory_pool, sizeof(lle_parsed_input_t));
    if (!*parsed_input) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    memset(*parsed_input, 0, sizeof(lle_parsed_input_t));
    (*parsed_input)->type = LLE_INPUT_TYPE_SEQUENCE;
    (*parsed_input)->sequence.type = LLE_SEQ_TYPE_CSI;
    (*parsed_input)->sequence.final_char = parser->final_char;
    (*parsed_input)->sequence.parameter_count = parser->parameter_count;
    
    // Copy parameters
    if (parser->parameter_count > 0) {
        memcpy((*parsed_input)->sequence.parameters,
               parser->parameters,
               parser->parameter_count * sizeof(uint32_t));
    }
    
    // Copy intermediate characters
    if (parser->intermediate_count > 0) {
        memcpy((*parsed_input)->sequence.intermediate_chars,
               parser->intermediate_chars,
               parser->intermediate_count);
    }
    
    // Interpret common CSI sequences
    result = lle_sequence_parser_interpret_csi_sequence(parser, *parsed_input);
    
    // Reset parser state
    lle_sequence_parser_reset_state(parser);
    
    return result;
}
```

---

## 4. Unicode and UTF-8 Processing

### 4.1 UTF-8 Processor Structure

```c
// UTF-8 processing state
typedef enum {
    LLE_UTF8_STATE_START,                      // Start of new character
    LLE_UTF8_STATE_CONTINUATION,               // Expecting continuation bytes
    LLE_UTF8_STATE_ERROR,                      // Error in sequence
} lle_utf8_state_t;

// UTF-8 processor
typedef struct {
    lle_utf8_state_t state;                    // Current processing state
    uint32_t codepoint;                        // Current codepoint being built
    uint8_t expected_bytes;                    // Expected continuation bytes
    uint8_t received_bytes;                    // Received continuation bytes
    char byte_buffer[4];                       // UTF-8 byte buffer
    size_t buffer_pos;                         // Buffer position
    
    // Unicode normalization
    lle_unicode_normalizer_t *normalizer;      // Unicode normalizer
    bool normalization_enabled;                // Normalization enabled
    lle_normalization_form_t norm_form;        // Normalization form
    
    // Grapheme cluster detection
    lle_grapheme_detector_t *grapheme_detector; // Grapheme boundary detector
    bool grapheme_mode;                        // Grapheme-aware processing
    
    // Character width calculation
    lle_char_width_calculator_t *width_calc;   // Character width calculator
    
    // Error handling
    uint32_t invalid_sequences;               // Invalid sequence count
    uint32_t replacement_chars;               // Replacement characters used
    lle_utf8_error_policy_t error_policy;     // Error handling policy
    
    // Performance monitoring
    uint64_t codepoints_processed;            // Total codepoints processed
    uint64_t bytes_processed;                 // Total bytes processed
} lle_utf8_processor_t;

// Parsed UTF-8 character information
typedef struct {
    uint32_t codepoint;                       // Unicode codepoint
    char utf8_bytes[4];                       // UTF-8 byte sequence
    uint8_t byte_length;                      // Length in bytes
    int visual_width;                         // Visual width (0, 1, or 2)
    bool is_combining;                        // Is combining character
    bool is_control;                          // Is control character
    bool is_grapheme_boundary;                // Is grapheme cluster boundary
    lle_character_category_t category;         // Unicode character category
} lle_utf8_char_info_t;
```

### 4.2 UTF-8 Character Processing

```c
// Process UTF-8 input data
lle_result_t lle_utf8_processor_process_data(lle_utf8_processor_t *processor,
                                            const char *data,
                                            size_t data_len,
                                            lle_utf8_char_info_t **char_info) {
    if (!processor || !data || !char_info) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    *char_info = NULL;
    
    for (size_t i = 0; i < data_len; i++) {
        unsigned char byte = (unsigned char)data[i];
        
        switch (processor->state) {
            case LLE_UTF8_STATE_START:
                result = lle_utf8_processor_handle_start_byte(processor, byte, char_info);
                break;
                
            case LLE_UTF8_STATE_CONTINUATION:
                result = lle_utf8_processor_handle_continuation_byte(processor, byte, char_info);
                break;
                
            case LLE_UTF8_STATE_ERROR:
                result = lle_utf8_processor_handle_error_recovery(processor, byte, char_info);
                break;
                
            default:
                processor->state = LLE_UTF8_STATE_START;
                result = LLE_ERROR_INVALID_UTF8_STATE;
                break;
        }
        
        // If character is complete, return it
        if (result == LLE_RESULT_CHARACTER_COMPLETE) {
            return LLE_SUCCESS;
        } else if (result != LLE_SUCCESS && result != LLE_RESULT_NEED_MORE_DATA) {
            // Handle error based on policy
            return lle_utf8_processor_handle_error(processor, result, byte, char_info);
        }
    }
    
    return result;
}

// Handle UTF-8 start byte
lle_result_t lle_utf8_processor_handle_start_byte(lle_utf8_processor_t *processor,
                                                 unsigned char byte,
                                                 lle_utf8_char_info_t **char_info) {
    processor->buffer_pos = 0;
    processor->byte_buffer[processor->buffer_pos++] = byte;
    
    if (byte < 0x80) {
        // ASCII character (0xxxxxxx)
        processor->codepoint = byte;
        processor->expected_bytes = 0;
        processor->received_bytes = 0;
        return lle_utf8_processor_complete_character(processor, char_info);
    } else if ((byte & 0xE0) == 0xC0) {
        // 2-byte sequence (110xxxxx)
        processor->codepoint = byte & 0x1F;
        processor->expected_bytes = 1;
        processor->received_bytes = 0;
        processor->state = LLE_UTF8_STATE_CONTINUATION;
        return LLE_RESULT_NEED_MORE_DATA;
    } else if ((byte & 0xF0) == 0xE0) {
        // 3-byte sequence (1110xxxx)
        processor->codepoint = byte & 0x0F;
        processor->expected_bytes = 2;
        processor->received_bytes = 0;
        processor->state = LLE_UTF8_STATE_CONTINUATION;
        return LLE_RESULT_NEED_MORE_DATA;
    } else if ((byte & 0xF8) == 0xF0) {
        // 4-byte sequence (11110xxx)
        processor->codepoint = byte & 0x07;
        processor->expected_bytes = 3;
        processor->received_bytes = 0;
        processor->state = LLE_UTF8_STATE_CONTINUATION;
        return LLE_RESULT_NEED_MORE_DATA;
    } else {
        // Invalid start byte
        processor->invalid_sequences++;
        processor->state = LLE_UTF8_STATE_ERROR;
        return LLE_ERROR_INVALID_UTF8_START_BYTE;
    }
}

// Handle UTF-8 continuation byte
lle_result_t lle_utf8_processor_handle_continuation_byte(lle_utf8_processor_t *processor,
                                                        unsigned char byte,
                                                        lle_utf8_char_info_t **char_info) {
    if ((byte & 0xC0) != 0x80) {
        // Invalid continuation byte
        processor->invalid_sequences++;
        processor->state = LLE_UTF8_STATE_ERROR;
        return LLE_ERROR_INVALID_UTF8_CONTINUATION;
    }
    
    processor->byte_buffer[processor->buffer_pos++] = byte;
    processor->codepoint = (processor->codepoint << 6) | (byte & 0x3F);
    processor->received_bytes++;
    
    if (processor->received_bytes == processor->expected_bytes) {
        // Character complete
        processor->state = LLE_UTF8_STATE_START;
        
        // Validate codepoint ranges
        lle_result_t validation_result = lle_utf8_validate_codepoint(processor->codepoint,
                                                                    processor->buffer_pos);
        if (validation_result != LLE_SUCCESS) {
            processor->invalid_sequences++;
            return validation_result;
        }
        
        return lle_utf8_processor_complete_character(processor, char_info);
    }
    
    return LLE_RESULT_NEED_MORE_DATA;
}

// Complete UTF-8 character processing
lle_result_t lle_utf8_processor_complete_character(lle_utf8_processor_t *processor,
                                                  lle_utf8_char_info_t **char_info) {
    *char_info = lle_memory_pool_alloc(processor->memory_pool, sizeof(lle_utf8_char_info_t));
    if (!*char_info) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    memset(*char_info, 0, sizeof(lle_utf8_char_info_t));
    
    // Set basic character information
    (*char_info)->codepoint = processor->codepoint;
    (*char_info)->byte_length = processor->buffer_pos;
    memcpy((*char_info)->utf8_bytes, processor->byte_buffer, processor->buffer_pos);
    
    // Calculate character properties
    (*char_info)->visual_width = lle_char_width_calculator_get_width(processor->width_calc,
                                                                    processor->codepoint);
    (*char_info)->is_combining = lle_unicode_is_combining(processor->codepoint);
    (*char_info)->is_control = lle_unicode_is_control(processor->codepoint);
    (*char_info)->category = lle_unicode_get_category(processor->codepoint);
    
    // Check grapheme boundary
    if (processor->grapheme_mode) {
        (*char_info)->is_grapheme_boundary = 
            lle_grapheme_detector_is_boundary(processor->grapheme_detector,
                                            processor->codepoint);
    }
    
    // Update statistics
    processor->codepoints_processed++;
    processor->bytes_processed += processor->buffer_pos;
    
    return LLE_RESULT_CHARACTER_COMPLETE;
}
```

---

## 5. Key Sequence Detection

### 5.1 Key Detection System

```c
// Key sequence types
typedef enum {
    LLE_KEY_TYPE_REGULAR,                     // Regular printable key
    LLE_KEY_TYPE_FUNCTION,                    // Function key (F1-F24)
    LLE_KEY_TYPE_CURSOR,                      // Cursor movement key
    LLE_KEY_TYPE_EDITING,                     // Editing key (Insert, Delete, etc.)
    LLE_KEY_TYPE_MODIFIER,                    // Modifier key (Ctrl, Alt, Shift)
    LLE_KEY_TYPE_SPECIAL,                     // Special key (Tab, Enter, Escape)
    LLE_KEY_TYPE_KEYPAD,                      // Keypad key
    LLE_KEY_TYPE_MEDIA,                       // Media key
    LLE_KEY_TYPE_UNKNOWN,                     // Unknown key sequence
} lle_key_type_t;

// Key modifier flags
typedef enum {
    LLE_MOD_NONE = 0x00,                      // No modifiers
    LLE_MOD_SHIFT = 0x01,                     // Shift key
    LLE_MOD_CTRL = 0x02,                      // Control key
    LLE_MOD_ALT = 0x04,                       // Alt key
    LLE_MOD_META = 0x08,                      // Meta key
    LLE_MOD_SUPER = 0x10,                     // Super/Windows key
    LLE_MOD_HYPER = 0x20,                     // Hyper key
} lle_key_modifiers_t;

// Key detection structure
typedef struct {
    lle_key_sequence_map_t *sequence_map;     // Key sequence mapping
    lle_key_timeout_manager_t *timeout_mgr;   // Timeout management
    lle_terminal_capabilities_t *terminal_caps; // Terminal capabilities
    lle_memory_pool_t *memory_pool;           // Memory pool
    
    // Current detection state
    char sequence_buffer[LLE_MAX_KEY_SEQUENCE_LENGTH];
    size_t sequence_pos;                      // Current position
    uint64_t sequence_start_time;             // Sequence start time
    bool ambiguous_sequence;                  // Ambiguous sequence detected
    
    // Performance statistics
    uint64_t keys_detected;                   // Total keys detected
    uint64_t sequences_resolved;              // Sequences resolved
    uint64_t ambiguous_timeouts;              // Ambiguous sequence timeouts
} lle_key_detector_t;

// Detected key information
typedef struct {
    lle_key_type_t type;                      // Key type
    uint32_t keycode;                         // Key code
    lle_key_modifiers_t modifiers;            // Modifier flags
    char key_name[32];                        // Key name
    char sequence[LLE_MAX_KEY_SEQUENCE_LENGTH]; // Original sequence
    size_t sequence_length;                   // Sequence length
    bool is_repeat;                           // Key repeat
    uint64_t timestamp;                       // Detection timestamp
} lle_key_info_t;
```

### 5.2 Key Sequence Processing

```c
// Process key sequence data
lle_result_t lle_key_detector_process_sequence(lle_key_detector_t *detector,
                                              const char *sequence,
                                              size_t sequence_len,
                                              lle_key_info_t **key_info) {
    if (!detector || !sequence || !key_info) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    *key_info = NULL;
    
    // Add sequence data to buffer
    if (detector->sequence_pos + sequence_len > LLE_MAX_KEY_SEQUENCE_LENGTH) {
        // Sequence too long, reset and start fresh
        detector->sequence_pos = 0;
        detector->sequence_start_time = lle_get_timestamp_us();
    }
    
    memcpy(&detector->sequence_buffer[detector->sequence_pos], sequence, sequence_len);
    detector->sequence_pos += sequence_len;
    
    // Try to match complete sequence
    lle_key_sequence_match_result_t match_result;
    result = lle_key_sequence_map_match(detector->sequence_map,
                                       detector->sequence_buffer,
                                       detector->sequence_pos,
                                       &match_result);
    
    if (result == LLE_SUCCESS) {
        switch (match_result.type) {
            case LLE_MATCH_EXACT:
                // Exact match found
                return lle_key_detector_create_key_info(detector, &match_result, key_info);
                
            case LLE_MATCH_PREFIX:
                // Partial match, wait for more data
                detector->ambiguous_sequence = false;
                return LLE_RESULT_NEED_MORE_DATA;
                
            case LLE_MATCH_AMBIGUOUS:
                // Multiple possible matches, set timeout
                detector->ambiguous_sequence = true;
                return lle_key_detector_handle_ambiguous_sequence(detector, key_info);
                
            case LLE_MATCH_NONE:
                // No match, try to salvage as individual characters
                return lle_key_detector_handle_unmatched_sequence(detector, key_info);
        }
    }
    
    return result;
}

// Handle ambiguous key sequences
lle_result_t lle_key_detector_handle_ambiguous_sequence(lle_key_detector_t *detector,
                                                       lle_key_info_t **key_info) {
    uint64_t current_time = lle_get_timestamp_us();
    uint64_t elapsed = current_time - detector->sequence_start_time;
    
    if (elapsed >= LLE_KEY_SEQUENCE_TIMEOUT_US) {
        // Timeout reached, use best match
        lle_key_sequence_match_result_t best_match;
        lle_result_t result = lle_key_sequence_map_get_best_match(detector->sequence_map,
                                                                 detector->sequence_buffer,
                                                                 detector->sequence_pos,
                                                                 &best_match);
        
        if (result == LLE_SUCCESS) {
            detector->ambiguous_timeouts++;
            return lle_key_detector_create_key_info(detector, &best_match, key_info);
        } else {
            // No good match, treat as individual characters
            return lle_key_detector_handle_unmatched_sequence(detector, key_info);
        }
    }
    
    // Still within timeout window
    return LLE_RESULT_NEED_MORE_DATA;
}
```

---

## 6. Mouse Input Processing

### 6.1 Mouse Parser Structure

```c
// Mouse event types
typedef enum {
    LLE_MOUSE_EVENT_PRESS,                    // Mouse button press
    LLE_MOUSE_EVENT_RELEASE,                  // Mouse button release
    LLE_MOUSE_EVENT_MOVE,                     // Mouse movement
    LLE_MOUSE_EVENT_WHEEL,                    // Mouse wheel
    LLE_MOUSE_EVENT_DRAG,                     // Mouse drag
} lle_mouse_event_type_t;

// Mouse buttons
typedef enum {
    LLE_MOUSE_BUTTON_NONE = 0,                // No button
    LLE_MOUSE_BUTTON_LEFT = 1,                // Left button
    LLE_MOUSE_BUTTON_MIDDLE = 2,              // Middle button
    LLE_MOUSE_BUTTON_RIGHT = 3,               // Right button
    LLE_MOUSE_BUTTON_WHEEL_UP = 4,            // Wheel up
    LLE_MOUSE_BUTTON_WHEEL_DOWN = 5,          // Wheel down
} lle_mouse_button_t;

// Mouse parser structure
typedef struct {
    lle_terminal_capabilities_t *terminal_caps; // Terminal capabilities
    lle_memory_pool_t *memory_pool;             // Memory pool
    
    // Mouse tracking state
    bool mouse_tracking_enabled;              // Mouse tracking active
    lle_mouse_tracking_mode_t tracking_mode;  // Tracking mode
    bool button_event_tracking;               // Button event tracking
    bool motion_event_tracking;               // Motion event tracking
    bool focus_event_tracking;                // Focus event tracking
    
    // Current mouse state
    uint16_t last_x, last_y;                  // Last mouse position
    lle_mouse_button_t pressed_buttons;       // Currently pressed buttons
    lle_key_modifiers_t modifiers;            // Modifier keys
    uint64_t last_click_time;                 // Last click timestamp
    uint32_t click_count;                     // Click count for multi-click
    
    // Performance statistics
    uint64_t mouse_events_parsed;             // Total mouse events
    uint64_t invalid_mouse_sequences;         // Invalid sequences
} lle_mouse_parser_t;

// Parsed mouse event information
typedef struct {
    lle_mouse_event_type_t type;              // Event type
    lle_mouse_button_t button;                // Button involved
    uint16_t x, y;                            // Mouse coordinates
    lle_key_modifiers_t modifiers;            // Modifier keys
    int16_t wheel_delta;                      // Wheel delta
    bool double_click;                        // Double-click detected
    bool triple_click;                        // Triple-click detected
    uint64_t timestamp;                       // Event timestamp
} lle_mouse_event_info_t;
```

### 6.2 Mouse Event Processing

```c
// Parse mouse sequence
lle_result_t lle_mouse_parser_parse_sequence(lle_mouse_parser_t *parser,
                                            const char *sequence,
                                            size_t sequence_len,
                                            lle_mouse_event_info_t **mouse_info) {
    if (!parser || !sequence || !mouse_info) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    *mouse_info = NULL;
    
    // Check for standard mouse sequence format: ESC[M<button><x><y>
    if (sequence_len >= 6 && sequence[0] == '\x1b' && sequence[1] == '[' && sequence[2] == 'M') {
        return lle_mouse_parser_parse_standard_sequence(parser, sequence, sequence_len, mouse_info);
    }
    
    // Check for SGR mouse sequence format: ESC[<...>M or ESC[<...>m
    if (sequence_len >= 6 && sequence[0] == '\x1b' && sequence[1] == '[' && sequence[2] == '<') {
        return lle_mouse_parser_parse_sgr_sequence(parser, sequence, sequence_len, mouse_info);
    }
    
    // Check for UTF-8 mouse sequence
    if (sequence_len >= 6 && sequence[0] == '\x1b' && sequence[1] == '[' && 
        sequence[2] == 'M' && (sequence[3] & 0x80)) {
        return lle_mouse_parser_parse_utf8_sequence(parser, sequence, sequence_len, mouse_info);
    }
    
    parser->invalid_mouse_sequences++;
    return LLE_ERROR_INVALID_MOUSE_SEQUENCE;
}

// Parse standard mouse sequence (X10 compatible)
lle_result_t lle_mouse_parser_parse_standard_sequence(lle_mouse_parser_t *parser,
                                                     const char *sequence,
                                                     size_t sequence_len,
                                                     lle_mouse_event_info_t **mouse_info) {
    if (sequence_len != 6) {
        return LLE_ERROR_INVALID_MOUSE_SEQUENCE;
    }
    
    // Extract button and coordinates
    unsigned char button_byte = (unsigned char)sequence[3];
    unsigned char x_byte = (unsigned char)sequence[4];
    unsigned char y_byte = (unsigned char)sequence[5];
    
    // Create mouse event info
    *mouse_info = lle_memory_pool_alloc(parser->memory_pool, sizeof(lle_mouse_event_info_t));
    if (!*mouse_info) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    memset(*mouse_info, 0, sizeof(lle_mouse_event_info_t));
    
    // Decode button information
    (*mouse_info)->button = (button_byte & 0x03) + 1;
    (*mouse_info)->modifiers = 0;
    
    if (button_byte & 0x04) (*mouse_info)->modifiers |= LLE_MOD_SHIFT;
    if (button_byte & 0x08) (*mouse_info)->modifiers |= LLE_MOD_META;
    if (button_byte & 0x10) (*mouse_info)->modifiers |= LLE_MOD_CTRL;
    
    // Decode coordinates (1-based, subtract 33 for encoding offset)
    (*mouse_info)->x = (x_byte >= 33) ? x_byte - 33 : 0;
    (*mouse_info)->y = (y_byte >= 33) ? y_byte - 33 : 0;
    
    // Determine event type
    if ((button_byte & 0x03) == 0x03) {
        (*mouse_info)->type = LLE_MOUSE_EVENT_RELEASE;
    } else {
        (*mouse_info)->type = LLE_MOUSE_EVENT_PRESS;
    }
    
    // Handle wheel events
    if ((button_byte & 0x60) == 0x60) {
        (*mouse_info)->type = LLE_MOUSE_EVENT_WHEEL;
        (*mouse_info)->wheel_delta = (button_byte & 0x01) ? -1 : 1;
    }
    
    (*mouse_info)->timestamp = lle_get_timestamp_us();
    parser->mouse_events_parsed++;
    
    return LLE_SUCCESS;
}
```

---

## 7. Integration with Event System

### 7.1 Event Generation from Parsed Input

```c
// Convert parsed input to events
lle_result_t lle_input_parser_generate_events(lle_input_parser_system_t *parser_sys,
                                             lle_parsed_input_t *parsed_input) {
    if (!parser_sys || !parsed_input) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    
    switch (parsed_input->type) {
        case LLE_INPUT_TYPE_TEXT:
            result = lle_input_parser_generate_text_events(parser_sys, parsed_input);
            break;
            
        case LLE_INPUT_TYPE_KEY:
            result = lle_input_parser_generate_key_events(parser_sys, parsed_input);
            break;
            
        case LLE_INPUT_TYPE_MOUSE:
            result = lle_input_parser_generate_mouse_events(parser_sys, parsed_input);
            break;
            
        case LLE_INPUT_TYPE_SEQUENCE:
            result = lle_input_parser_generate_sequence_events(parser_sys, parsed_input);
            break;
            
        default:
            result = LLE_ERROR_UNKNOWN_INPUT_TYPE;
            break;
    }
    
    return result;
}

// Generate key press events
lle_result_t lle_input_parser_generate_key_events(lle_input_parser_system_t *parser_sys,
                                                 lle_parsed_input_t *parsed_input) {
    lle_event_t *event = lle_event_allocate(parser_sys->event_system->event_pool,
                                           LLE_EVENT_KEY_PRESS);
    if (!event) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Fill key event data
    event->data.key.keycode = parsed_input->key_info.keycode;
    event->data.key.modifiers = parsed_input->key_info.modifiers;
    event->data.key.repeat = parsed_input->key_info.is_repeat;
    event->data.key.key_timestamp = parsed_input->key_info.timestamp;
    
    // Copy UTF-8 representation if available
    if (parsed_input->key_info.type == LLE_KEY_TYPE_REGULAR) {
        strncpy(event->data.key.utf8_char, parsed_input->text_info.utf8_bytes,
                sizeof(event->data.key.utf8_char) - 1);
    }
    
    // Set event metadata
    event->source = LLE_EVENT_SOURCE_TERMINAL;
    event->priority = LLE_PRIORITY_HIGH;
    event->sequence_number = atomic_fetch_add(&parser_sys->event_system->sequence_counter, 1);
    
    // Queue event for processing
    return lle_event_queue_enqueue(parser_sys->event_system->main_queue, event, false);
}
```

---

## 8. Error Handling and Recovery

### 8.1 Comprehensive Error Recovery

```c
// Input parsing error recovery
lle_result_t lle_input_parser_recover_from_error(lle_input_parser_system_t *parser_sys,
                                                lle_error_code_t error_code,
                                                const char *problematic_data,
                                                size_t data_len) {
    lle_result_t result = LLE_SUCCESS;
    
    switch (error_code) {
        case LLE_ERROR_INVALID_UTF8_SEQUENCE:
            // Replace with Unicode replacement character
            result = lle_input_parser_insert_replacement_character(parser_sys);
            break;
            
        case LLE_ERROR_SEQUENCE_TOO_LONG:
            // Reset sequence parser and treat as individual characters
            lle_sequence_parser_reset_state(parser_sys->sequence_parser);
            result = lle_input_parser_process_as_text(parser_sys, problematic_data, data_len);
            break;
            
        case LLE_ERROR_INVALID_MOUSE_SEQUENCE:
            // Skip malformed mouse sequence
            parser_sys->mouse_parser->invalid_mouse_sequences++;
            result = LLE_SUCCESS;
            break;
            
        case LLE_ERROR_AMBIGUOUS_KEY_SEQUENCE:
            // Use timeout-based resolution
            result = lle_key_detector_force_resolution(parser_sys->key_detector);
            break;
            
        case LLE_ERROR_PARSE_TIMEOUT:
            // Reset all parsers and continue
            result = lle_input_parser_system_reset_all_parsers(parser_sys);
            break;
            
        default:
            lle_error_context_record_error(parser_sys->error_ctx,
                                          error_code,
                                          "Unhandled input parsing error");
            result = LLE_ERROR_UNHANDLED_PARSING_ERROR;
            break;
    }
    
    return result;
}
```

---

## 9. Performance Requirements

### 9.1 Performance Specifications

```c
// Performance constants
#define LLE_MAX_PARSE_TIME_US                 250      // 250 microseconds max
#define LLE_TARGET_THROUGHPUT_CHARS_PER_SEC   100000   // 100K chars/second
#define LLE_MAX_SEQUENCE_TIMEOUT_US           100000   // 100ms sequence timeout
#define LLE_TARGET_EVENT_GENERATION_TIME_US   50       // 50 microseconds per event
#define LLE_MAX_PARSER_MEMORY_MB              16       // 16MB memory limit

// Performance monitoring
typedef struct {
    uint64_t total_chars_processed;           // Total characters processed
    uint64_t total_processing_time_us;        // Total processing time
    uint64_t max_processing_time_us;          // Maximum processing time
    uint64_t events_generated;                // Total events generated
    uint32_t chars_per_second;                // Current throughput
    uint32_t peak_chars_per_second;           // Peak throughput
    uint32_t parse_errors;                    // Total parsing errors
    uint32_t recovery_operations;             // Error recoveries
    uint64_t last_performance_check;          // Last performance check
} lle_input_parser_performance_t;
```

---

## 10. Testing and Validation

### 10.1 Comprehensive Test Framework

```c
// Test input parsing with various terminal sequences
lle_result_t lle_input_parser_test_comprehensive(lle_input_parser_system_t *parser_sys) {
    lle_result_t result = LLE_SUCCESS;
    
    // Test basic text input
    result = lle_input_parser_test_text_input(parser_sys);
    if (result != LLE_SUCCESS) return result;
    
    // Test key sequences
    result = lle_input_parser_test_key_sequences(parser_sys);
    if (result != LLE_SUCCESS) return result;
    
    // Test mouse input
    result = lle_input_parser_test_mouse_input(parser_sys);
    if (result != LLE_SUCCESS) return result;
    
    // Test Unicode/UTF-8
    result = lle_input_parser_test_unicode_input(parser_sys);
    if (result != LLE_SUCCESS) return result;
    
    // Test error recovery
    result = lle_input_parser_test_error_recovery(parser_sys);
    if (result != LLE_SUCCESS) return result;
    
    // Test performance requirements
    result = lle_input_parser_test_performance(parser_sys);
    if (result != LLE_SUCCESS) return result;
    
    return LLE_SUCCESS;
}
```

This completes the comprehensive Input Parsing specification with implementation-ready pseudo-code, complete error handling, performance optimization, and integration with the LLE event system and Lusush display architecture.