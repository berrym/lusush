# Phase 0: Validation Specification

**Document Status**: Living Document  
**Version**: 1.0.0  
**Last Updated**: 2025-10-14  
**Authority Level**: Operational (Implementation Tasks)

---

## Overview

This document provides detailed week-by-week, day-by-day implementation tasks for the Phase 0 Rapid Validation Prototype. Each task includes specific code examples, data structures, validation methods, and success criteria.

**Duration**: 4 weeks (28 days)  
**Objective**: Validate core architectural assumptions with quantitative criteria

---

## Week 1: Terminal State Abstraction

**Goal**: Prove "never query terminal" approach works across diverse terminal emulators

**Success Criteria**:
- ✅ Works on ≥5/7 terminals (≥70%)
- ✅ State updates <100μs
- ✅ Zero terminal queries in hot paths
- ✅ Capability detection <50ms

---

### Day 1: Foundation Setup

**Tasks**:
1. Create directory structure
2. Define core data structures
3. Implement basic terminal state tracking

**Code to Write**:

```c
// src/lle/validation/terminal/state.h

#ifndef LLE_VALIDATION_TERMINAL_STATE_H
#define LLE_VALIDATION_TERMINAL_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>

// Terminal state - internal source of truth
typedef struct {
    // Cursor position (0-based)
    uint16_t cursor_row;
    uint16_t cursor_col;
    
    // Screen dimensions
    uint16_t rows;
    uint16_t cols;
    
    // Scroll region
    uint16_t scroll_top;
    uint16_t scroll_bottom;
    
    // Terminal mode flags
    bool application_keypad;
    bool application_cursor;
    bool auto_wrap;
    bool origin_mode;
    
    // Validation tracking
    uint64_t last_update_ns;  // Nanosecond timestamp
    uint32_t update_count;
} lle_terminal_state_t;

// Terminal capabilities (detected once at init)
typedef struct {
    bool has_color;
    bool has_256_color;
    bool has_true_color;
    bool has_unicode;
    bool has_mouse;
    bool has_bracketed_paste;
    
    // Terminal type
    enum {
        TERM_VT100,
        TERM_XTERM,
        TERM_XTERM_256,
        TERM_RXVT,
        TERM_SCREEN,
        TERM_TMUX,
        TERM_UNKNOWN
    } type;
    
    // Detection metadata
    uint32_t detection_time_ms;
    bool detection_successful;
} lle_terminal_caps_t;

// Terminal validation context
typedef struct {
    // Original terminal settings (restore on exit)
    struct termios original_termios;
    
    // Raw mode settings
    struct termios raw_termios;
    
    // Internal state (authority)
    lle_terminal_state_t state;
    
    // Capabilities (detected once)
    lle_terminal_caps_t caps;
    
    // File descriptors
    int input_fd;   // stdin
    int output_fd;  // stdout
    
    // Validation metrics
    uint64_t total_updates;
    uint64_t total_update_time_ns;
} lle_terminal_validation_t;

// Initialize terminal validation
int lle_terminal_validation_init(lle_terminal_validation_t *term);

// Update internal state (never query terminal)
void lle_terminal_state_update_cursor(lle_terminal_validation_t *term,
                                       uint16_t row, uint16_t col);

// One-time capability detection (at initialization)
int lle_terminal_detect_capabilities(lle_terminal_validation_t *term,
                                      uint32_t timeout_ms);

// Cleanup
void lle_terminal_validation_cleanup(lle_terminal_validation_t *term);

#endif // LLE_VALIDATION_TERMINAL_STATE_H
```

**Validation**:
- Code compiles without errors
- Header guards correct
- Data structures aligned (check with pahole if available)

---

### Day 2: Capability Detection

**Tasks**:
1. Implement one-time capability detection
2. Add timeout mechanism
3. Parse terminal responses safely

**Code to Write**:

```c
// src/lle/validation/terminal/capability.c

#include "state.h"
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// Query terminal with timeout
static int query_terminal(int fd, const char *query, char *response,
                          size_t max_len, uint32_t timeout_ms) {
    // Write query
    ssize_t written = write(fd, query, strlen(query));
    if (written != (ssize_t)strlen(query)) {
        return -1;
    }
    
    // Wait for response with timeout
    fd_set readfds;
    struct timeval timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000
    };
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
    if (ret <= 0) {
        return -1;  // Timeout or error
    }
    
    // Read response
    ssize_t n = read(fd, response, max_len - 1);
    if (n > 0) {
        response[n] = '\0';
        return n;
    }
    
    return -1;
}

int lle_terminal_detect_capabilities(lle_terminal_validation_t *term,
                                      uint32_t timeout_ms) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    char response[256];
    
    // Detect color support (query with DA1 - Device Attributes)
    if (query_terminal(term->output_fd, "\x1b[c", response,
                       sizeof(response), timeout_ms) > 0) {
        // Parse response for capabilities
        term->caps.has_color = (strstr(response, "4;") != NULL);
        term->caps.detection_successful = true;
    }
    
    // Detect 256 color (check TERM environment)
    const char *term_env = getenv("TERM");
    if (term_env) {
        term->caps.has_256_color = (strstr(term_env, "256color") != NULL);
        term->caps.has_true_color = (strstr(term_env, "truecolor") != NULL);
        
        // Detect terminal type
        if (strstr(term_env, "xterm")) {
            term->caps.type = TERM_XTERM;
            if (strstr(term_env, "256")) {
                term->caps.type = TERM_XTERM_256;
            }
        } else if (strstr(term_env, "rxvt")) {
            term->caps.type = TERM_RXVT;
        } else if (strstr(term_env, "screen")) {
            term->caps.type = TERM_SCREEN;
        } else if (strstr(term_env, "tmux")) {
            term->caps.type = TERM_TMUX;
        } else if (strstr(term_env, "vt100")) {
            term->caps.type = TERM_VT100;
        } else {
            term->caps.type = TERM_UNKNOWN;
        }
    }
    
    // Detect Unicode support (check LANG/LC_ALL)
    const char *lang = getenv("LANG");
    if (lang && strstr(lang, "UTF-8")) {
        term->caps.has_unicode = true;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    term->caps.detection_time_ms = 
        (end.tv_sec - start.tv_sec) * 1000 +
        (end.tv_nsec - start.tv_nsec) / 1000000;
    
    return 0;
}
```

**Validation**:
- Capability detection completes in <50ms
- Works across different terminals
- Timeout mechanism functions correctly

---

### Day 3: State Management Implementation

**Tasks**:
1. Implement state update functions
2. Add performance tracking
3. Ensure no terminal queries in update paths

**Code to Write**:

```c
// src/lle/validation/terminal/state.c

#include "state.h"
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>

int lle_terminal_validation_init(lle_terminal_validation_t *term) {
    memset(term, 0, sizeof(*term));
    
    term->input_fd = STDIN_FILENO;
    term->output_fd = STDOUT_FILENO;
    
    // Save original terminal settings
    if (tcgetattr(term->input_fd, &term->original_termios) != 0) {
        return -1;
    }
    
    // Setup raw mode
    term->raw_termios = term->original_termios;
    term->raw_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    term->raw_termios.c_iflag &= ~(IXON | ICRNL);
    term->raw_termios.c_cc[VMIN] = 0;
    term->raw_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(term->input_fd, TCSANOW, &term->raw_termios) != 0) {
        return -1;
    }
    
    // Get initial window size (ONE-TIME query, not repeated)
    struct winsize ws;
    if (ioctl(term->output_fd, TIOCGWINSZ, &ws) == 0) {
        term->state.rows = ws.ws_row;
        term->state.cols = ws.ws_col;
    } else {
        // Fallback defaults
        term->state.rows = 24;
        term->state.cols = 80;
    }
    
    // Initialize scroll region to full screen
    term->state.scroll_top = 0;
    term->state.scroll_bottom = term->state.rows - 1;
    
    // Detect capabilities
    lle_terminal_detect_capabilities(term, 50);  // 50ms timeout
    
    return 0;
}

void lle_terminal_state_update_cursor(lle_terminal_validation_t *term,
                                       uint16_t row, uint16_t col) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Update internal state (NO TERMINAL QUERY)
    term->state.cursor_row = row;
    term->state.cursor_col = col;
    term->state.update_count++;
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Track performance
    uint64_t update_time_ns = 
        (end.tv_sec - start.tv_sec) * 1000000000ULL +
        (end.tv_nsec - start.tv_nsec);
    
    term->state.last_update_ns = update_time_ns;
    term->total_update_time_ns += update_time_ns;
    term->total_updates++;
}

void lle_terminal_validation_cleanup(lle_terminal_validation_t *term) {
    // Restore original terminal settings
    tcsetattr(term->input_fd, TCSANOW, &term->original_termios);
}
```

**Validation**:
- State updates measured <100μs
- No terminal queries in update functions (code review)
- Cleanup restores terminal correctly

---

### Day 4: Multi-Terminal Testing

**Tasks**:
1. Create terminal compatibility test
2. Test on all 7 target terminals
3. Document compatibility results

**Code to Write**:

```c
// src/lle/validation/terminal/test/terminal_compatibility_test.c

#include "../state.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Test terminal initialization
static int test_init(void) {
    lle_terminal_validation_t term;
    
    int ret = lle_terminal_validation_init(&term);
    if (ret != 0) {
        fprintf(stderr, "FAIL: Initialization failed\n");
        return -1;
    }
    
    // Verify state initialized
    assert(term.state.rows > 0);
    assert(term.state.cols > 0);
    assert(term.caps.detection_time_ms < 50);
    
    lle_terminal_validation_cleanup(&term);
    
    printf("PASS: Initialization\n");
    return 0;
}

// Test state updates
static int test_state_updates(void) {
    lle_terminal_validation_t term;
    lle_terminal_validation_init(&term);
    
    // Perform 10,000 state updates
    for (int i = 0; i < 10000; i++) {
        lle_terminal_state_update_cursor(&term, i % 24, i % 80);
    }
    
    // Verify performance
    uint64_t avg_time_ns = term.total_update_time_ns / term.total_updates;
    uint64_t avg_time_us = avg_time_ns / 1000;
    
    printf("Average update time: %lu μs\n", avg_time_us);
    
    if (avg_time_us >= 100) {
        fprintf(stderr, "FAIL: Update time %lu μs >= 100 μs\n", avg_time_us);
        lle_terminal_validation_cleanup(&term);
        return -1;
    }
    
    lle_terminal_validation_cleanup(&term);
    
    printf("PASS: State updates\n");
    return 0;
}

// Test capability detection
static int test_capabilities(void) {
    lle_terminal_validation_t term;
    lle_terminal_validation_init(&term);
    
    // Print detected capabilities
    printf("Terminal type: %d\n", term.caps.type);
    printf("Color: %d\n", term.caps.has_color);
    printf("256 color: %d\n", term.caps.has_256_color);
    printf("Unicode: %d\n", term.caps.has_unicode);
    printf("Detection time: %u ms\n", term.caps.detection_time_ms);
    
    lle_terminal_validation_cleanup(&term);
    
    printf("PASS: Capability detection\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("=== Terminal Compatibility Test ===\n");
    printf("Terminal: %s\n\n", getenv("TERM"));
    
    if (test_init() != 0) failures++;
    if (test_state_updates() != 0) failures++;
    if (test_capabilities() != 0) failures++;
    
    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("✅ ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("❌ %d TESTS FAILED\n", failures);
        return 1;
    }
}
```

**Test Terminals**:
1. xterm
2. gnome-terminal
3. konsole
4. alacritty
5. kitty
6. urxvt
7. st

**Validation Method**:
```bash
# Compile
gcc -o terminal_test src/lle/validation/terminal/test/terminal_compatibility_test.c \
    src/lle/validation/terminal/*.c

# Test in each terminal
for term in xterm gnome-terminal konsole alacritty kitty urxvt st; do
    echo "Testing in $term..."
    $term -e ./terminal_test
done
```

**Success Criteria**: ≥5/7 terminals pass all tests

---

### Day 5: Week 1 Validation & Documentation

**Tasks**:
1. Run comprehensive tests on all terminals
2. Measure and document performance
3. Update DAILY_LOG.md with results
4. Assess Week 1 success criteria

**Performance Benchmark**:

```c
// src/lle/validation/terminal/test/state_update_benchmark.c

#include "../state.h"
#include <stdio.h>
#include <time.h>

int main(void) {
    lle_terminal_validation_t term;
    lle_terminal_validation_init(&term);
    
    const int iterations = 100000;
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        lle_terminal_state_update_cursor(&term, i % 24, i % 80);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t total_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL +
                        (end.tv_nsec - start.tv_nsec);
    uint64_t avg_ns = total_ns / iterations;
    
    printf("=== State Update Benchmark ===\n");
    printf("Iterations: %d\n", iterations);
    printf("Total time: %.3f ms\n", total_ns / 1000000.0);
    printf("Average: %lu ns (%.3f μs)\n", avg_ns, avg_ns / 1000.0);
    
    // Calculate percentiles (simple approximation)
    uint64_t p50_us = term.total_update_time_ns / term.total_updates / 1000;
    printf("P50: ~%lu μs\n", p50_us);
    
    lle_terminal_validation_cleanup(&term);
    
    if (avg_ns / 1000 < 100) {
        printf("✅ PASS: < 100 μs\n");
        return 0;
    } else {
        printf("❌ FAIL: >= 100 μs\n");
        return 1;
    }
}
```

**Documentation Requirements**:
- Update `DAILY_LOG.md` with all Day 1-5 progress
- Document terminal compatibility matrix (which terminals work)
- Document performance results (p50, p99 latencies)
- Assess against Week 1 success criteria

**Week 1 Success Criteria Check**:
- [ ] Terminal compatibility: ≥70% (5/7 terminals)?
- [ ] State updates: <100μs?
- [ ] Terminal queries: 0 in hot paths?
- [ ] Capability detection: <50ms?

---

## Week 2: Display Layer Integration

**Goal**: Prove LLE can function as pure client of Lusush display system

**Success Criteria**:
- ✅ All 7 rendering scenarios work
- ✅ Zero direct terminal escape sequences
- ✅ Update latency <10ms for complex edits
- ✅ No visual artifacts

---

### Day 6: Display Client Interface

**Tasks**:
1. Review Lusush display system API
2. Implement LLE display client interface
3. Basic rendering test

**Code to Write**:

```c
// src/lle/validation/display/client.h

#ifndef LLE_VALIDATION_DISPLAY_CLIENT_H
#define LLE_VALIDATION_DISPLAY_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

// Display cell (character + attributes)
typedef struct {
    uint32_t codepoint;  // Unicode codepoint
    uint8_t fg_color;    // Foreground color
    uint8_t bg_color;    // Background color
    uint8_t attrs;       // Bold, underline, etc.
} lle_display_cell_t;

// Display buffer (screen representation)
typedef struct {
    lle_display_cell_t *cells;
    uint16_t rows;
    uint16_t cols;
    bool dirty;  // Needs redraw
} lle_display_buffer_t;

// Display client context
typedef struct {
    // Reference to Lusush display system
    void *display_system;  // Opaque pointer to display system
    
    // Display buffer
    lle_display_buffer_t buffer;
    
    // Cursor position
    uint16_t cursor_row;
    uint16_t cursor_col;
    
    // Performance tracking
    uint64_t render_count;
    uint64_t total_render_time_ns;
} lle_display_client_t;

// Initialize display client
int lle_display_client_init(lle_display_client_t *client, uint16_t rows, uint16_t cols);

// Render line editor content (ONLY through display system)
int lle_display_client_render(lle_display_client_t *client, const char *line);

// Cleanup
void lle_display_client_cleanup(lle_display_client_t *client);

#endif // LLE_VALIDATION_DISPLAY_CLIENT_H
```

**Note**: This is a simplified validation version. In Phase 1, this will integrate with the actual Lusush display system. For Phase 0, we create a minimal display client to validate the architectural approach.

---

### Day 7: Rendering Implementation

**Tasks**:
1. Implement basic rendering through display client
2. Add multi-line support
3. Test single-line and multi-line rendering

**Code to Write**:

```c
// src/lle/validation/display/rendering.c

#include "client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

int lle_display_client_init(lle_display_client_t *client, uint16_t rows, uint16_t cols) {
    memset(client, 0, sizeof(*client));
    
    client->buffer.rows = rows;
    client->buffer.cols = cols;
    client->buffer.cells = calloc(rows * cols, sizeof(lle_display_cell_t));
    
    if (!client->buffer.cells) {
        return -1;
    }
    
    return 0;
}

int lle_display_client_render(lle_display_client_t *client, const char *line) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Clear buffer
    memset(client->buffer.cells, 0, 
           client->buffer.rows * client->buffer.cols * sizeof(lle_display_cell_t));
    
    // Render line into buffer (simplified for validation)
    uint16_t row = 0, col = 0;
    for (const char *p = line; *p; p++) {
        if (*p == '\n') {
            row++;
            col = 0;
            if (row >= client->buffer.rows) break;
            continue;
        }
        
        if (col >= client->buffer.cols) {
            // Wrap to next line
            row++;
            col = 0;
            if (row >= client->buffer.rows) break;
        }
        
        lle_display_cell_t *cell = &client->buffer.cells[row * client->buffer.cols + col];
        cell->codepoint = (uint32_t)*p;
        cell->fg_color = 7;  // Default white
        cell->bg_color = 0;  // Default black
        cell->attrs = 0;
        
        col++;
    }
    
    client->cursor_row = row;
    client->cursor_col = col;
    client->buffer.dirty = true;
    
    // In real implementation, this would call Lusush display system
    // For validation, we just track that we would render
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t render_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL +
                               (end.tv_nsec - start.tv_nsec);
    
    client->total_render_time_ns += render_time_ns;
    client->render_count++;
    
    return 0;
}

void lle_display_client_cleanup(lle_display_client_t *client) {
    free(client->buffer.cells);
    memset(client, 0, sizeof(*client));
}
```

---

### Day 8: All Rendering Scenarios

**Tasks**:
1. Implement all 7 rendering scenarios
2. Create tests for each scenario
3. Verify zero escape sequences

**Test Scenarios**:

```c
// src/lle/validation/display/test/rendering_scenarios_test.c

#include "../client.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int test_single_line(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    const char *line = "echo hello world";
    lle_display_client_render(&client, line);
    
    // Verify rendered correctly
    assert(client.cursor_row == 0);
    assert(client.cursor_col == strlen(line));
    
    lle_display_client_cleanup(&client);
    printf("PASS: Single-line rendering\n");
    return 0;
}

static int test_multi_line(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    // 200 character line (wraps multiple times)
    char line[256];
    memset(line, 'a', 200);
    line[200] = '\0';
    
    lle_display_client_render(&client, line);
    
    // Should wrap to multiple lines
    assert(client.cursor_row >= 2);  // At least 3 lines
    
    lle_display_client_cleanup(&client);
    printf("PASS: Multi-line rendering\n");
    return 0;
}

static int test_prompt(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    // Simulate prompt + input
    const char *prompt = "user@host:~$ ";
    const char *input = "ls -la";
    
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", prompt, input);
    
    lle_display_client_render(&client, combined);
    
    assert(client.cursor_col == strlen(combined));
    
    lle_display_client_cleanup(&client);
    printf("PASS: Prompt rendering\n");
    return 0;
}

// TODO: Add tests for:
// - Syntax highlighting (Day 9)
// - Completion preview (Day 9)
// - Scroll region (Day 9)
// - Atomic updates (Day 10)

int main(void) {
    printf("=== Display Rendering Scenarios Test ===\n\n");
    
    int failures = 0;
    if (test_single_line() != 0) failures++;
    if (test_multi_line() != 0) failures++;
    if (test_prompt() != 0) failures++;
    
    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("✅ ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("❌ %d TESTS FAILED\n", failures);
        return 1;
    }
}
```

---

### Day 9: Advanced Rendering Features

**Tasks**:
1. Add syntax highlighting support
2. Add completion preview
3. Add scroll region management
4. Complete all 7 scenarios

---

### Day 10: Escape Sequence Detection & Performance

**Tasks**:
1. Create escape sequence detector
2. Run all rendering tests with detector
3. Performance benchmarking
4. Week 2 validation

**Escape Sequence Detector**:

```c
// src/lle/validation/display/test/escape_detector.c

#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Intercept write() calls to detect escape sequences
static int escape_count = 0;

ssize_t write(int fd, const void *buf, size_t count) {
    // Check for escape sequences
    const char *str = (const char *)buf;
    for (size_t i = 0; i < count; i++) {
        if (str[i] == '\x1b') {
            escape_count++;
            fprintf(stderr, "WARNING: Escape sequence detected at offset %zu\n", i);
        }
    }
    
    // Call real write (use syscall to avoid recursion)
    return syscall(SYS_write, fd, buf, count);
}

// Run tests and report
int main(void) {
    // Run all rendering tests
    // ...
    
    if (escape_count == 0) {
        printf("✅ PASS: No escape sequences detected\n");
        return 0;
    } else {
        printf("❌ FAIL: %d escape sequences detected\n", escape_count);
        return 1;
    }
}
```

**Success Criteria Check**:
- [ ] All 7 scenarios render correctly?
- [ ] Zero escape sequences detected?
- [ ] Update latency <10ms?
- [ ] No visual artifacts?

---

## Week 3: Performance & Memory

**Goal**: Validate <100μs latency and <1MB memory footprint

**Success Criteria**:
- ✅ Input latency: <50μs (p50), <100μs (p99)
- ✅ Memory footprint: <1MB incremental
- ✅ Allocation latency: <100μs
- ✅ Zero memory leaks

---

### Day 11-12: Comprehensive Performance Benchmarking

*[Detailed day-by-day tasks for Week 3 would continue here...]*

### Day 13-14: Memory Pool Integration

*[Details...]*

### Day 15: Week 3 Validation

*[Details...]*

---

## Week 4: Event-Driven Architecture

**Goal**: Validate event system safety and performance

**Success Criteria**:
- ✅ All 6 event scenarios work
- ✅ Zero race conditions
- ✅ Zero deadlocks
- ✅ Event latency <50μs

---

### Day 16-17: Event Queue Implementation

*[Detailed day-by-day tasks for Week 4 would continue here...]*

### Day 18-19: Signal Handling

*[Details...]*

### Day 20: Week 4 Gate Decision Preparation

*[Details...]*

---

## Summary

This specification provides the detailed implementation roadmap for Phase 0. Each week builds on the previous, culminating in the Week 4 gate decision.

**Note**: Due to document length, Weeks 3-4 detailed daily tasks are abbreviated. During implementation, expand these sections with equivalent detail to Weeks 1-2.

**Success**: Week 4 gate decision of PROCEED validates the LLE architecture and enables confident progression to Phase 1.

---

**END OF DOCUMENT**
