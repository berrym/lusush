# Spec 02 Terminal Abstraction - Phase 1 Implementation Plan

**Phase**: 1 of 4 - Terminal Capability Detection  
**Status**: Planning  
**Estimated Lines**: ~400 lines  
**Estimated Time**: 2-3 days  

## Overview

Phase 1 implements terminal capability detection - the foundation for all terminal operations. This phase detects terminal type, features, and capabilities at initialization time through environment variables, terminfo queries, and terminal type matching.

**Critical Principle**: ONE-TIME DETECTION at startup. NO runtime terminal queries during operation.

## Objectives

1. ✅ Detect terminal type from environment
2. ✅ Query terminfo database for capabilities
3. ✅ Detect color support (8, 256, or 24-bit)
4. ✅ Detect advanced features (mouse, bracketed paste, focus events)
5. ✅ Detect terminal geometry
6. ✅ Cache all capabilities for runtime use
7. ✅ Provide query API for other components

## File to Create

**`src/lle/terminal_capabilities.c`** (~400 lines)

## Implementation Structure

### 1. Environment Variable Detection (~80 lines)

```c
/* Detect terminal type from TERM environment variable */
static lle_terminal_type_t detect_terminal_type(const char *term_env) {
    if (!term_env) return LLE_TERMINAL_GENERIC;
    
    /* Check for specific terminal types */
    if (strstr(term_env, "xterm")) return LLE_TERMINAL_XTERM;
    if (strstr(term_env, "rxvt")) return LLE_TERMINAL_RXVT;
    if (strstr(term_env, "konsole")) return LLE_TERMINAL_KONSOLE;
    if (strstr(term_env, "gnome")) return LLE_TERMINAL_GNOME_TERMINAL;
    if (strstr(term_env, "screen")) return LLE_TERMINAL_SCREEN;
    if (strstr(term_env, "tmux")) return LLE_TERMINAL_TMUX;
    if (strstr(term_env, "linux")) return LLE_TERMINAL_LINUX_CONSOLE;
    
    /* macOS */
    if (strstr(term_env, "nsterm")) return LLE_TERMINAL_DARWIN_TERMINAL;
    
    /* Modern terminals */
    if (strstr(term_env, "iterm")) return LLE_TERMINAL_ITERM2;
    if (strstr(term_env, "alacritty")) return LLE_TERMINAL_ALACRITTY;
    if (strstr(term_env, "kitty")) return LLE_TERMINAL_KITTY;
    
    return LLE_TERMINAL_GENERIC;
}

/* Detect if running in a TTY */
static bool detect_is_tty(void) {
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

/* Get terminal program name from environment */
static const char* detect_terminal_program(void) {
    /* Check common environment variables */
    const char *term_program = getenv("TERM_PROGRAM");
    if (term_program) return term_program;
    
    const char *colorterm = getenv("COLORTERM");
    if (colorterm) return colorterm;
    
    return "unknown";
}
```

### 2. Terminfo Capability Detection (~120 lines)

```c
#include <term.h>  /* ncurses terminfo */

/* Query terminfo for capability */
static bool query_terminfo_flag(const char *capability_name) {
    int result = tigetflag((char *)capability_name);
    return (result == 1);
}

/* Query terminfo for numeric capability */
static int query_terminfo_num(const char *capability_name) {
    int result = tigetnum((char *)capability_name);
    return (result >= 0) ? result : 0;
}

/* Detect color capabilities from terminfo */
static void detect_color_capabilities(lle_terminal_capabilities_t *caps) {
    /* Initialize terminfo */
    int err;
    if (setupterm(NULL, STDOUT_FILENO, &err) != OK) {
        /* Terminfo unavailable - use conservative defaults */
        caps->supports_ansi_colors = false;
        caps->supports_256_colors = false;
        caps->supports_truecolor = false;
        caps->detected_color_depth = 0;
        return;
    }
    
    /* Query color capability */
    int colors = query_terminfo_num("colors");
    
    if (colors >= 256) {
        caps->supports_ansi_colors = true;
        caps->supports_256_colors = true;
        caps->detected_color_depth = 8;
    } else if (colors >= 8) {
        caps->supports_ansi_colors = true;
        caps->supports_256_colors = false;
        caps->detected_color_depth = 4;
    } else {
        caps->supports_ansi_colors = false;
        caps->supports_256_colors = false;
        caps->detected_color_depth = 0;
    }
    
    /* Check for truecolor via environment (not in terminfo) */
    const char *colorterm = getenv("COLORTERM");
    if (colorterm && (strcmp(colorterm, "truecolor") == 0 || 
                      strcmp(colorterm, "24bit") == 0)) {
        caps->supports_truecolor = true;
        caps->detected_color_depth = 24;
    } else {
        caps->supports_truecolor = false;
    }
}

/* Detect text attributes from terminfo */
static void detect_text_attributes(lle_terminal_capabilities_t *caps) {
    caps->supports_bold = query_terminfo_flag("bold");
    caps->supports_italic = query_terminfo_flag("sitm");  /* enter italic */
    caps->supports_underline = query_terminfo_flag("smul");  /* enter underline */
    caps->supports_strikethrough = false;  /* Not in terminfo, check terminal type */
    caps->supports_reverse = query_terminfo_flag("rev");
    caps->supports_dim = query_terminfo_flag("dim");
    
    /* Some terminals support strikethrough */
    if (caps->terminal_type_enum == LLE_TERMINAL_XTERM ||
        caps->terminal_type_enum == LLE_TERMINAL_ALACRITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_KITTY) {
        caps->supports_strikethrough = true;
    }
}
```

### 3. Advanced Feature Detection (~80 lines)

```c
/* Detect advanced terminal features */
static void detect_advanced_features(lle_terminal_capabilities_t *caps) {
    /* Mouse reporting - most modern terminals */
    caps->supports_mouse_reporting = false;
    if (caps->terminal_type_enum == LLE_TERMINAL_XTERM ||
        caps->terminal_type_enum == LLE_TERMINAL_RXVT ||
        caps->terminal_type_enum == LLE_TERMINAL_GNOME_TERMINAL ||
        caps->terminal_type_enum == LLE_TERMINAL_ITERM2 ||
        caps->terminal_type_enum == LLE_TERMINAL_ALACRITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_KITTY) {
        caps->supports_mouse_reporting = true;
    }
    
    /* Bracketed paste mode */
    caps->supports_bracketed_paste = false;
    if (caps->terminal_type_enum == LLE_TERMINAL_XTERM ||
        caps->terminal_type_enum == LLE_TERMINAL_RXVT ||
        caps->terminal_type_enum == LLE_TERMINAL_GNOME_TERMINAL ||
        caps->terminal_type_enum == LLE_TERMINAL_ITERM2 ||
        caps->terminal_type_enum == LLE_TERMINAL_ALACRITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_KITTY) {
        caps->supports_bracketed_paste = true;
    }
    
    /* Focus events (FocusIn/FocusOut) */
    caps->supports_focus_events = false;
    if (caps->terminal_type_enum == LLE_TERMINAL_XTERM ||
        caps->terminal_type_enum == LLE_TERMINAL_ITERM2 ||
        caps->terminal_type_enum == LLE_TERMINAL_ALACRITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_KITTY) {
        caps->supports_focus_events = true;
    }
    
    /* Synchronized output (reduces flicker) */
    caps->supports_synchronized_output = false;
    if (caps->terminal_type_enum == LLE_TERMINAL_KITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_ALACRITTY) {
        caps->supports_synchronized_output = true;
    }
    
    /* Unicode support - assume yes for modern terminals */
    caps->supports_unicode = true;
    if (caps->terminal_type_enum == LLE_TERMINAL_LINUX_CONSOLE) {
        caps->supports_unicode = false;  /* Limited unicode */
    }
}
```

### 4. Geometry Detection (~40 lines)

```c
#include <sys/ioctl.h>

/* Detect terminal window size */
static void detect_terminal_geometry(lle_terminal_capabilities_t *caps) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        caps->terminal_width = ws.ws_col;
        caps->terminal_height = ws.ws_row;
    } else {
        /* Fallback to environment variables */
        const char *cols = getenv("COLUMNS");
        const char *lines = getenv("LINES");
        
        caps->terminal_width = cols ? atoi(cols) : 80;   /* Default 80 */
        caps->terminal_height = lines ? atoi(lines) : 24;  /* Default 24 */
    }
    
    /* Sanity checks */
    if (caps->terminal_width < 20) caps->terminal_width = 80;
    if (caps->terminal_height < 5) caps->terminal_height = 24;
}

/* Update geometry after resize (SIGWINCH) */
lle_result_t lle_capabilities_update_geometry(lle_terminal_capabilities_t *caps,
                                              size_t *width_out,
                                              size_t *height_out) {
    if (!caps) return LLE_ERROR_INVALID_PARAMETER;
    
    detect_terminal_geometry(caps);
    
    if (width_out) *width_out = caps->terminal_width;
    if (height_out) *height_out = caps->terminal_height;
    
    return LLE_SUCCESS;
}
```

### 5. Performance Characteristics (~30 lines)

```c
/* Estimate terminal latency based on type */
static void detect_performance_characteristics(lle_terminal_capabilities_t *caps) {
    /* Estimated round-trip latency in milliseconds */
    switch (caps->terminal_type_enum) {
        case LLE_TERMINAL_ALACRITTY:
        case LLE_TERMINAL_KITTY:
            caps->estimated_latency_ms = 5;    /* Very fast */
            caps->supports_fast_updates = true;
            break;
            
        case LLE_TERMINAL_ITERM2:
        case LLE_TERMINAL_GNOME_TERMINAL:
            caps->estimated_latency_ms = 10;   /* Fast */
            caps->supports_fast_updates = true;
            break;
            
        case LLE_TERMINAL_XTERM:
        case LLE_TERMINAL_RXVT:
            caps->estimated_latency_ms = 15;   /* Moderate */
            caps->supports_fast_updates = true;
            break;
            
        case LLE_TERMINAL_SCREEN:
        case LLE_TERMINAL_TMUX:
            caps->estimated_latency_ms = 20;   /* Slower (multiplexed) */
            caps->supports_fast_updates = false;
            break;
            
        case LLE_TERMINAL_LINUX_CONSOLE:
            caps->estimated_latency_ms = 30;   /* Slower */
            caps->supports_fast_updates = false;
            break;
            
        default:
            caps->estimated_latency_ms = 15;   /* Conservative default */
            caps->supports_fast_updates = true;
            break;
    }
}
```

### 6. Optimization Flags (~20 lines)

```c
/* Set optimization flags based on capabilities */
static void set_optimization_flags(lle_terminal_capabilities_t *caps) {
    caps->optimizations = LLE_OPT_NONE;
    
    /* Fast cursor positioning */
    if (caps->terminal_type_enum == LLE_TERMINAL_ALACRITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_KITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_ITERM2) {
        caps->optimizations |= LLE_OPT_FAST_CURSOR;
    }
    
    /* Batch updates for slow terminals */
    if (caps->terminal_type_enum == LLE_TERMINAL_SCREEN ||
        caps->terminal_type_enum == LLE_TERMINAL_TMUX) {
        caps->optimizations |= LLE_OPT_BATCH_UPDATES;
    }
    
    /* Incremental drawing for fast terminals */
    if (caps->supports_fast_updates) {
        caps->optimizations |= LLE_OPT_INCREMENTAL_DRAW;
    }
    
    /* Unicode awareness */
    if (caps->supports_unicode) {
        caps->optimizations |= LLE_OPT_UNICODE_AWARE;
    }
}
```

### 7. Main Detection Function (~50 lines)

```c
/* Main capability detection entry point */
lle_result_t lle_capabilities_detect_environment(lle_terminal_capabilities_t **caps,
                                                 lle_memory_pool_t *pool) {
    if (!caps || !pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate capabilities structure */
    lle_terminal_capabilities_t *c = lle_pool_alloc(sizeof(lle_terminal_capabilities_t));
    if (!c) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(c, 0, sizeof(lle_terminal_capabilities_t));
    
    /* Detect TTY status */
    c->is_tty = detect_is_tty();
    
    /* Get environment variables */
    const char *term_env = getenv("TERM");
    c->terminal_type = term_env ? strdup(term_env) : strdup("unknown");
    c->terminal_program = strdup(detect_terminal_program());
    
    /* Detect terminal type */
    c->terminal_type_enum = detect_terminal_type(term_env);
    
    /* Detect capabilities */
    detect_color_capabilities(c);
    detect_text_attributes(c);
    detect_advanced_features(c);
    detect_terminal_geometry(c);
    detect_performance_characteristics(c);
    set_optimization_flags(c);
    
    *caps = c;
    return LLE_SUCCESS;
}

/* Cleanup */
void lle_capabilities_destroy(lle_terminal_capabilities_t *caps) {
    if (!caps) return;
    
    if (caps->terminal_type) free((char *)caps->terminal_type);
    if (caps->terminal_program) free((char *)caps->terminal_program);
    
    lle_pool_free(caps);
}
```

## Testing Strategy

### Unit Tests (~200 lines in `tests/lle/unit/test_terminal_capability.c`)

**Test Categories:**

1. **Terminal Type Detection Tests** (6 tests)
   - Test TERM=xterm detection
   - Test TERM=rxvt detection
   - Test TERM=alacritty detection
   - Test TERM=kitty detection
   - Test TERM=screen detection
   - Test unknown terminal fallback

2. **Color Capability Tests** (4 tests)
   - Test 8-color terminal detection
   - Test 256-color terminal detection
   - Test truecolor detection (COLORTERM=truecolor)
   - Test no-color terminal

3. **Feature Detection Tests** (5 tests)
   - Test mouse support detection
   - Test bracketed paste detection
   - Test focus events detection
   - Test unicode support detection
   - Test performance characteristics

4. **Geometry Detection Tests** (3 tests)
   - Test ioctl window size query
   - Test environment variable fallback
   - Test geometry update

5. **TTY Detection Tests** (2 tests)
   - Test TTY detection
   - Test non-TTY handling

**Total: 20 unit tests**

## Dependencies

### Required Headers
```c
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <term.h>  /* ncurses terminfo */

#include "lle/terminal_abstraction.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
```

### External Libraries
- **ncurses/terminfo** - Already available in system
- **System calls** - ioctl, isatty (POSIX)

## Build Integration

Add to `src/lle/meson.build`:
```python
# Spec 02: Terminal Abstraction - Phase 1
if fs.exists('terminal_capabilities.c')
  lle_sources += files('terminal_capabilities.c')
endif
```

## Error Handling

All error paths properly handled:
- ✅ NULL pointer checks
- ✅ Memory allocation failures
- ✅ Terminfo unavailable (fallback to defaults)
- ✅ Invalid environment variables
- ✅ ioctl failures (use environment fallback)

## Performance Targets

- **Initialization**: < 1ms (one-time cost)
- **Geometry update**: < 100μs (ioctl only)
- **Memory footprint**: < 1KB per capabilities struct

## Zero-Tolerance Compliance

✅ **No TODOs** - Complete implementation  
✅ **No stubs** - All functions fully implemented  
✅ **Complete error handling** - All paths covered  
✅ **Memory safe** - No leaks, proper cleanup  
✅ **Thread safe** - Read-only after initialization  
✅ **Comprehensive tests** - 20 unit tests covering all paths  
✅ **Documentation** - Inline comments explaining logic  

## Success Criteria

Phase 1 complete when:
- ✅ `terminal_capabilities.c` implemented (400 lines)
- ✅ All 20 unit tests passing (100%)
- ✅ Build succeeds with no warnings
- ✅ Valgrind shows no leaks
- ✅ Capability detection works on multiple terminal types
- ✅ Performance targets met

## Next Phase

**Phase 2**: Terminal State Management (raw mode, signal handling)
