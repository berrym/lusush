# LLE Build Infrastructure

**Date**: 2025-10-19  
**Status**: Build Structure Definition  
**Purpose**: Define professional, logical, flat build structure for LLE implementation

---

## Design Principles

1. **Flat Module Structure**: Each specification = one module at same level
2. **No Deep Nesting**: Everything under `src/lle/` is foundational/important
3. **Clear Module Names**: Module name directly matches specification name
4. **Professional Organization**: Standard C project layout patterns
5. **Build as Static Library**: `liblle.a` linked into final lusush executable

---

## Directory Structure

### Source Layout

```
src/lle/
├── error_handling.c         # Spec 16: Error Handling
├── memory_management.c       # Spec 15: Memory Management  
├── performance.c             # Spec 14: Performance Optimization
├── testing.c                 # Spec 17: Testing Framework
├── terminal.c                # Spec 02: Terminal Abstraction
├── buffer.c                  # Spec 03: Buffer Management
├── event_system.c            # Spec 04: Event System
├── hashtable_integration.c   # Spec 05: libhashtable Integration
├── input_parsing.c           # Spec 06: Input Parsing
├── extensibility.c           # Spec 07: Extensibility Framework
├── display.c                 # Spec 08: Display Integration
├── history.c                 # Spec 09: History System
├── autosuggestions.c         # Spec 10: Autosuggestions
├── syntax_highlighting.c     # Spec 11: Syntax Highlighting
├── completion.c              # Spec 12: Completion System
├── customization.c           # Spec 13: User Customization
├── plugin_api.c              # Spec 18: Plugin API
├── security.c                # Spec 19: Security Analysis
└── meson.build               # LLE build configuration
```

### Header Layout

```
include/lle/
├── error_handling.h          # Spec 16: Public error handling API
├── memory_management.h        # Spec 15: Public memory API
├── performance.h              # Spec 14: Public performance API
├── testing.h                  # Spec 17: Public testing API
├── terminal.h                 # Spec 02: Public terminal API
├── buffer.h                   # Spec 03: Public buffer API
├── event_system.h             # Spec 04: Public event API
├── hashtable_integration.h    # Spec 05: Public hashtable API
├── input_parsing.h            # Spec 06: Public input API
├── extensibility.h            # Spec 07: Public extensibility API
├── display.h                  # Spec 08: Public display API
├── history.h                  # Spec 09: Public history API
├── autosuggestions.h          # Spec 10: Public autosuggestions API
├── syntax_highlighting.h      # Spec 11: Public syntax API
├── completion.h               # Spec 12: Public completion API
├── customization.h            # Spec 13: Public customization API
├── plugin_api.h               # Spec 18: Public plugin API
├── security.h                 # Spec 19: Public security API
└── lle.h                      # Master header (includes all public APIs)
```

### Internal Headers (Optional)

If a module needs internal/private headers not exposed to other modules:

```
src/lle/
├── error_handling.c
├── error_handling_internal.h    # Private implementation details
├── buffer.c
├── buffer_internal.h             # Private implementation details
└── ...
```

### Test Layout

```
tests/lle/
├── test_error_handling.c      # Spec 16 tests
├── test_memory_management.c   # Spec 15 tests
├── test_performance.c         # Spec 14 tests
├── test_terminal.c            # Spec 02 tests
├── test_buffer.c              # Spec 03 tests
├── test_event_system.c        # Spec 04 tests
├── test_input_parsing.c       # Spec 06 tests
├── test_display.c             # Spec 08 tests
├── test_history.c             # Spec 09 tests
├── test_autosuggestions.c     # Spec 10 tests
├── test_syntax_highlighting.c # Spec 11 tests
├── test_completion.c          # Spec 12 tests
├── test_integration.c         # Cross-module integration tests
└── meson.build                # Test build configuration
```

---

## Build System Integration

### Static Library Approach

LLE will be built as a **static library** (`liblle.a`) and linked into the final `lusush` executable.

**Rationale**:
1. Clean separation between LLE and rest of Lusush
2. Can be tested independently
3. Clear API boundary via public headers
4. Easier to track LLE-specific compilation
5. Could potentially be reused in other projects (though Lusush-specific)

### Meson Build Structure

#### Root `meson.build` Changes

```meson
# In root meson.build, after readline dependency:

# Build LLE as static library
lle_lib = static_library('lle',
                         sources: lle_sources,
                         include_directories: inc,
                         dependencies: [readline_dep])

# Create LLE dependency for linking
lle_dep = declare_dependency(link_with: lle_lib,
                             include_directories: include_directories('include/lle'))

# Update executable to link LLE
executable('lusush',
           src,
           include_directories: inc,
           dependencies: [readline_dep, lle_dep])
```

#### LLE-specific `src/lle/meson.build`

This file will be created and will define `lle_sources`:

```meson
# src/lle/meson.build

# LLE source files - add incrementally as specs are implemented
lle_sources = []

# Phase 0: Foundation (implement first)
if fs.exists('src/lle/error_handling.c')
  lle_sources += 'error_handling.c'
endif

if fs.exists('src/lle/memory_management.c')
  lle_sources += 'memory_management.c'
endif

if fs.exists('src/lle/performance.c')
  lle_sources += 'performance.c'
endif

if fs.exists('src/lle/testing.c')
  lle_sources += 'testing.c'
endif

# Phase 1: Core Systems (implement after Phase 0)
if fs.exists('src/lle/terminal.c')
  lle_sources += 'terminal.c'
endif

if fs.exists('src/lle/event_system.c')
  lle_sources += 'event_system.c'
endif

if fs.exists('src/lle/hashtable_integration.c')
  lle_sources += 'hashtable_integration.c'
endif

# Phase 2: Buffer and Display (implement after Phase 1)
if fs.exists('src/lle/buffer.c')
  lle_sources += 'buffer.c'
endif

if fs.exists('src/lle/display.c')
  lle_sources += 'display.c'
endif

# Phase 3: Input and Integration (implement after Phase 2)
if fs.exists('src/lle/input_parsing.c')
  lle_sources += 'input_parsing.c'
endif

if fs.exists('src/lle/extensibility.c')
  lle_sources += 'extensibility.c'
endif

# Phase 4: Features (implement after Phase 3)
if fs.exists('src/lle/history.c')
  lle_sources += 'history.c'
endif

if fs.exists('src/lle/syntax_highlighting.c')
  lle_sources += 'syntax_highlighting.c'
endif

if fs.exists('src/lle/completion.c')
  lle_sources += 'completion.c'
endif

if fs.exists('src/lle/autosuggestions.c')
  lle_sources += 'autosuggestions.c'
endif

if fs.exists('src/lle/customization.c')
  lle_sources += 'customization.c'
endif

# Phase 5: Infrastructure (implement after Phase 4)
if fs.exists('src/lle/plugin_api.c')
  lle_sources += 'plugin_api.c'
endif

if fs.exists('src/lle/security.c')
  lle_sources += 'security.c'
endif

# Export lle_sources to parent
```

#### Root `meson.build` Integration

```meson
# In root meson.build, before executable() call:

# Include LLE build system
subdir('src/lle')

# Only build LLE library if sources exist
if lle_sources.length() > 0
  lle_lib = static_library('lle',
                           sources: lle_sources,
                           include_directories: inc,
                           dependencies: [readline_dep])
  
  lle_dep = declare_dependency(link_with: lle_lib,
                               include_directories: include_directories('include/lle'))
else
  # LLE not yet implemented, create empty dependency
  lle_dep = dependency('', required: false)
endif

# Build lusush executable (with or without LLE)
executable('lusush',
           src,
           include_directories: inc,
           dependencies: [readline_dep, lle_dep])
```

---

## File Naming Conventions

### Source Files

- **One spec = one `.c` file** (unless spec is extremely large >5000 LOC)
- Name matches specification concept directly
- Use underscores for multi-word names
- Examples:
  - `error_handling.c` (Spec 16)
  - `memory_management.c` (Spec 15)
  - `syntax_highlighting.c` (Spec 11)

### Header Files

- **Public API**: `include/lle/<module>.h`
  - Contains ONLY public API visible to Lusush and other LLE modules
  - Matches source file name
  
- **Internal API**: `src/lle/<module>_internal.h` (if needed)
  - Contains private implementation details
  - Only included by the corresponding `.c` file

### Test Files

- **Pattern**: `test_<module>.c`
- Matches source module being tested
- Examples:
  - `test_error_handling.c` tests `error_handling.c`
  - `test_buffer.c` tests `buffer.c`

---

## Adding New Modules

### Procedure for Implementing a New Spec

When implementing a new specification (e.g., Spec 16: Error Handling):

1. **Create header file**:
   ```bash
   # Create public header
   touch include/lle/error_handling.h
   ```

2. **Implement header**:
   - Copy EXACT type definitions from specification
   - Copy EXACT function signatures from specification
   - Add header guards, includes, documentation

3. **Create source file**:
   ```bash
   # Create implementation
   touch src/lle/error_handling.c
   ```

4. **Implement source**:
   - Include public header: `#include "lle/error_handling.h"`
   - Implement COMPLETE algorithms from specification
   - NO stubs, NO TODOs, NO placeholders

5. **Create test file**:
   ```bash
   # Create test
   touch tests/lle/test_error_handling.c
   ```

6. **Implement tests**:
   - Comprehensive test coverage
   - All functions tested
   - Edge cases covered
   - Performance validated

7. **Update build system**:
   - `src/lle/meson.build` automatically includes the new file (via `fs.exists()`)
   - No manual build file changes needed!

8. **Compile and test**:
   ```bash
   meson compile -C build
   ./build/tests/lle/test_error_handling
   ```

9. **Update living documents**:
   - AI_ASSISTANT_HANDOFF_DOCUMENT.md
   - SPEC_IMPLEMENTATION_ORDER.md (mark spec complete)
   - LLE_IMPLEMENTATION_GUIDE.md (if procedures changed)

---

## Master Header (`lle.h`)

The master header `include/lle/lle.h` provides a single include for all LLE functionality:

```c
/**
 * @file lle.h
 * @brief Lusush Line Editor - Master Public API Header
 * 
 * This is the single entry point for all LLE functionality.
 * Include this header to access the complete LLE API.
 * 
 * Individual module headers can also be included separately if needed.
 */

#ifndef LLE_H
#define LLE_H

/* Phase 0: Foundation */
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/performance.h"
#include "lle/testing.h"

/* Phase 1: Core Systems */
#include "lle/terminal.h"
#include "lle/event_system.h"
#include "lle/hashtable_integration.h"

/* Phase 2: Buffer and Display */
#include "lle/buffer.h"
#include "lle/display.h"

/* Phase 3: Input and Integration */
#include "lle/input_parsing.h"
#include "lle/extensibility.h"

/* Phase 4: Features */
#include "lle/history.h"
#include "lle/syntax_highlighting.h"
#include "lle/completion.h"
#include "lle/autosuggestions.h"
#include "lle/customization.h"

/* Phase 5: Infrastructure */
#include "lle/plugin_api.h"
#include "lle/security.h"

/**
 * LLE Version Information
 */
#define LLE_VERSION_MAJOR 1
#define LLE_VERSION_MINOR 0
#define LLE_VERSION_PATCH 0

#endif /* LLE_H */
```

**Usage in Lusush**:
```c
#include "lle/lle.h"  // Single include for all LLE functionality

// Or include specific modules:
#include "lle/error_handling.h"
#include "lle/buffer.h"
```

---

## Compilation Standards

### Compiler Flags

All LLE code MUST compile with:
- `-std=c99` (C99 standard)
- `-Wall -Wextra -Werror` (all warnings, warnings are errors)
- `-O3` (optimization level 3)
- `-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700` (POSIX compliance)

### Zero Warnings Policy

**Absolute requirement**: LLE code MUST compile with ZERO warnings.

Any warning is considered a compilation failure and MUST be fixed before proceeding.

---

## Current Implementation Status

### Completed Modules
**None** - Zero LLE code exists after Nuclear Option #2.

### Next Module to Implement
**Spec 16: Error Handling** (`error_handling.c` / `error_handling.h`)

### Build System Status
- ✅ Directory structure defined
- ✅ Build approach decided (static library)
- ⏭️ `src/lle/meson.build` to be created
- ⏭️ Root `meson.build` to be updated
- ⏭️ First module implementation (Spec 16)

---

## Summary

**Structure**: Flat, professional, one spec = one module  
**Build**: Static library (`liblle.a`) linked into lusush  
**Naming**: Clear, consistent, matches specifications  
**Scalability**: Automatic via `fs.exists()` checks in meson  
**Maintenance**: Easy to navigate, easy to understand  

This structure provides:
- ✅ Clear organization
- ✅ Professional appearance
- ✅ Easy module location (flat = no hunting)
- ✅ Clean build system integration
- ✅ Automatic scaling as modules added
- ✅ Independent testing capability
- ✅ Reusability potential

**Ready to begin Spec 16 implementation with this infrastructure.**
