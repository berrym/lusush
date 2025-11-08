# Phase 1: Continuation Prompt Layer API Design

**Document**: phase1_continuation_prompt_layer_api_design.md  
**Date**: 2025-11-08  
**Phase**: 1 - Detailed Design  
**Status**: Design Complete - Ready for Review  
**Purpose**: Complete API specification for continuation_prompt_layer

---

## Overview

The continuation_prompt_layer provides visual feedback for multiline input by displaying continuation prompts (e.g., `> `, `loop>`, `if>`) on lines following the initial prompt.

### Design Principles

1. **Consistency with existing layers**: Follow prompt_layer pattern
2. **Separation of concerns**: Layer owns prompt logic, not rendering
3. **Dual mode support**: Simple (fixed `> `) and context-aware (`loop>`, `if>`, etc.)
4. **Integration with input_continuation.c**: Reuse proven parser
5. **Configuration support**: PS2, colors, ANSI sequences
6. **Performance**: Minimal overhead, caching where appropriate

---

## Header File Structure

```c
/*
 * Lusush Shell - Layered Display Architecture
 * Continuation Prompt Layer Header - Multiline Input Visual Feedback
 * 
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ============================================================================
 * 
 * CONTINUATION PROMPT LAYER SYSTEM
 * 
 * This header defines the API for the continuation prompt layer of the 
 * Lusush Display System. The continuation prompt layer provides visual
 * feedback for multiline input by displaying continuation prompts on
 * subsequent lines after the initial prompt.
 * 
 * Key Features:
 * - Dual mode: Simple (fixed PS2) or Context-aware (loop>, if>, etc.)
 * - Integration with shared continuation parser (input_continuation.c)
 * - Configuration support for PS2 customization
 * - ANSI escape sequence support for colored prompts
 * - Efficient prompt generation with caching
 * - Event-driven communication with display layers
 * 
 * Design Principles:
 * - Separation of concerns: Layer provides prompts, screen buffer renders
 * - No buffer pollution: Prompts never added to command buffer
 * - Reuses proven parser: input_continuation.c for context analysis
 * - Performance optimized: Minimal per-line overhead
 * - Memory safe: Proper resource management
 */

#ifndef CONTINUATION_PROMPT_LAYER_H
#define CONTINUATION_PROMPT_LAYER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "layer_events.h"
#include "input_continuation.h"  // Shared continuation parser

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS AND CONFIGURATION
// ============================================================================

#define CONTINUATION_PROMPT_LAYER_VERSION_MAJOR 1
#define CONTINUATION_PROMPT_LAYER_VERSION_MINOR 0
#define CONTINUATION_PROMPT_LAYER_VERSION_PATCH 0

// Prompt content limits
#define CONTINUATION_PROMPT_MAX_LENGTH 64      // Max characters in continuation prompt
#define CONTINUATION_PROMPT_DEFAULT_PS2 "> "   // Default continuation prompt

// Context-aware prompt types (matching GNU Readline)
#define CONTINUATION_PROMPT_LOOP "loop> "      // for/while/until loops
#define CONTINUATION_PROMPT_IF "if> "          // if statements
#define CONTINUATION_PROMPT_CASE "case> "      // case statements
#define CONTINUATION_PROMPT_FUNCTION "function> "  // function definitions
#define CONTINUATION_PROMPT_QUOTE "quote> "    // Unclosed quotes

// Performance targets
#define CONTINUATION_PROMPT_TARGET_GENERATION_TIME_US 100  // 100 microseconds

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

/**
 * Error codes for continuation prompt layer operations
 */
typedef enum {
    CONTINUATION_PROMPT_SUCCESS = 0,
    CONTINUATION_PROMPT_ERROR_INVALID_PARAM,
    CONTINUATION_PROMPT_ERROR_NULL_POINTER,
    CONTINUATION_PROMPT_ERROR_MEMORY_ALLOCATION,
    CONTINUATION_PROMPT_ERROR_PARSER_FAILURE,
    CONTINUATION_PROMPT_ERROR_INVALID_MODE,
    CONTINUATION_PROMPT_ERROR_BUFFER_TOO_SMALL,
    CONTINUATION_PROMPT_ERROR_UNKNOWN
} continuation_prompt_error_t;

/**
 * Continuation prompt modes
 */
typedef enum {
    CONTINUATION_PROMPT_MODE_SIMPLE,       // Fixed PS2 prompt (ZSH style)
    CONTINUATION_PROMPT_MODE_CONTEXT_AWARE // Dynamic prompts (GNU Readline style)
} continuation_prompt_mode_t;

/**
 * Performance metrics for continuation prompt generation
 */
typedef struct {
    uint64_t prompt_generation_count;      // Total prompts generated
    uint64_t parser_invocations;           // Times parser was called
    uint64_t cache_hits;                   // Cached prompt reuses
    uint64_t total_generation_time_us;     // Total time in microseconds
    uint64_t max_generation_time_us;       // Max generation time
    uint64_t avg_generation_time_us;       // Average generation time
} continuation_prompt_performance_t;

/**
 * Continuation prompt configuration
 */
typedef struct {
    char *ps2_prompt;                      // Custom PS2 prompt string
    bool strip_ansi_for_length;            // Strip ANSI when calculating length
    bool use_colors;                       // Enable ANSI color sequences
    uint32_t default_color;                // Default prompt color (RGB)
    uint32_t context_colors[5];            // Colors for each context type
} continuation_prompt_config_t;

/**
 * Main continuation prompt layer structure
 */
typedef struct {
    // Mode configuration
    continuation_prompt_mode_t mode;       // Simple or context-aware
    continuation_prompt_config_t config;   // Prompt configuration
    
    // Parser integration
    continuation_state_t *parser_state;    // Shared continuation parser state
    bool parser_state_owned;               // Whether we own parser state
    
    // Cached prompts for performance
    char *cached_simple_prompt;            // Cached simple mode prompt
    char *cached_context_prompts[5];       // Cached context-aware prompts
    size_t cached_simple_length;           // Cached simple prompt length
    size_t cached_context_lengths[5];      // Cached context prompt lengths
    
    // Performance tracking
    continuation_prompt_performance_t performance;
    
    // Event communication
    layer_event_system_t *events;          // Event system reference
    bool events_initialized;               // Event system ready
    
    // Layer state
    bool initialized;                      // Layer initialization complete
    bool enabled;                          // Layer enabled/disabled
    uint64_t creation_time_ns;             // Creation timestamp
    uint64_t last_update_time_ns;          // Last update timestamp
    
    // Memory management
    uint32_t magic_header;                 // Memory corruption detection
    uint32_t magic_footer;
} continuation_prompt_layer_t;

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

/**
 * Create a new continuation prompt layer instance
 * 
 * Allocates and initializes a continuation_prompt_layer_t structure with
 * default values. Layer starts in SIMPLE mode with default PS2 ("> ").
 * 
 * @return Pointer to new layer instance, or NULL on allocation failure
 * 
 * @note Must call continuation_prompt_layer_init() to complete initialization
 * @note Must free with continuation_prompt_layer_destroy() when done
 */
continuation_prompt_layer_t *continuation_prompt_layer_create(void);

/**
 * Initialize continuation prompt layer
 * 
 * Completes initialization including:
 * - Event system integration
 * - Parser state initialization
 * - Prompt cache preparation
 * - Default configuration setup
 * 
 * @param layer Pointer to layer instance
 * @param events Pointer to layer events system (may be NULL)
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 * 
 * @note Layer must be created before calling this
 * @note Automatically enables layer on success
 */
continuation_prompt_error_t continuation_prompt_layer_init(
    continuation_prompt_layer_t *layer,
    layer_event_system_t *events
);

/**
 * Destroy continuation prompt layer and free resources
 * 
 * Frees all allocated memory including:
 * - Cached prompts
 * - Parser state (if owned)
 * - Configuration strings
 * - Layer structure itself
 * 
 * @param layer Pointer to layer instance
 * 
 * @note Safe to call with NULL pointer
 * @note Layer pointer invalid after this call
 */
void continuation_prompt_layer_destroy(continuation_prompt_layer_t *layer);

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

/**
 * Set continuation prompt mode
 * 
 * Changes between simple (fixed PS2) and context-aware (dynamic) modes.
 * 
 * @param layer Pointer to layer instance
 * @param mode Desired prompt mode
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 * 
 * @note Mode change invalidates prompt cache
 * @note Does not require reinitialization
 */
continuation_prompt_error_t continuation_prompt_layer_set_mode(
    continuation_prompt_layer_t *layer,
    continuation_prompt_mode_t mode
);

/**
 * Set custom PS2 prompt string
 * 
 * Updates the continuation prompt string used in simple mode.
 * May contain ANSI escape sequences for colors.
 * 
 * @param layer Pointer to layer instance
 * @param ps2 Custom PS2 string (copied internally)
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 * 
 * @note Invalidates simple prompt cache
 * @note Supports ANSI color sequences
 * @note Pass NULL to reset to default ("> ")
 */
continuation_prompt_error_t continuation_prompt_layer_set_ps2(
    continuation_prompt_layer_t *layer,
    const char *ps2
);

/**
 * Enable or disable the layer
 * 
 * @param layer Pointer to layer instance
 * @param enabled true to enable, false to disable
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 * 
 * @note Disabled layer returns empty prompts
 * @note Does not free resources (can be re-enabled)
 */
continuation_prompt_error_t continuation_prompt_layer_set_enabled(
    continuation_prompt_layer_t *layer,
    bool enabled
);

// ============================================================================
// PROMPT GENERATION FUNCTIONS
// ============================================================================

/**
 * Get continuation prompt for a specific line
 * 
 * Returns the appropriate continuation prompt based on current mode
 * and command context. This is the primary function used by the
 * composition engine.
 * 
 * @param layer Pointer to layer instance
 * @param line_number Line number (0 = first line after initial prompt)
 * @param command_content Full command buffer content (for context analysis)
 * @param out_prompt Output buffer for prompt string
 * @param out_prompt_size Size of output buffer
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 * 
 * @note Line 0 typically uses prompt_layer, not continuation prompt
 * @note In simple mode, ignores command_content
 * @note In context-aware mode, analyzes command_content up to line_number
 * @note out_prompt may contain ANSI escape sequences
 * 
 * Example usage:
 *   char prompt[64];
 *   continuation_prompt_layer_get_prompt_for_line(
 *       layer, 
 *       1,  // Second line
 *       "for i in 1 2 3; do\n",
 *       prompt,
 *       sizeof(prompt)
 *   );
 *   // prompt = "loop> " in context-aware mode
 *   // prompt = "> " in simple mode
 */
continuation_prompt_error_t continuation_prompt_layer_get_prompt_for_line(
    continuation_prompt_layer_t *layer,
    size_t line_number,
    const char *command_content,
    char *out_prompt,
    size_t out_prompt_size
);

/**
 * Get continuation prompt length (excluding ANSI codes)
 * 
 * Returns the visual length of the continuation prompt, excluding
 * ANSI escape sequences. Used for cursor position translation.
 * 
 * @param layer Pointer to layer instance
 * @param line_number Line number
 * @param command_content Full command buffer content
 * @return Visual length in characters, or 0 on error
 * 
 * @note Strips ANSI codes when calculating length
 * @note Returns 0 if layer is disabled
 */
size_t continuation_prompt_layer_get_prompt_length(
    continuation_prompt_layer_t *layer,
    size_t line_number,
    const char *command_content
);

/**
 * Batch get prompts for all lines
 * 
 * Gets continuation prompts for all lines in a multiline command.
 * More efficient than calling get_prompt_for_line() repeatedly.
 * 
 * @param layer Pointer to layer instance
 * @param command_content Full command buffer content
 * @param line_count Number of lines in command
 * @param out_prompts Array of prompt strings (caller allocates)
 * @param out_prompts_size Size of each prompt string buffer
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 * 
 * @note out_prompts[0] should be set to NULL (use prompt_layer instead)
 * @note Caller must allocate out_prompts array: char[line_count][size]
 * 
 * Example usage:
 *   char prompts[3][64];
 *   continuation_prompt_layer_get_prompts_for_lines(
 *       layer,
 *       "for i in 1; do\necho $i\ndone",
 *       3,
 *       prompts,
 *       64
 *   );
 *   // prompts[0] = NULL (not used)
 *   // prompts[1] = "loop> "
 *   // prompts[2] = "loop> "
 */
continuation_prompt_error_t continuation_prompt_layer_get_prompts_for_lines(
    continuation_prompt_layer_t *layer,
    const char *command_content,
    size_t line_count,
    char (*out_prompts)[CONTINUATION_PROMPT_MAX_LENGTH],
    size_t out_prompts_size
);

// ============================================================================
// PARSER INTEGRATION FUNCTIONS
// ============================================================================

/**
 * Update parser state with current command content
 * 
 * Analyzes command content and updates internal parser state.
 * Used in context-aware mode to determine appropriate prompts.
 * 
 * @param layer Pointer to layer instance
 * @param command_content Full command buffer content
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 * 
 * @note Only needed in context-aware mode
 * @note Automatically called by get_prompt_for_line()
 * @note Can be called preemptively for performance
 */
continuation_prompt_error_t continuation_prompt_layer_update_state(
    continuation_prompt_layer_t *layer,
    const char *command_content
);

/**
 * Get current parser state (for debugging/testing)
 * 
 * Returns pointer to internal parser state for inspection.
 * 
 * @param layer Pointer to layer instance
 * @return Pointer to parser state, or NULL if not available
 * 
 * @note Read-only access - do not modify
 * @note Pointer invalid after layer destruction
 */
const continuation_state_t *continuation_prompt_layer_get_parser_state(
    const continuation_prompt_layer_t *layer
);

// ============================================================================
// PERFORMANCE AND DIAGNOSTICS
// ============================================================================

/**
 * Get performance metrics
 * 
 * Returns performance statistics for prompt generation.
 * 
 * @param layer Pointer to layer instance
 * @param out_perf Output buffer for performance metrics
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 */
continuation_prompt_error_t continuation_prompt_layer_get_performance(
    const continuation_prompt_layer_t *layer,
    continuation_prompt_performance_t *out_perf
);

/**
 * Reset performance metrics
 * 
 * Clears all performance counters.
 * 
 * @param layer Pointer to layer instance
 * @return CONTINUATION_PROMPT_SUCCESS or error code
 */
continuation_prompt_error_t continuation_prompt_layer_reset_performance(
    continuation_prompt_layer_t *layer
);

/**
 * Convert error code to string
 * 
 * @param error Error code
 * @return Human-readable error description
 */
const char *continuation_prompt_layer_error_string(continuation_prompt_error_t error);

#ifdef __cplusplus
}
#endif

#endif // CONTINUATION_PROMPT_LAYER_H
```

---

## Implementation Notes

### Mode Behavior

**Simple Mode** (`CONTINUATION_PROMPT_MODE_SIMPLE`):
- Returns fixed PS2 string for all continuation lines
- Default: `"> "`
- Configurable via `continuation_prompt_layer_set_ps2()`
- Ignores command content
- Fastest performance (cached)

**Context-Aware Mode** (`CONTINUATION_PROMPT_MODE_CONTEXT_AWARE`):
- Analyzes command content using `input_continuation.c` parser
- Returns dynamic prompts based on construct type:
  - `loop>` for for/while/until loops
  - `if>` for if statements
  - `case>` for case statements
  - `function>` for function definitions
  - `quote>` for unclosed quotes
  - `> ` as fallback
- Moderate performance (parser invocation, caching helps)

### Parser Integration

The layer uses the **existing, proven** `input_continuation.c` parser:

```c
// From input_continuation.h
continuation_state_t *parser_state;  // Owned by layer

// Update state
continuation_analyze_line(command_content, parser_state);

// Get appropriate prompt
const char *prompt = continuation_get_prompt(parser_state);
```

**Benefits**:
- Reuses thoroughly tested code
- Consistent with GNU Readline behavior
- No code duplication
- Proven shell construct detection

### Caching Strategy

**Simple Mode Cache**:
- Single cached prompt string
- Invalidated only on PS2 change

**Context-Aware Mode Cache**:
- One cached prompt per context type (5 total)
- Invalidated on configuration change
- Parser state not cached (must analyze fresh content)

### Performance Targets

- **Target**: < 100 microseconds per prompt generation
- **Simple mode**: ~1-5 microseconds (cache hit)
- **Context-aware mode**: ~50-100 microseconds (parser + cache)

---

## Usage Examples

### Example 1: Simple Mode (Default)

```c
// Create and initialize
continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
continuation_prompt_layer_init(layer, events);

// Default mode is SIMPLE with PS2 = "> "
char prompt[64];
continuation_prompt_layer_get_prompt_for_line(
    layer,
    1,  // Line 1 (second line)
    "for i in 1 2 3; do\n",
    prompt,
    sizeof(prompt)
);
// Result: prompt = "> "

continuation_prompt_layer_destroy(layer);
```

### Example 2: Context-Aware Mode

```c
// Create and initialize
continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
continuation_prompt_layer_init(layer, events);

// Switch to context-aware mode
continuation_prompt_layer_set_mode(
    layer,
    CONTINUATION_PROMPT_MODE_CONTEXT_AWARE
);

// Get prompt for different contexts
char prompt[64];

// For loop
continuation_prompt_layer_get_prompt_for_line(
    layer,
    1,
    "for i in 1 2 3; do\n",
    prompt,
    sizeof(prompt)
);
// Result: prompt = "loop> "

// If statement
continuation_prompt_layer_get_prompt_for_line(
    layer,
    1,
    "if [ -f file ]; then\n",
    prompt,
    sizeof(prompt)
);
// Result: prompt = "if> "

continuation_prompt_layer_destroy(layer);
```

### Example 3: Custom PS2 with Colors

```c
// Create and initialize
continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
continuation_prompt_layer_init(layer, events);

// Set custom PS2 with ANSI color (green)
continuation_prompt_layer_set_ps2(
    layer,
    "\033[1;32m> \033[0m"  // Bold green "> " with reset
);

char prompt[64];
continuation_prompt_layer_get_prompt_for_line(
    layer,
    1,
    "echo line1\necho line2",
    prompt,
    sizeof(prompt)
);
// Result: prompt = "\033[1;32m> \033[0m"

// Get visual length (ANSI stripped)
size_t len = continuation_prompt_layer_get_prompt_length(
    layer, 1, "echo line1\n"
);
// Result: len = 2 ("> " without ANSI codes)

continuation_prompt_layer_destroy(layer);
```

### Example 4: Batch Prompt Generation

```c
continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
continuation_prompt_layer_init(layer, events);
continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);

const char *command = "for i in 1 2 3; do\necho $i\ndone\n";
char prompts[4][CONTINUATION_PROMPT_MAX_LENGTH];

continuation_prompt_layer_get_prompts_for_lines(
    layer,
    command,
    4,  // 4 lines
    prompts,
    CONTINUATION_PROMPT_MAX_LENGTH
);

// Result:
// prompts[0] = NULL (use prompt_layer)
// prompts[1] = "loop> "
// prompts[2] = "loop> "
// prompts[3] = "loop> " (or empty if loop complete)

continuation_prompt_layer_destroy(layer);
```

---

## Integration Points

### With Composition Engine

```c
// Compositor creates continuation layer
compositor->continuation_layer = continuation_prompt_layer_create();
continuation_prompt_layer_init(compositor->continuation_layer, events);

// During composition
for (size_t i = 1; i < line_count; i++) {
    char prompt[64];
    continuation_prompt_layer_get_prompt_for_line(
        compositor->continuation_layer,
        i,
        command_content,
        prompt,
        sizeof(prompt)
    );
    
    // Pass to screen buffer
    screen_buffer_set_line_with_prefix(
        screen_buffer,
        i,
        prompt,  // prefix
        lines[i] // content
    );
}
```

### With Screen Buffer

```c
// Screen buffer receives prompts as prefixes
screen_buffer_set_line_with_prefix(
    sb,
    line_num,
    continuation_prompt,  // From continuation layer
    command_line_content  // From command layer
);
```

### With Configuration System

```c
// Read PS2 from config or environment
const char *ps2 = getenv("PS2");
if (!ps2) {
    ps2 = symtable_get_global_default("PS2", "> ");
}

continuation_prompt_layer_set_ps2(layer, ps2);

// Read mode from config
if (config.lle_continuation_prompt_mode == MODE_CONTEXT_AWARE) {
    continuation_prompt_layer_set_mode(
        layer,
        CONTINUATION_PROMPT_MODE_CONTEXT_AWARE
    );
}
```

---

## Testing Strategy

### Unit Tests

1. **Lifecycle Tests**:
   - Create/init/destroy
   - Double initialization
   - Destroy NULL/uninitialized

2. **Simple Mode Tests**:
   - Default PS2 ("> ")
   - Custom PS2
   - PS2 with ANSI colors
   - Prompt length calculation

3. **Context-Aware Mode Tests**:
   - For loops → "loop> "
   - If statements → "if> "
   - Case statements → "case> "
   - Function definitions → "function> "
   - Unclosed quotes → "quote> "
   - Mixed constructs
   - Nested constructs

4. **Edge Cases**:
   - Empty command content
   - Single line command
   - Very long commands
   - Invalid line numbers
   - NULL parameters

5. **Performance Tests**:
   - Simple mode < 5 microseconds
   - Context-aware < 100 microseconds
   - Cache effectiveness

### Integration Tests

1. With composition engine
2. With screen buffer
3. With configuration system
4. Mode switching during operation
5. Multiple layers (ensure independence)

---

## Open Questions for Review

1. **Parser State Ownership**: Should layer create its own `continuation_state_t` or receive it?
   - Current design: Layer owns it
   - Alternative: Share with input.c parser

2. **Batch API Necessity**: Is `get_prompts_for_lines()` worth the complexity?
   - Pro: More efficient for multiline
   - Con: Adds API surface, composition can call get_prompt_for_line() in loop

3. **Event System Integration**: What events should layer emit/subscribe to?
   - Mode changes?
   - Configuration updates?
   - Performance warnings?

4. **ANSI Stripping**: Should layer provide utility to strip ANSI or rely on external?
   - Current: Internal utility function
   - Alternative: Use existing lusush ANSI utilities

---

## Next Steps

1. ✅ API design complete
2. ⏳ Review and approval
3. ⏳ Create header file stub
4. ⏳ Design screen_buffer enhancement (next document)

---

**Design Status**: Complete - Ready for implementation  
**Estimated Implementation Time**: 2 days  
**Dependencies**: input_continuation.h (existing), layer_events.h (existing)  
**Author**: AI Assistant with user collaboration
