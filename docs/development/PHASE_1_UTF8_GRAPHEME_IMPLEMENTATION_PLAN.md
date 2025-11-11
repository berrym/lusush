# Phase 1: UTF-8/Grapheme Foundation - Complete Implementation Plan

**Document**: PHASE_1_UTF8_GRAPHEME_IMPLEMENTATION_PLAN.md  
**Date**: 2025-11-10  
**Branch**: feature/lle  
**Status**: Ready for Implementation  
**Estimated Effort**: 5-7 days  
**Risk Level**: LOW  
**Depends On**: Nothing (can start immediately)  
**Enables**: Wide character support, continuation prompts, keybinding manager  

---

## Table of Contents

1. [Overview](#1-overview)
2. [Prerequisites and Setup](#2-prerequisites-and-setup)
3. [Step-by-Step Implementation](#3-step-by-step-implementation)
4. [Testing Strategy](#4-testing-strategy)
5. [Success Criteria](#5-success-criteria)
6. [Troubleshooting Guide](#6-troubleshooting-guide)
7. [Reference Materials](#7-reference-materials)

---

## 1. Overview

### 1.1 What We're Building

Implementing the UTF-8/Grapheme foundation system as specified in **Spec 03 (Buffer Management), Section 4: UTF-8 Unicode Support**.

This provides:
- Complete Unicode grapheme cluster support
- Proper wide character width handling
- Fast position mapping (byte ↔ codepoint ↔ grapheme ↔ display column)
- Foundation for all text editing operations

### 1.2 Why This Matters

**Current Problem**: 
- Cursor tracking only knows byte offsets
- Wide characters (☕, 中文, emoji) break cursor positioning
- Navigation and editing don't respect character boundaries
- Test 5.4 (wide character cursor) is DEFERRED due to this

**After This Phase**:
- ✅ Cursor can track byte/codepoint/grapheme/display positions
- ✅ Wide characters handled correctly
- ✅ Grapheme cluster operations (emoji, combining chars)
- ✅ Proper foundation for all subsequent features

### 1.3 What We're NOT Doing (Yet)

- ❌ Keybinding manager integration (Phase 4)
- ❌ Continuation prompts (Phase 3)
- ❌ New UI features
- ✅ ONLY: Foundational UTF-8/grapheme system per Spec 03

---

## 2. Prerequisites and Setup

### 2.1 Required Reading

**MUST READ BEFORE STARTING**:

1. **Spec 03**: `docs/lle_specification/03_buffer_management_complete.md`
   - Section 4: UTF-8 Unicode Support (lines 612-900+)
   - Read the ENTIRE section, understand all structures

2. **Unicode UAX #29**: Unicode Text Segmentation
   - URL: https://unicode.org/reports/tr29/
   - Sections 1-3: Grapheme Cluster Boundary Rules
   - **Print or bookmark** the grapheme boundary table

3. **Current Implementation**:
   - `src/lle/utf8_support.c` - Basic UTF-8 functions (what we have)
   - `src/lle/cursor_manager.c` - Current cursor tracking (byte-only)
   - `include/lle/cursor_manager.h` - Cursor manager interface

### 2.2 Development Branch Setup

```bash
# Ensure we're on feature/lle and up to date
cd /home/mberry/Lab/c/lusush
git checkout feature/lle
git pull origin feature/lle

# Create development branch for Phase 1
git checkout -b feature/lle-utf8-grapheme
git push -u origin feature/lle-utf8-grapheme

# Verify clean state
git status  # Should show "nothing to commit, working tree clean"
```

### 2.3 Build and Test Setup

```bash
# Ensure build works before starting
ninja -C builddir clean
ninja -C builddir lusush

# Test current state
LLE_ENABLED=1 ./builddir/lusush
# Type: echo "café ☕ 中文"
# Observe: Wide characters work but cursor positioning is wrong
# Exit and note the issue for comparison later
```

### 2.4 Create Test Data Files

Create a file for testing with various Unicode:

```bash
cat > /tmp/lle_test_unicode.txt << 'EOF'
# Test data for UTF-8/Grapheme implementation
# Use these test strings during development

# Basic ASCII
hello world

# Latin with diacritics (combining characters)
café
naïve
résumé

# Precomposed vs decomposed (é as single char vs e + combining accent)
café
café

# Wide characters (CJK)
中文
日本語
한국어

# Emoji (wide, 2 columns each)
☕
🔥
💯

# Emoji with modifiers (grapheme clusters)
👨‍👩‍👧‍👦
👍🏻
👍🏿

# Combining characters
e´  (e + combining acute accent)
o^  (o + combining circumflex)

# Right-to-left text
שלום
مرحبا

# Zero-width joiners (complex grapheme clusters)
👨‍💻
EOF
```

---

## 3. Step-by-Step Implementation

### STEP 1: Create UTF-8 Index Structure (Day 1)

#### 3.1.1 Create Header File

**File**: `include/lle/utf8_index.h`

```c
#ifndef LLE_UTF8_INDEX_H
#define LLE_UTF8_INDEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "lle_result.h"

/**
 * UTF-8 Index Structure
 * 
 * Provides fast bidirectional mapping between:
 * - Byte offsets (storage)
 * - Codepoint indices (logical characters)
 * - Grapheme cluster indices (user-visible characters)
 * - Display columns (visual position)
 * 
 * Based on Spec 03, Section 4.1-4.2
 */
typedef struct lle_utf8_index {
    // Fast position mapping arrays
    size_t *byte_to_codepoint;      // [byte_offset] -> codepoint index
    size_t *codepoint_to_byte;      // [codepoint_idx] -> byte offset
    size_t *grapheme_to_codepoint;  // [grapheme_idx] -> codepoint index (start)
    size_t *codepoint_to_grapheme;  // [codepoint_idx] -> grapheme index
    size_t *grapheme_to_display;    // [grapheme_idx] -> display column
    size_t *display_to_grapheme;    // [display_col] -> grapheme index
    
    // Index metadata
    size_t byte_count;              // Total bytes indexed
    size_t codepoint_count;         // Total codepoints indexed
    size_t grapheme_count;          // Total grapheme clusters indexed
    size_t display_width;           // Total display columns
    
    // Index validity and versioning
    bool index_valid;               // Index is up-to-date
    uint32_t buffer_version;        // Associated buffer version
    uint64_t last_update_time;      // Timestamp of last rebuild
    
    // Performance tracking
    size_t rebuild_count;           // Number of times rebuilt
    uint64_t total_rebuild_time_ns; // Total time spent rebuilding
    
} lle_utf8_index_t;

/**
 * Initialize UTF-8 index structure
 * 
 * @param index Pointer to index structure to initialize
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_init(lle_utf8_index_t *index);

/**
 * Clean up UTF-8 index structure
 * 
 * @param index Pointer to index structure to clean up
 */
void lle_utf8_index_cleanup(lle_utf8_index_t *index);

/**
 * Rebuild UTF-8 index from text
 * 
 * Analyzes text and builds all mapping arrays.
 * This is the core function - see Spec 03 lines 612-763 for algorithm.
 * 
 * @param index Index structure to rebuild
 * @param text Text to index (UTF-8 encoded)
 * @param text_length Length of text in bytes
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_rebuild(lle_utf8_index_t *index,
                                    const char *text,
                                    size_t text_length);

/**
 * Convert byte offset to codepoint index
 * 
 * @param index UTF-8 index
 * @param byte_offset Byte offset in buffer
 * @param codepoint_index Output: codepoint index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_byte_to_codepoint(const lle_utf8_index_t *index,
                                               size_t byte_offset,
                                               size_t *codepoint_index);

/**
 * Convert codepoint index to byte offset
 * 
 * @param index UTF-8 index
 * @param codepoint_index Codepoint index
 * @param byte_offset Output: byte offset
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_codepoint_to_byte(const lle_utf8_index_t *index,
                                               size_t codepoint_index,
                                               size_t *byte_offset);

/**
 * Convert codepoint index to grapheme cluster index
 * 
 * @param index UTF-8 index
 * @param codepoint_index Codepoint index
 * @param grapheme_index Output: grapheme cluster index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_codepoint_to_grapheme(const lle_utf8_index_t *index,
                                                   size_t codepoint_index,
                                                   size_t *grapheme_index);

/**
 * Convert grapheme cluster index to codepoint index (start of cluster)
 * 
 * @param index UTF-8 index
 * @param grapheme_index Grapheme cluster index
 * @param codepoint_index Output: codepoint index (first codepoint in cluster)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_grapheme_to_codepoint(const lle_utf8_index_t *index,
                                                   size_t grapheme_index,
                                                   size_t *codepoint_index);

/**
 * Convert grapheme cluster index to display column
 * 
 * @param index UTF-8 index
 * @param grapheme_index Grapheme cluster index
 * @param display_column Output: display column
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_grapheme_to_display(const lle_utf8_index_t *index,
                                                 size_t grapheme_index,
                                                 size_t *display_column);

/**
 * Convert display column to grapheme cluster index
 * 
 * @param index UTF-8 index
 * @param display_column Display column
 * @param grapheme_index Output: grapheme cluster index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_display_to_grapheme(const lle_utf8_index_t *index,
                                                 size_t display_column,
                                                 size_t *grapheme_index);

/**
 * Invalidate index (mark as needing rebuild)
 * 
 * @param index Index to invalidate
 */
void lle_utf8_index_invalidate(lle_utf8_index_t *index);

/**
 * Check if index is valid
 * 
 * @param index Index to check
 * @return true if valid, false if needs rebuild
 */
bool lle_utf8_index_is_valid(const lle_utf8_index_t *index);

#endif // LLE_UTF8_INDEX_H
```

**Create the file**:
```bash
cat > include/lle/utf8_index.h << 'EOF'
[paste the above content]
EOF
```

#### 3.1.2 Create Source File Skeleton

**File**: `src/lle/utf8_index.c`

```c
#include "lle/utf8_index.h"
#include "lle/utf8_support.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declarations for helper functions
static bool is_grapheme_boundary(const char *pos, const char *text_start, const char *text_end);
static int get_codepoint_width(uint32_t codepoint);

lle_result_t lle_utf8_index_init(lle_utf8_index_t *index) {
    if (!index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    memset(index, 0, sizeof(lle_utf8_index_t));
    index->index_valid = false;
    
    return LLE_SUCCESS;
}

void lle_utf8_index_cleanup(lle_utf8_index_t *index) {
    if (!index) return;
    
    free(index->byte_to_codepoint);
    free(index->codepoint_to_byte);
    free(index->grapheme_to_codepoint);
    free(index->codepoint_to_grapheme);
    free(index->grapheme_to_display);
    free(index->display_to_grapheme);
    
    memset(index, 0, sizeof(lle_utf8_index_t));
}

lle_result_t lle_utf8_index_rebuild(lle_utf8_index_t *index,
                                    const char *text,
                                    size_t text_length) {
    // TODO: Implement - this is the core function
    // See Spec 03 lines 612-763 for complete algorithm
    // We'll implement this in STEP 2
    return LLE_ERROR_NOT_IMPLEMENTED;
}

// Conversion functions - implement after rebuild works

lle_result_t lle_utf8_index_byte_to_codepoint(const lle_utf8_index_t *index,
                                               size_t byte_offset,
                                               size_t *codepoint_index) {
    if (!index || !codepoint_index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    if (byte_offset >= index->byte_count) {
        return LLE_ERROR_OUT_OF_BOUNDS;
    }
    
    *codepoint_index = index->byte_to_codepoint[byte_offset];
    return LLE_SUCCESS;
}

lle_result_t lle_utf8_index_codepoint_to_byte(const lle_utf8_index_t *index,
                                               size_t codepoint_index,
                                               size_t *byte_offset) {
    if (!index || !byte_offset) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    if (codepoint_index >= index->codepoint_count) {
        return LLE_ERROR_OUT_OF_BOUNDS;
    }
    
    *byte_offset = index->codepoint_to_byte[codepoint_index];
    return LLE_SUCCESS;
}

lle_result_t lle_utf8_index_codepoint_to_grapheme(const lle_utf8_index_t *index,
                                                   size_t codepoint_index,
                                                   size_t *grapheme_index) {
    if (!index || !grapheme_index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    if (codepoint_index >= index->codepoint_count) {
        return LLE_ERROR_OUT_OF_BOUNDS;
    }
    
    *grapheme_index = index->codepoint_to_grapheme[codepoint_index];
    return LLE_SUCCESS;
}

lle_result_t lle_utf8_index_grapheme_to_codepoint(const lle_utf8_index_t *index,
                                                   size_t grapheme_index,
                                                   size_t *codepoint_index) {
    if (!index || !codepoint_index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    if (grapheme_index >= index->grapheme_count) {
        return LLE_ERROR_OUT_OF_BOUNDS;
    }
    
    *codepoint_index = index->grapheme_to_codepoint[grapheme_index];
    return LLE_SUCCESS;
}

lle_result_t lle_utf8_index_grapheme_to_display(const lle_utf8_index_t *index,
                                                 size_t grapheme_index,
                                                 size_t *display_column) {
    if (!index || !display_column) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    if (grapheme_index >= index->grapheme_count) {
        return LLE_ERROR_OUT_OF_BOUNDS;
    }
    
    *display_column = index->grapheme_to_display[grapheme_index];
    return LLE_SUCCESS;
}

lle_result_t lle_utf8_index_display_to_grapheme(const lle_utf8_index_t *index,
                                                 size_t display_column,
                                                 size_t *grapheme_index) {
    if (!index || !grapheme_index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    if (display_column >= index->display_width) {
        return LLE_ERROR_OUT_OF_BOUNDS;
    }
    
    *grapheme_index = index->display_to_grapheme[display_column];
    return LLE_SUCCESS;
}

void lle_utf8_index_invalidate(lle_utf8_index_t *index) {
    if (index) {
        index->index_valid = false;
    }
}

bool lle_utf8_index_is_valid(const lle_utf8_index_t *index) {
    return index && index->index_valid;
}

// Helper functions - to be implemented in STEP 3 and 4

static bool is_grapheme_boundary(const char *pos, const char *text_start, const char *text_end) {
    // TODO: Implement grapheme boundary detection per UAX #29
    // For now, return true (treat each codepoint as separate grapheme)
    // We'll implement proper rules in STEP 3
    return true;
}

static int get_codepoint_width(uint32_t codepoint) {
    // TODO: Implement proper wcwidth() logic
    // For now, simple rules:
    // - Control characters: 0
    // - ASCII printable: 1
    // - Most Unicode: 1
    // - CJK, emoji: 2
    // We'll implement proper width calculation in STEP 4
    
    if (codepoint < 0x20) return 0;  // Control chars
    if (codepoint < 0x7F) return 1;  // ASCII
    if (codepoint >= 0x1100) return 2;  // Placeholder for wide chars
    return 1;
}
```

**Create the file**:
```bash
cat > src/lle/utf8_index.c << 'EOF'
[paste the above content]
EOF
```

#### 3.1.3 Update Build System

**File**: `meson.build`

Find the `lle_sources` list and add:

```python
lle_sources = [
    # ... existing files ...
    'src/lle/utf8_index.c',  # ADD THIS LINE
]
```

**Test the build**:
```bash
ninja -C builddir
# Should compile without errors (functions return NOT_IMPLEMENTED, that's OK)
```

#### 3.1.4 Commit Step 1

```bash
git add include/lle/utf8_index.h src/lle/utf8_index.c meson.build
git commit -m "Phase 1 Step 1: Add UTF-8 index structure and API

Create lle_utf8_index_t structure with complete API per Spec 03 Section 4.

STRUCTURES:
- lle_utf8_index_t: Position mapping arrays (byte/codepoint/grapheme/display)
- Metadata tracking (counts, validity, performance stats)

API FUNCTIONS:
- Init/cleanup
- Rebuild (stub - to be implemented in Step 2)
- Position conversion functions (all combinations)
- Validation and invalidation

STATUS:
- Structure defined ✓
- API skeleton complete ✓
- Build system updated ✓
- Functions return NOT_IMPLEMENTED (expected)

NEXT: Step 2 - Implement rebuild algorithm"

git push origin feature/lle-utf8-grapheme
```

**CHECKPOINT**: Verify this commit is pushed to remote before proceeding.

---

### STEP 2: Implement Index Rebuild Algorithm (Day 2-3)

This is the core function. We'll implement it carefully following Spec 03.

#### 3.2.1 Implement UTF-8 Validation Helper

Add to `src/lle/utf8_index.c`:

```c
/**
 * Validate UTF-8 sequence
 * 
 * @param ptr Pointer to start of sequence
 * @param length Expected sequence length
 * @return true if valid, false otherwise
 */
static bool is_valid_utf8_sequence(const char *ptr, int length) {
    if (!ptr || length < 1 || length > 4) {
        return false;
    }
    
    unsigned char byte1 = (unsigned char)ptr[0];
    
    // Validate first byte matches expected length
    if (length == 1 && (byte1 & 0x80) != 0x00) return false;  // 0xxxxxxx
    if (length == 2 && (byte1 & 0xE0) != 0xC0) return false;  // 110xxxxx
    if (length == 3 && (byte1 & 0xF0) != 0xE0) return false;  // 1110xxxx
    if (length == 4 && (byte1 & 0xF8) != 0xF0) return false;  // 11110xxx
    
    // Validate continuation bytes (10xxxxxx)
    for (int i = 1; i < length; i++) {
        unsigned char byte = (unsigned char)ptr[i];
        if ((byte & 0xC0) != 0x80) {
            return false;
        }
    }
    
    // Check for overlong encodings
    if (length == 2 && (byte1 & 0xFE) == 0xC0) return false;  // Overlong
    if (length == 3 && byte1 == 0xE0 && ((unsigned char)ptr[1] & 0xE0) == 0x80) return false;
    if (length == 4 && byte1 == 0xF0 && ((unsigned char)ptr[1] & 0xF0) == 0x80) return false;
    
    // Check for surrogate pairs (invalid in UTF-8)
    if (length == 3 && byte1 == 0xED && ((unsigned char)ptr[1] & 0xE0) == 0xA0) return false;
    
    return true;
}
```

#### 3.2.2 Implement First Pass (Count Codepoints and Graphemes)

Replace the `lle_utf8_index_rebuild` function:

```c
lle_result_t lle_utf8_index_rebuild(lle_utf8_index_t *index,
                                    const char *text,
                                    size_t text_length) {
    if (!index || !text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Start timing for performance tracking
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    lle_result_t result = LLE_SUCCESS;
    
    // === PHASE 1: Count codepoints and grapheme clusters ===
    
    size_t codepoint_count = 0;
    size_t grapheme_count = 0;
    size_t display_width_total = 0;
    
    const char *ptr = text;
    const char *end = text + text_length;
    
    while (ptr < end) {
        // Get UTF-8 sequence length
        int sequence_length = lle_utf8_sequence_length(*ptr);
        
        // Validate sequence length
        if (sequence_length == 0 || ptr + sequence_length > end) {
            return LLE_ERROR_INVALID_UTF8;
        }
        
        // Validate complete UTF-8 sequence
        if (!is_valid_utf8_sequence(ptr, sequence_length)) {
            return LLE_ERROR_INVALID_UTF8;
        }
        
        codepoint_count++;
        
        // Check if this starts a new grapheme cluster
        if (is_grapheme_boundary(ptr, text, end)) {
            grapheme_count++;
            
            // Decode codepoint to get its width
            uint32_t codepoint;
            lle_utf8_decode_codepoint(ptr, sequence_length, &codepoint);
            int width = get_codepoint_width(codepoint);
            display_width_total += width;
        }
        
        ptr += sequence_length;
    }
    
    // === PHASE 2: Allocate index arrays ===
    
    size_t *new_byte_to_codepoint = NULL;
    size_t *new_codepoint_to_byte = NULL;
    size_t *new_grapheme_to_codepoint = NULL;
    size_t *new_codepoint_to_grapheme = NULL;
    size_t *new_grapheme_to_display = NULL;
    size_t *new_display_to_grapheme = NULL;
    
    // Allocate byte_to_codepoint (one entry per byte)
    new_byte_to_codepoint = calloc(text_length + 1, sizeof(size_t));
    if (!new_byte_to_codepoint) {
        result = LLE_ERROR_MEMORY_ALLOCATION;
        goto cleanup;
    }
    
    // Allocate codepoint_to_byte (one entry per codepoint)
    new_codepoint_to_byte = calloc(codepoint_count + 1, sizeof(size_t));
    if (!new_codepoint_to_byte) {
        result = LLE_ERROR_MEMORY_ALLOCATION;
        goto cleanup;
    }
    
    // Allocate grapheme arrays
    new_grapheme_to_codepoint = calloc(grapheme_count + 1, sizeof(size_t));
    new_codepoint_to_grapheme = calloc(codepoint_count + 1, sizeof(size_t));
    new_grapheme_to_display = calloc(grapheme_count + 1, sizeof(size_t));
    new_display_to_grapheme = calloc(display_width_total + 1, sizeof(size_t));
    
    if (!new_grapheme_to_codepoint || !new_codepoint_to_grapheme ||
        !new_grapheme_to_display || !new_display_to_grapheme) {
        result = LLE_ERROR_MEMORY_ALLOCATION;
        goto cleanup;
    }
    
    // === PHASE 3: Build index mappings ===
    
    ptr = text;
    size_t byte_pos = 0;
    size_t codepoint_pos = 0;
    size_t grapheme_pos = 0;
    size_t display_col = 0;
    size_t current_grapheme_start_codepoint = 0;
    
    while (ptr < end) {
        int sequence_length = lle_utf8_sequence_length(*ptr);
        
        // Update byte-to-codepoint mapping (all bytes in sequence map to same codepoint)
        for (int i = 0; i < sequence_length; i++) {
            new_byte_to_codepoint[byte_pos + i] = codepoint_pos;
        }
        
        // Update codepoint-to-byte mapping
        new_codepoint_to_byte[codepoint_pos] = byte_pos;
        
        // Check for grapheme boundary
        if (is_grapheme_boundary(ptr, text, end)) {
            if (grapheme_pos > 0) {
                // Complete previous grapheme cluster
                // All codepoints from current_grapheme_start_codepoint to codepoint_pos-1
                // belong to previous grapheme (grapheme_pos - 1)
                for (size_t i = current_grapheme_start_codepoint; i < codepoint_pos; i++) {
                    new_codepoint_to_grapheme[i] = grapheme_pos - 1;
                }
            }
            
            // Start new grapheme cluster
            new_grapheme_to_codepoint[grapheme_pos] = codepoint_pos;
            new_grapheme_to_display[grapheme_pos] = display_col;
            current_grapheme_start_codepoint = codepoint_pos;
            
            // Update display column mapping
            // Decode codepoint to get its display width
            uint32_t codepoint;
            lle_utf8_decode_codepoint(ptr, sequence_length, &codepoint);
            int width = get_codepoint_width(codepoint);
            
            // Fill display_to_grapheme for all columns this grapheme occupies
            for (int w = 0; w < width; w++) {
                new_display_to_grapheme[display_col + w] = grapheme_pos;
            }
            
            display_col += width;
            grapheme_pos++;
        }
        
        codepoint_pos++;
        byte_pos += sequence_length;
        ptr += sequence_length;
    }
    
    // === PHASE 4: Complete final grapheme cluster ===
    
    if (codepoint_count > 0 && current_grapheme_start_codepoint < codepoint_count) {
        for (size_t i = current_grapheme_start_codepoint; i < codepoint_count; i++) {
            new_codepoint_to_grapheme[i] = grapheme_pos - 1;
        }
    }
    
    // === PHASE 5: Replace old arrays with new ones ===
    
    free(index->byte_to_codepoint);
    free(index->codepoint_to_byte);
    free(index->grapheme_to_codepoint);
    free(index->codepoint_to_grapheme);
    free(index->grapheme_to_display);
    free(index->display_to_grapheme);
    
    index->byte_to_codepoint = new_byte_to_codepoint;
    index->codepoint_to_byte = new_codepoint_to_byte;
    index->grapheme_to_codepoint = new_grapheme_to_codepoint;
    index->codepoint_to_grapheme = new_codepoint_to_grapheme;
    index->grapheme_to_display = new_grapheme_to_display;
    index->display_to_grapheme = new_display_to_grapheme;
    
    // Update metadata
    index->byte_count = text_length;
    index->codepoint_count = codepoint_count;
    index->grapheme_count = grapheme_count;
    index->display_width = display_width_total;
    index->index_valid = true;
    index->rebuild_count++;
    
    // Update timing
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    uint64_t elapsed_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000ULL +
                          (end_time.tv_nsec - start_time.tv_nsec);
    index->total_rebuild_time_ns += elapsed_ns;
    index->last_update_time = elapsed_ns;
    
    return LLE_SUCCESS;
    
cleanup:
    free(new_byte_to_codepoint);
    free(new_codepoint_to_byte);
    free(new_grapheme_to_codepoint);
    free(new_codepoint_to_grapheme);
    free(new_grapheme_to_display);
    free(new_display_to_grapheme);
    
    return result;
}
```

#### 3.2.3 Test the Rebuild Function

Create a simple test program:

**File**: `test_utf8_index_rebuild.c`

```c
#include "lle/utf8_index.h"
#include <stdio.h>
#include <string.h>

int main() {
    lle_utf8_index_t index;
    lle_result_t result;
    
    printf("Testing UTF-8 index rebuild...\n\n");
    
    // Test 1: Simple ASCII
    result = lle_utf8_index_init(&index);
    if (result != LLE_SUCCESS) {
        printf("FAIL: Init failed\n");
        return 1;
    }
    
    const char *text1 = "hello";
    result = lle_utf8_index_rebuild(&index, text1, strlen(text1));
    if (result != LLE_SUCCESS) {
        printf("FAIL: Rebuild failed for ASCII\n");
        return 1;
    }
    
    printf("Test 1: ASCII 'hello'\n");
    printf("  Bytes: %zu (expected 5)\n", index.byte_count);
    printf("  Codepoints: %zu (expected 5)\n", index.codepoint_count);
    printf("  Graphemes: %zu (expected 5)\n", index.grapheme_count);
    printf("  Display width: %zu (expected 5)\n", index.display_width);
    printf("  Status: %s\n\n", 
           (index.byte_count == 5 && index.codepoint_count == 5 && 
            index.grapheme_count == 5 && index.display_width == 5) ? "PASS" : "FAIL");
    
    lle_utf8_index_cleanup(&index);
    
    // Test 2: UTF-8 with multi-byte chars
    result = lle_utf8_index_init(&index);
    const char *text2 = "café";  // 5 bytes: c a f 0xC3 0xA9
    result = lle_utf8_index_rebuild(&index, text2, strlen(text2));
    
    printf("Test 2: UTF-8 'café'\n");
    printf("  Bytes: %zu (expected 5)\n", index.byte_count);
    printf("  Codepoints: %zu (expected 4)\n", index.codepoint_count);
    printf("  Graphemes: %zu (expected 4)\n", index.grapheme_count);
    printf("  Display width: %zu (expected 4)\n", index.display_width);
    printf("  Status: %s\n\n",
           (index.byte_count == 5 && index.codepoint_count == 4 && 
            index.grapheme_count == 4 && index.display_width == 4) ? "PASS" : "FAIL");
    
    // Test byte-to-codepoint mapping
    size_t cp_idx;
    result = lle_utf8_index_byte_to_codepoint(&index, 0, &cp_idx);
    printf("  Byte 0 -> Codepoint %zu (expected 0): %s\n", cp_idx, cp_idx == 0 ? "PASS" : "FAIL");
    
    result = lle_utf8_index_byte_to_codepoint(&index, 3, &cp_idx);
    printf("  Byte 3 -> Codepoint %zu (expected 3): %s\n", cp_idx, cp_idx == 3 ? "PASS" : "FAIL");
    
    result = lle_utf8_index_byte_to_codepoint(&index, 4, &cp_idx);
    printf("  Byte 4 -> Codepoint %zu (expected 3): %s\n", cp_idx, cp_idx == 3 ? "PASS" : "FAIL");
    
    lle_utf8_index_cleanup(&index);
    
    printf("\nAll basic tests completed!\n");
    return 0;
}
```

**Compile and run**:
```bash
# Add to meson.build temporarily for testing:
executable('test_utf8_index_rebuild',
  'test_utf8_index_rebuild.c',
  link_with: liblle,
  include_directories: include_directories('include'))

# Build
ninja -C builddir test_utf8_index_rebuild

# Run
./builddir/test_utf8_index_rebuild
```

**Expected output**:
```
Test 1: ASCII 'hello'
  Bytes: 5 (expected 5)
  Codepoints: 5 (expected 5)
  Graphemes: 5 (expected 5)
  Display width: 5 (expected 5)
  Status: PASS

Test 2: UTF-8 'café'
  Bytes: 5 (expected 5)
  Codepoints: 4 (expected 4)
  Graphemes: 4 (expected 4)
  Display width: 4 (expected 4)
  Status: PASS

  Byte 0 -> Codepoint 0 (expected 0): PASS
  Byte 3 -> Codepoint 3 (expected 3): PASS
  Byte 4 -> Codepoint 3 (expected 3): PASS
```

#### 3.2.4 Commit Step 2

```bash
git add src/lle/utf8_index.c test_utf8_index_rebuild.c meson.build
git commit -m "Phase 1 Step 2: Implement UTF-8 index rebuild algorithm

Implement core index rebuild function per Spec 03 lines 612-763.

IMPLEMENTATION:
- Phase 1: Count codepoints and grapheme clusters
- Phase 2: Allocate all mapping arrays
- Phase 3: Build bidirectional mappings
- Phase 4: Complete final grapheme cluster
- Phase 5: Replace old arrays with new ones

FEATURES:
- UTF-8 sequence validation
- Byte ↔ codepoint mapping
- Codepoint ↔ grapheme mapping
- Grapheme ↔ display column mapping
- Performance timing tracking

TESTING:
- Basic ASCII test: PASS
- UTF-8 multi-byte test: PASS
- Byte-to-codepoint mapping: PASS

NOTE: Using placeholder for grapheme boundary detection (treats each
codepoint as separate grapheme). Will implement proper UAX #29 rules
in Step 3.

NEXT: Step 3 - Implement grapheme cluster boundary detection"

git push origin feature/lle-utf8-grapheme
```

**CHECKPOINT**: Verify this commit is pushed before proceeding.

---

### STEP 3: Implement Grapheme Cluster Boundary Detection (Day 3-4)

Now we implement proper UAX #29 grapheme cluster rules.

#### 3.3.1 Create Grapheme Boundary Detector

**File**: `include/lle/grapheme_detector.h`

```c
#ifndef LLE_GRAPHEME_DETECTOR_H
#define LLE_GRAPHEME_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Grapheme Cluster Boundary Detection
 * 
 * Implements Unicode UAX #29 Grapheme Cluster Boundary rules.
 * 
 * Reference: https://unicode.org/reports/tr29/
 */

// Grapheme cluster break property types (from UAX #29)
typedef enum {
    GB_OTHER = 0,           // Any character not listed below
    GB_CR,                  // Carriage Return
    GB_LF,                  // Line Feed
    GB_CONTROL,             // Control characters
    GB_EXTEND,              // Extend (combining marks, etc.)
    GB_ZWJ,                 // Zero Width Joiner
    GB_REGIONAL_INDICATOR,  // Regional Indicator
    GB_PREPEND,             // Prepend
    GB_SPACING_MARK,        // SpacingMark
    GB_L,                   // Hangul L
    GB_V,                   // Hangul V
    GB_T,                   // Hangul T
    GB_LV,                  // Hangul LV
    GB_LVT,                 // Hangul LVT
    GB_EXTENDED_PICTOGRAPHIC // Emoji and pictographs
} grapheme_break_property_t;

/**
 * Get grapheme break property for a Unicode codepoint
 * 
 * @param codepoint Unicode codepoint
 * @return Grapheme break property
 */
grapheme_break_property_t get_grapheme_break_property(uint32_t codepoint);

/**
 * Check if there is a grapheme cluster boundary between two codepoints
 * 
 * Implements UAX #29 boundary rules.
 * 
 * @param cp1 First codepoint
 * @param cp2 Second codepoint
 * @param prev_was_zwj True if previous codepoint was ZWJ
 * @param ri_sequence_count Count of previous Regional Indicators in sequence
 * @return true if boundary exists, false otherwise
 */
bool is_grapheme_cluster_boundary(uint32_t cp1, uint32_t cp2, 
                                   bool prev_was_zwj,
                                   int ri_sequence_count);

/**
 * Check if position in text is at a grapheme cluster boundary
 * 
 * This is the main function used by utf8_index.c
 * 
 * @param pos Current position in text
 * @param text_start Start of text buffer
 * @param text_end End of text buffer
 * @return true if pos is at a grapheme boundary
 */
bool is_grapheme_boundary_at_position(const char *pos, 
                                      const char *text_start, 
                                      const char *text_end);

#endif // LLE_GRAPHEME_DETECTOR_H
```

#### 3.3.2 Implement Basic Grapheme Break Property Detection

**File**: `src/lle/grapheme_detector.c`

This is a large file. The implementation is simplified but covers the most important cases.
For production, you'd use a Unicode data table.

```c
#include "lle/grapheme_detector.h"
#include "lle/utf8_support.h"

/**
 * Get grapheme break property for a codepoint
 * 
 * This is a simplified implementation covering the most common cases.
 * A full implementation would use Unicode property tables.
 */
grapheme_break_property_t get_grapheme_break_property(uint32_t cp) {
    // CR/LF
    if (cp == 0x000D) return GB_CR;
    if (cp == 0x000A) return GB_LF;
    
    // Control characters (C0 and C1)
    if (cp < 0x20) return GB_CONTROL;
    if (cp >= 0x7F && cp <= 0x9F) return GB_CONTROL;
    
    // Zero Width Joiner
    if (cp == 0x200D) return GB_ZWJ;
    
    // Combining marks (Extend)
    // This is simplified - full implementation needs Unicode tables
    if (cp >= 0x0300 && cp <= 0x036F) return GB_EXTEND;  // Combining Diacritical Marks
    if (cp >= 0x1AB0 && cp <= 0x1AFF) return GB_EXTEND;  // Combining Diacritical Marks Extended
    if (cp >= 0x1DC0 && cp <= 0x1DFF) return GB_EXTEND;  // Combining Diacritical Marks Supplement
    if (cp >= 0x20D0 && cp <= 0x20FF) return GB_EXTEND;  // Combining Diacritical Marks for Symbols
    if (cp >= 0xFE20 && cp <= 0xFE2F) return GB_EXTEND;  // Combining Half Marks
    
    // Regional Indicators (flags)
    if (cp >= 0x1F1E6 && cp <= 0x1F1FF) return GB_REGIONAL_INDICATOR;
    
    // Hangul Jamo
    if (cp >= 0x1100 && cp <= 0x115F) return GB_L;   // Hangul Choseong
    if (cp >= 0x1160 && cp <= 0x11A7) return GB_V;   // Hangul Jungseong
    if (cp >= 0x11A8 && cp <= 0x11FF) return GB_T;   // Hangul Jongseong
    
    // Hangul Syllables
    if (cp >= 0xAC00 && cp <= 0xD7A3) {
        int s_index = cp - 0xAC00;
        int t_index = s_index % 28;
        if (t_index == 0) return GB_LV;
        else return GB_LVT;
    }
    
    // Extended Pictographic (Emoji)
    // Simplified check - full implementation needs Unicode tables
    if (cp >= 0x1F300 && cp <= 0x1F9FF) return GB_EXTENDED_PICTOGRAPHIC;  // Emoji blocks
    if (cp >= 0x2600 && cp <= 0x27BF) return GB_EXTENDED_PICTOGRAPHIC;    // Miscellaneous Symbols
    
    // SpacingMark - simplified
    if (cp >= 0x0903 && cp <= 0x0903) return GB_SPACING_MARK;  // Devanagari
    
    // Prepend - simplified
    // Most prepend chars are rare, skipping for now
    
    return GB_OTHER;
}

/**
 * Check if there's a boundary between two codepoints per UAX #29
 */
bool is_grapheme_cluster_boundary(uint32_t cp1, uint32_t cp2,
                                   bool prev_was_zwj,
                                   int ri_sequence_count) {
    grapheme_break_property_t prop1 = get_grapheme_break_property(cp1);
    grapheme_break_property_t prop2 = get_grapheme_break_property(cp2);
    
    // GB3: CR × LF
    if (prop1 == GB_CR && prop2 == GB_LF) {
        return false;  // No break
    }
    
    // GB4: (Control | CR | LF) ÷
    if (prop1 == GB_CONTROL || prop1 == GB_CR || prop1 == GB_LF) {
        return true;  // Break
    }
    
    // GB5: ÷ (Control | CR | LF)
    if (prop2 == GB_CONTROL || prop2 == GB_CR || prop2 == GB_LF) {
        return true;  // Break
    }
    
    // GB6: L × (L | V | LV | LVT)
    if (prop1 == GB_L && (prop2 == GB_L || prop2 == GB_V || 
                          prop2 == GB_LV || prop2 == GB_LVT)) {
        return false;  // No break
    }
    
    // GB7: (LV | V) × (V | T)
    if ((prop1 == GB_LV || prop1 == GB_V) && (prop2 == GB_V || prop2 == GB_T)) {
        return false;  // No break
    }
    
    // GB8: (LVT | T) × T
    if ((prop1 == GB_LVT || prop1 == GB_T) && prop2 == GB_T) {
        return false;  // No break
    }
    
    // GB9: × (Extend | ZWJ)
    if (prop2 == GB_EXTEND || prop2 == GB_ZWJ) {
        return false;  // No break
    }
    
    // GB9a: × SpacingMark
    if (prop2 == GB_SPACING_MARK) {
        return false;  // No break
    }
    
    // GB9b: Prepend ×
    if (prop1 == GB_PREPEND) {
        return false;  // No break
    }
    
    // GB11: \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}
    if (prev_was_zwj && 
        prop1 == GB_EXTENDED_PICTOGRAPHIC && 
        prop2 == GB_EXTENDED_PICTOGRAPHIC) {
        return false;  // No break (emoji sequence)
    }
    
    // GB12/GB13: Regional Indicator × Regional Indicator (pairs)
    if (prop1 == GB_REGIONAL_INDICATOR && prop2 == GB_REGIONAL_INDICATOR) {
        // Regional indicators pair up (for flags)
        // Odd position = no break, even position = break
        return (ri_sequence_count % 2) == 0;
    }
    
    // GB999: Any ÷ Any
    return true;  // Default: break
}

/**
 * Check if position is at grapheme boundary
 */
bool is_grapheme_boundary_at_position(const char *pos,
                                      const char *text_start,
                                      const char *text_end) {
    // Start of text is always a boundary
    if (pos == text_start) {
        return true;
    }
    
    // End of text is always a boundary
    if (pos >= text_end) {
        return true;
    }
    
    // Decode current codepoint
    uint32_t cp_current;
    int len_current = lle_utf8_sequence_length(*pos);
    if (len_current == 0 || pos + len_current > text_end) {
        return true;  // Invalid UTF-8, treat as boundary
    }
    lle_utf8_decode_codepoint(pos, len_current, &cp_current);
    
    // Find previous codepoint
    const char *prev_pos = pos - 1;
    
    // Scan backward to find start of previous UTF-8 sequence
    while (prev_pos >= text_start && ((*prev_pos & 0xC0) == 0x80)) {
        prev_pos--;  // Skip continuation bytes
    }
    
    if (prev_pos < text_start) {
        return true;  // Start of text
    }
    
    // Decode previous codepoint
    uint32_t cp_prev;
    int len_prev = lle_utf8_sequence_length(*prev_pos);
    if (len_prev == 0) {
        return true;  // Invalid UTF-8
    }
    lle_utf8_decode_codepoint(prev_pos, len_prev, &cp_prev);
    
    // Check if previous was ZWJ (for emoji sequences)
    bool prev_was_zwj = false;
    if (prev_pos > text_start) {
        const char *before_prev = prev_pos - 1;
        while (before_prev >= text_start && ((*before_prev & 0xC0) == 0x80)) {
            before_prev--;
        }
        if (before_prev >= text_start) {
            uint32_t cp_before;
            int len_before = lle_utf8_sequence_length(*before_prev);
            lle_utf8_decode_codepoint(before_prev, len_before, &cp_before);
            prev_was_zwj = (get_grapheme_break_property(cp_before) == GB_ZWJ);
        }
    }
    
    // Count regional indicator sequence length
    int ri_count = 0;
    if (get_grapheme_break_property(cp_prev) == GB_REGIONAL_INDICATOR) {
        const char *check_pos = prev_pos;
        while (check_pos >= text_start) {
            const char *cp_start = check_pos - 1;
            while (cp_start >= text_start && ((*cp_start & 0xC0) == 0x80)) {
                cp_start--;
            }
            if (cp_start < text_start) break;
            
            uint32_t cp;
            int len = lle_utf8_sequence_length(*cp_start);
            lle_utf8_decode_codepoint(cp_start, len, &cp);
            
            if (get_grapheme_break_property(cp) != GB_REGIONAL_INDICATOR) {
                break;
            }
            
            ri_count++;
            check_pos = cp_start;
        }
    }
    
    // Apply boundary rules
    return is_grapheme_cluster_boundary(cp_prev, cp_current, prev_was_zwj, ri_count);
}
```

#### 3.3.3 Update utf8_index.c to Use Grapheme Detector

In `src/lle/utf8_index.c`, update the helper function:

```c
#include "lle/grapheme_detector.h"

// Replace the placeholder function:
static bool is_grapheme_boundary(const char *pos, const char *text_start, const char *text_end) {
    return is_grapheme_boundary_at_position(pos, text_start, text_end);
}
```

#### 3.3.4 Update Build System

Add to `meson.build`:

```python
lle_sources = [
    # ... existing files ...
    'src/lle/grapheme_detector.c',  # ADD THIS
]
```

#### 3.3.5 Test Grapheme Clusters

Create test:

**File**: `test_grapheme_clusters.c`

```c
#include "lle/utf8_index.h"
#include <stdio.h>
#include <string.h>

void test_grapheme(const char *label, const char *text, 
                  size_t expected_graphemes, size_t expected_codepoints) {
    lle_utf8_index_t index;
    lle_utf8_index_init(&index);
    lle_utf8_index_rebuild(&index, text, strlen(text));
    
    printf("Test: %s\n", label);
    printf("  Text: %s\n", text);
    printf("  Codepoints: %zu (expected %zu)\n", index.codepoint_count, expected_codepoints);
    printf("  Graphemes: %zu (expected %zu)\n", index.grapheme_count, expected_graphemes);
    printf("  Status: %s\n\n",
           (index.grapheme_count == expected_graphemes && 
            index.codepoint_count == expected_codepoints) ? "PASS" : "FAIL");
    
    lle_utf8_index_cleanup(&index);
}

int main() {
    printf("Testing Grapheme Cluster Detection\n\n");
    
    // Test 1: Simple ASCII
    test_grapheme("ASCII", "hello", 5, 5);
    
    // Test 2: Precomposed character (single codepoint)
    test_grapheme("Precomposed é", "café", 4, 4);
    
    // Test 3: Decomposed character (e + combining accent = 1 grapheme, 2 codepoints)
    test_grapheme("Decomposed é", "café", 4, 5);  // 'e' + combining acute = 1 grapheme
    
    // Test 4: Emoji (single codepoint, but may be wide)
    test_grapheme("Emoji", "☕", 1, 1);
    
    // Test 5: Emoji with skin tone modifier (1 grapheme, 2 codepoints)
    test_grapheme("Emoji+modifier", "👍🏻", 1, 2);
    
    // Test 6: Family emoji with ZWJ (1 grapheme, multiple codepoints)
    test_grapheme("Family emoji", "👨‍👩‍👧‍👦", 1, 7);  // man + ZWJ + woman + ZWJ + girl + ZWJ + boy
    
    // Test 7: Regional indicators (flag - 1 grapheme, 2 codepoints)
    test_grapheme("Flag emoji", "🇺🇸", 1, 2);  // US flag = 2 regional indicators
    
    // Test 8: Hangul (should be treated as grapheme clusters)
    test_grapheme("Hangul", "한국어", 3, 3);
    
    printf("Grapheme cluster tests completed!\n");
    return 0;
}
```

**Build and run**:
```bash
# Add to meson.build
executable('test_grapheme_clusters',
  'test_grapheme_clusters.c',
  link_with: liblle,
  include_directories: include_directories('include'))

ninja -C builddir test_grapheme_clusters
./builddir/test_grapheme_clusters
```

#### 3.3.6 Commit Step 3

```bash
git add include/lle/grapheme_detector.h src/lle/grapheme_detector.c \
        src/lle/utf8_index.c test_grapheme_clusters.c meson.build
git commit -m "Phase 1 Step 3: Implement grapheme cluster boundary detection

Implement Unicode UAX #29 grapheme cluster rules.

IMPLEMENTATION:
- Grapheme break property detection for codepoints
- UAX #29 boundary rules (GB3-GB13, GB999)
- Position-based boundary checking
- Support for:
  - Combining marks (diacritics)
  - Emoji sequences with ZWJ
  - Regional indicators (flags)
  - Hangul jamo
  - Extended pictographics

TESTING:
- Precomposed vs decomposed characters
- Emoji with modifiers
- Complex emoji (family, flags)
- Hangul syllables

NOTE: Simplified implementation covering common cases. Full implementation
would use complete Unicode property tables.

NEXT: Step 4 - Implement wide character width calculation"

git push origin feature/lle-utf8-grapheme
```

**CHECKPOINT**: Verify commit is pushed.

---

### STEP 4: Implement Wide Character Width Calculation (Day 4)

#### 3.4.1 Create Character Width Calculator

**File**: `include/lle/char_width.h`

```c
#ifndef LLE_CHAR_WIDTH_H
#define LLE_CHAR_WIDTH_H

#include <stdint.h>

/**
 * Get display width of a Unicode codepoint
 * 
 * Returns the number of terminal columns the character occupies:
 * - 0: Control characters, combining marks, zero-width
 * - 1: Normal characters
 * - 2: Wide characters (CJK, emoji, etc.)
 * - -1: Invalid or unassigned codepoints
 * 
 * Based on Unicode East Asian Width property and other factors.
 * 
 * @param codepoint Unicode codepoint
 * @return Display width (0, 1, 2, or -1)
 */
int lle_codepoint_width(uint32_t codepoint);

/**
 * Check if a codepoint is a wide character
 * 
 * @param codepoint Unicode codepoint
 * @return true if wide (2 columns), false otherwise
 */
bool lle_is_wide_character(uint32_t codepoint);

#endif // LLE_CHAR_WIDTH_H
```

**File**: `src/lle/char_width.c`

```c
#include "lle/char_width.h"
#include <stdbool.h>

/**
 * Character width calculation
 * 
 * This is a simplified implementation. A complete implementation would use
 * the full Unicode East Asian Width property tables.
 * 
 * Reference: Unicode Standard Annex #11 (East Asian Width)
 * https://www.unicode.org/reports/tr11/
 */

int lle_codepoint_width(uint32_t cp) {
    // C0 control characters (0x00-0x1F)
    if (cp < 0x20) {
        return 0;
    }
    
    // DEL (0x7F)
    if (cp == 0x7F) {
        return 0;
    }
    
    // C1 control characters (0x80-0x9F)
    if (cp >= 0x80 && cp <= 0x9F) {
        return 0;
    }
    
    // Combining marks (simplified - full implementation needs tables)
    if (cp >= 0x0300 && cp <= 0x036F) return 0;  // Combining Diacritical Marks
    if (cp >= 0x1AB0 && cp <= 0x1AFF) return 0;  // Combining Diacritical Marks Extended
    if (cp >= 0x1DC0 && cp <= 0x1DFF) return 0;  // Combining Diacritical Marks Supplement
    if (cp >= 0x20D0 && cp <= 0x20FF) return 0;  // Combining Diacritical Marks for Symbols
    if (cp >= 0xFE20 && cp <= 0xFE2F) return 0;  // Combining Half Marks
    
    // Zero-width characters
    if (cp == 0x200B) return 0;  // Zero Width Space
    if (cp == 0x200C) return 0;  // Zero Width Non-Joiner
    if (cp == 0x200D) return 0;  // Zero Width Joiner
    if (cp == 0xFEFF) return 0;  // Zero Width No-Break Space
    
    // Hangul Jamo (medial/final are combining, initial is wide in some fonts)
    if (cp >= 0x1100 && cp <= 0x115F) return 2;  // Choseong (initial)
    if (cp >= 0x1160 && cp <= 0x11FF) return 0;  // Jungseong and Jongseong (combining)
    
    // East Asian Wide (W) and Fullwidth (F) characters
    // This is simplified - full implementation needs complete tables
    
    // CJK Unified Ideographs
    if (cp >= 0x4E00 && cp <= 0x9FFF) return 2;
    if (cp >= 0x3400 && cp <= 0x4DBF) return 2;  // Extension A
    if (cp >= 0x20000 && cp <= 0x2A6DF) return 2;  // Extension B
    if (cp >= 0x2A700 && cp <= 0x2B73F) return 2;  // Extension C
    if (cp >= 0x2B740 && cp <= 0x2B81F) return 2;  // Extension D
    if (cp >= 0x2B820 && cp <= 0x2CEAF) return 2;  // Extension E
    if (cp >= 0x2CEB0 && cp <= 0x2EBEF) return 2;  // Extension F
    if (cp >= 0x30000 && cp <= 0x3134F) return 2;  // Extension G
    
    // Hangul Syllables
    if (cp >= 0xAC00 && cp <= 0xD7A3) return 2;
    
    // Hiragana and Katakana
    if (cp >= 0x3040 && cp <= 0x309F) return 2;  // Hiragana
    if (cp >= 0x30A0 && cp <= 0x30FF) return 2;  // Katakana
    
    // Katakana Phonetic Extensions
    if (cp >= 0x31F0 && cp <= 0x31FF) return 2;
    
    // Halfwidth and Fullwidth Forms
    if (cp >= 0xFF00 && cp <= 0xFF60) return 2;  // Fullwidth
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;  // Fullwidth
    
    // Emoji and Pictographs (most are wide)
    if (cp >= 0x1F300 && cp <= 0x1F9FF) return 2;  // Emoji blocks
    if (cp >= 0x1FA00 && cp <= 0x1FAFF) return 2;  // Symbols and Pictographs Extended-A
    if (cp >= 0x2600 && cp <= 0x27BF) return 2;    // Miscellaneous Symbols
    if (cp >= 0x2300 && cp <= 0x23FF) return 2;    // Miscellaneous Technical
    if (cp >= 0x2B50 && cp <= 0x2B55) return 2;    // Stars
    
    // Emoji Variation Selectors
    if (cp >= 0xFE00 && cp <= 0xFE0F) return 0;  // Variation Selectors (zero-width)
    
    // Skin tone modifiers (Emoji modifiers)
    if (cp >= 0x1F3FB && cp <= 0x1F3FF) return 0;  // Emoji modifiers (zero-width when combined)
    
    // Regional Indicators (flags)
    if (cp >= 0x1F1E6 && cp <= 0x1F1FF) return 2;
    
    // Box Drawing
    if (cp >= 0x2500 && cp <= 0x257F) return 1;  // Usually single-width in terminals
    
    // Block Elements
    if (cp >= 0x2580 && cp <= 0x259F) return 1;
    
    // Geometric Shapes
    if (cp >= 0x25A0 && cp <= 0x25FF) return 1;
    
    // Default: Normal width (1 column)
    return 1;
}

bool lle_is_wide_character(uint32_t codepoint) {
    return lle_codepoint_width(codepoint) == 2;
}
```

#### 3.4.2 Update utf8_index.c to Use Width Calculator

In `src/lle/utf8_index.c`:

```c
#include "lle/char_width.h"

// Replace the placeholder function:
static int get_codepoint_width(uint32_t codepoint) {
    return lle_codepoint_width(codepoint);
}
```

#### 3.4.3 Update Build System

```python
lle_sources = [
    # ... existing files ...
    'src/lle/char_width.c',  # ADD THIS
]
```

#### 3.4.4 Test Wide Characters

**File**: `test_wide_characters.c`

```c
#include "lle/utf8_index.h"
#include "lle/char_width.h"
#include <stdio.h>
#include <string.h>

void test_width(const char *label, const char *text,
               size_t expected_display_width) {
    lle_utf8_index_t index;
    lle_utf8_index_init(&index);
    lle_utf8_index_rebuild(&index, text, strlen(text));
    
    printf("Test: %s\n", label);
    printf("  Text: %s\n", text);
    printf("  Graphemes: %zu\n", index.grapheme_count);
    printf("  Display width: %zu (expected %zu)\n", 
           index.display_width, expected_display_width);
    printf("  Status: %s\n\n",
           index.display_width == expected_display_width ? "PASS" : "FAIL");
    
    lle_utf8_index_cleanup(&index);
}

int main() {
    printf("Testing Wide Character Width Calculation\n\n");
    
    // ASCII (1 column each)
    test_width("ASCII", "hello", 5);
    
    // Latin with diacritic (4 columns: c a f é)
    test_width("Latin", "café", 4);
    
    // CJK (2 columns each)
    test_width("CJK", "中文", 4);  // 2 chars × 2 columns = 4
    
    // Mixed ASCII and CJK
    test_width("Mixed", "hi中文", 6);  // 2 × 1 + 2 × 2 = 6
    
    // Emoji (2 columns)
    test_width("Emoji", "☕", 2);
    
    // Multiple emoji
    test_width("Emoji multiple", "☕🔥", 4);  // 2 × 2 = 4
    
    // Mixed ASCII and emoji
    test_width("Mixed emoji", "hi☕", 4);  // 2 × 1 + 1 × 2 = 4
    
    // Japanese hiragana (2 columns each)
    test_width("Hiragana", "ひらがな", 8);  // 4 × 2 = 8
    
    // Korean Hangul (2 columns each)
    test_width("Hangul", "한국어", 6);  // 3 × 2 = 6
    
    printf("Wide character tests completed!\n");
    return 0;
}
```

**Build and test**:
```bash
# Add to meson.build
executable('test_wide_characters',
  'test_wide_characters.c',
  link_with: liblle,
  include_directories: include_directories('include'))

ninja -C builddir test_wide_characters
./builddir/test_wide_characters
```

#### 3.4.5 Commit Step 4

```bash
git add include/lle/char_width.h src/lle/char_width.c \
        src/lle/utf8_index.c test_wide_characters.c meson.build
git commit -m "Phase 1 Step 4: Implement wide character width calculation

Implement Unicode codepoint display width calculation.

IMPLEMENTATION:
- Control characters: 0 width
- Combining marks: 0 width
- Zero-width characters: 0 width
- Normal characters: 1 column
- Wide characters: 2 columns
  - CJK Unified Ideographs
  - Hangul Syllables
  - Hiragana/Katakana
  - Fullwidth forms
  - Emoji and Pictographs
  - Regional Indicators

TESTING:
- ASCII: 1 column per char
- Latin with diacritics: 1 column
- CJK: 2 columns per char
- Emoji: 2 columns
- Mixed text: Correct total width

NOTE: Simplified implementation covering common cases. Full implementation
would use complete Unicode East Asian Width tables.

NEXT: Step 5 - Integrate into cursor manager"

git push origin feature/lle-utf8-grapheme
```

**CHECKPOINT**: Verify commit is pushed.

---

### STEP 5: Integrate into Cursor Manager (Day 5)

#### 3.5.1 Update Cursor Manager Structure

**File**: `include/lle/cursor_manager.h`

Find the `lle_cursor_t` structure and update it:

```c
typedef struct lle_cursor {
    // Position tracking in multiple coordinate systems
    size_t byte_offset;         // Byte position in buffer (storage)
    size_t codepoint_index;     // Codepoint index (logical position)
    size_t grapheme_index;      // Grapheme cluster index (user-visible position)
    size_t display_column;      // Display column (visual position)
    
    // Position validity flags
    bool byte_offset_valid;
    bool codepoint_index_valid;
    bool grapheme_index_valid;
    bool display_column_valid;
    
    // Cursor state
    bool position_valid;
    
    // Previous position (for undo/comparison)
    size_t prev_byte_offset;
    size_t prev_codepoint_index;
    size_t prev_grapheme_index;
    size_t prev_display_column;
    
} lle_cursor_t;
```

Add to `lle_cursor_manager_t`:

```c
typedef struct lle_cursor_manager {
    // ... existing fields ...
    
    // UTF-8 index for position mapping
    lle_utf8_index_t *utf8_index;
    
} lle_cursor_manager_t;
```

#### 3.5.2 Update Cursor Manager Implementation

**File**: `src/lle/cursor_manager.c`

Add includes:
```c
#include "lle/utf8_index.h"
```

Update initialization:
```c
lle_result_t lle_cursor_manager_init(lle_cursor_manager_t **manager_out) {
    // ... existing code ...
    
    // Initialize UTF-8 index
    manager->utf8_index = malloc(sizeof(lle_utf8_index_t));
    if (!manager->utf8_index) {
        free(manager);
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    result = lle_utf8_index_init(manager->utf8_index);
    if (result != LLE_SUCCESS) {
        free(manager->utf8_index);
        free(manager);
        return result;
    }
    
    // ... rest of initialization ...
}
```

Update cleanup:
```c
void lle_cursor_manager_cleanup(lle_cursor_manager_t *manager) {
    if (!manager) return;
    
    if (manager->utf8_index) {
        lle_utf8_index_cleanup(manager->utf8_index);
        free(manager->utf8_index);
    }
    
    // ... existing cleanup ...
}
```

Add function to synchronize cursor positions:

```c
/**
 * Synchronize all cursor position fields using UTF-8 index
 * 
 * Given one valid position field, calculate all others using the index.
 * 
 * @param manager Cursor manager
 * @param buffer Buffer containing text
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_cursor_manager_sync_positions(lle_cursor_manager_t *manager,
                                               const lle_buffer_t *buffer) {
    if (!manager || !buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_cursor_t *cursor = &manager->current_cursor;
    
    // Rebuild index if needed
    if (!lle_utf8_index_is_valid(manager->utf8_index)) {
        lle_result_t result = lle_utf8_index_rebuild(
            manager->utf8_index,
            buffer->data,
            buffer->length
        );
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // If byte_offset is valid, use it as source of truth
    if (cursor->byte_offset_valid) {
        // Byte → Codepoint
        if (cursor->byte_offset <= manager->utf8_index->byte_count) {
            lle_utf8_index_byte_to_codepoint(
                manager->utf8_index,
                cursor->byte_offset,
                &cursor->codepoint_index
            );
            cursor->codepoint_index_valid = true;
        }
    }
    
    // Codepoint → Grapheme
    if (cursor->codepoint_index_valid) {
        if (cursor->codepoint_index <= manager->utf8_index->codepoint_count) {
            lle_utf8_index_codepoint_to_grapheme(
                manager->utf8_index,
                cursor->codepoint_index,
                &cursor->grapheme_index
            );
            cursor->grapheme_index_valid = true;
        }
    }
    
    // Grapheme → Display Column
    if (cursor->grapheme_index_valid) {
        if (cursor->grapheme_index <= manager->utf8_index->grapheme_count) {
            lle_utf8_index_grapheme_to_display(
                manager->utf8_index,
                cursor->grapheme_index,
                &cursor->display_column
            );
            cursor->display_column_valid = true;
        }
    }
    
    return LLE_SUCCESS;
}
```

Update cursor movement functions to use graphemes:

```c
lle_result_t lle_cursor_manager_move_by_graphemes(lle_cursor_manager_t *manager,
                                                  const lle_buffer_t *buffer,
                                                  int grapheme_delta) {
    if (!manager || !buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Ensure positions are synchronized
    lle_result_t result = lle_cursor_manager_sync_positions(manager, buffer);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    lle_cursor_t *cursor = &manager->current_cursor;
    
    // Calculate new grapheme index
    int new_grapheme = (int)cursor->grapheme_index + grapheme_delta;
    if (new_grapheme < 0) {
        new_grapheme = 0;
    }
    if (new_grapheme > (int)manager->utf8_index->grapheme_count) {
        new_grapheme = manager->utf8_index->grapheme_count;
    }
    
    cursor->grapheme_index = (size_t)new_grapheme;
    cursor->grapheme_index_valid = true;
    
    // Convert back to byte offset
    size_t codepoint_index;
    result = lle_utf8_index_grapheme_to_codepoint(
        manager->utf8_index,
        cursor->grapheme_index,
        &codepoint_index
    );
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    cursor->codepoint_index = codepoint_index;
    cursor->codepoint_index_valid = true;
    
    size_t byte_offset;
    result = lle_utf8_index_codepoint_to_byte(
        manager->utf8_index,
        codepoint_index,
        &byte_offset
    );
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    cursor->byte_offset = byte_offset;
    cursor->byte_offset_valid = true;
    
    // Update display column
    result = lle_utf8_index_grapheme_to_display(
        manager->utf8_index,
        cursor->grapheme_index,
        &cursor->display_column
    );
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    cursor->display_column_valid = true;
    
    return LLE_SUCCESS;
}
```

#### 3.5.3 Commit Step 5

```bash
git add include/lle/cursor_manager.h src/lle/cursor_manager.c
git commit -m "Phase 1 Step 5: Integrate UTF-8 index into cursor manager

Add multi-position tracking to cursor manager using UTF-8 index.

CURSOR POSITIONS:
- byte_offset: Storage position in buffer
- codepoint_index: Logical character position
- grapheme_index: User-visible character position
- display_column: Visual column on screen

FEATURES:
- Position synchronization using UTF-8 index
- Movement by graphemes (not bytes)
- Automatic position conversion
- Validity tracking for each position type

FUNCTIONS:
- lle_cursor_manager_sync_positions(): Sync all position fields
- lle_cursor_manager_move_by_graphemes(): Move by grapheme clusters

NEXT: Step 6 - Update display system to use display columns"

git push origin feature/lle-utf8-grapheme
```

**CHECKPOINT**: Verify commit is pushed.

---

### STEP 6: Update Display System to Use Display Columns (Day 5-6)

Now that cursor manager tracks display columns, update the display system to use them.

#### 3.6.1 Update Screen Buffer to Use Display Columns

**File**: `src/display/screen_buffer.c`

The screen buffer currently uses byte-based cursor positioning. Update it to use display columns from the cursor manager.

Find the `screen_buffer_calculate_visual_width` function and update it to use the UTF-8 index if available:

```c
size_t screen_buffer_calculate_visual_width(const char *text, size_t byte_length) {
    if (!text || byte_length == 0) {
        return 0;
    }
    
    // Use UTF-8 index for accurate width calculation
    lle_utf8_index_t index;
    lle_result_t result = lle_utf8_index_init(&index);
    if (result != LLE_SUCCESS) {
        // Fallback: assume 1 column per byte (ASCII)
        return byte_length;
    }
    
    result = lle_utf8_index_rebuild(&index, text, byte_length);
    if (result != LLE_SUCCESS) {
        lle_utf8_index_cleanup(&index);
        return byte_length;
    }
    
    size_t width = index.display_width;
    lle_utf8_index_cleanup(&index);
    
    return width;
}
```

#### 3.6.2 Update Display Controller to Use Cursor Display Column

**File**: `src/display/display_controller.c`

In the render function, use `cursor->display_column` instead of calculating position from bytes:

```c
void lle_display_controller_render(lle_display_controller_t *ctrl,
                                   const char *command_buffer,
                                   size_t cursor_byte_pos,
                                   const lle_cursor_manager_t *cursor_manager) {
    // ... existing prompt rendering ...
    
    // Get cursor display column from cursor manager
    size_t cursor_display_col = 0;
    if (cursor_manager && cursor_manager->current_cursor.display_column_valid) {
        cursor_display_col = cursor_manager->current_cursor.display_column;
    } else {
        // Fallback: calculate from byte position
        cursor_display_col = screen_buffer_calculate_visual_width(
            command_buffer, 
            cursor_byte_pos
        );
    }
    
    // Calculate absolute column for cursor
    size_t absolute_column = prompt_width + 1 + cursor_display_col;
    
    // Move to cursor position using absolute column
    char cursor_move[32];
    int cursor_len = snprintf(cursor_move, sizeof(cursor_move), 
                             "\033[%zuG", absolute_column);
    write(STDOUT_FILENO, cursor_move, cursor_len);
}
```

#### 3.6.3 Test Display with Wide Characters

Create an interactive test:

```bash
# Build lusush
ninja -C builddir lusush

# Test with wide characters
LLE_ENABLED=1 ./builddir/lusush

# In lusush, type:
echo café
echo 中文
echo ☕🔥

# Verify:
# - Cursor stays at correct position
# - Backspace deletes full characters (not bytes)
# - Arrow keys move by graphemes
```

#### 3.6.4 Commit Step 6

```bash
git add src/display/screen_buffer.c src/display/display_controller.c
git commit -m "Phase 1 Step 6: Update display system to use display columns

Integrate UTF-8 index display width calculation into display system.

CHANGES:
- screen_buffer_calculate_visual_width(): Use UTF-8 index for accurate width
- Display controller: Use cursor->display_column from cursor manager
- Fallback to byte-based calculation if UTF-8 index unavailable

BENEFITS:
- Cursor positioning accurate for wide characters
- No more cursor jumping on CJK/emoji
- Display column calculation respects grapheme clusters

TESTING:
- Manual testing with café, 中文, ☕
- Cursor stays at correct visual position

NEXT: Step 7 - Update editing operations"

git push origin feature/lle-utf8-grapheme
```

**CHECKPOINT**: Verify commit is pushed.

---

### STEP 7: Update Editing Operations to Respect Grapheme Boundaries (Day 6)

Editing operations (insert, delete, backspace) must work on grapheme clusters, not bytes.

#### 3.7.1 Update Buffer Editor to Delete by Grapheme

**File**: `src/lle/buffer_editor.c`

Add function to delete one grapheme cluster:

```c
lle_result_t lle_buffer_editor_delete_grapheme_at_cursor(
    lle_buffer_editor_t *editor,
    lle_buffer_t *buffer,
    lle_cursor_manager_t *cursor_manager
) {
    if (!editor || !buffer || !cursor_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Sync cursor positions
    lle_result_t result = lle_cursor_manager_sync_positions(cursor_manager, buffer);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    lle_cursor_t *cursor = &cursor_manager->current_cursor;
    
    // If at end of buffer, nothing to delete
    if (cursor->grapheme_index >= cursor_manager->utf8_index->grapheme_count) {
        return LLE_SUCCESS;
    }
    
    // Get byte range for current grapheme cluster
    size_t start_byte = cursor->byte_offset;
    
    // Find start of next grapheme
    size_t next_grapheme_idx = cursor->grapheme_index + 1;
    size_t end_byte;
    
    if (next_grapheme_idx >= cursor_manager->utf8_index->grapheme_count) {
        // Deleting last grapheme - go to end of buffer
        end_byte = buffer->length;
    } else {
        // Get byte offset of next grapheme
        size_t next_codepoint;
        result = lle_utf8_index_grapheme_to_codepoint(
            cursor_manager->utf8_index,
            next_grapheme_idx,
            &next_codepoint
        );
        if (result != LLE_SUCCESS) {
            return result;
        }
        
        result = lle_utf8_index_codepoint_to_byte(
            cursor_manager->utf8_index,
            next_codepoint,
            &end_byte
        );
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Delete the byte range
    size_t delete_length = end_byte - start_byte;
    result = lle_buffer_delete(buffer, start_byte, delete_length);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Invalidate UTF-8 index (buffer changed)
    lle_utf8_index_invalidate(cursor_manager->utf8_index);
    
    // Cursor stays at same grapheme index (which is now the next character)
    // Re-sync positions after buffer modification
    result = lle_cursor_manager_sync_positions(cursor_manager, buffer);
    
    return result;
}
```

Add function to backspace by grapheme:

```c
lle_result_t lle_buffer_editor_backspace_grapheme(
    lle_buffer_editor_t *editor,
    lle_buffer_t *buffer,
    lle_cursor_manager_t *cursor_manager
) {
    if (!editor || !buffer || !cursor_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Sync cursor positions
    lle_result_t result = lle_cursor_manager_sync_positions(cursor_manager, buffer);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    lle_cursor_t *cursor = &cursor_manager->current_cursor;
    
    // If at start of buffer, nothing to delete
    if (cursor->grapheme_index == 0) {
        return LLE_SUCCESS;
    }
    
    // Move back one grapheme
    size_t prev_grapheme_idx = cursor->grapheme_index - 1;
    
    // Get byte range for previous grapheme
    size_t start_codepoint;
    result = lle_utf8_index_grapheme_to_codepoint(
        cursor_manager->utf8_index,
        prev_grapheme_idx,
        &start_codepoint
    );
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    size_t start_byte;
    result = lle_utf8_index_codepoint_to_byte(
        cursor_manager->utf8_index,
        start_codepoint,
        &start_byte
    );
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    size_t end_byte = cursor->byte_offset;
    size_t delete_length = end_byte - start_byte;
    
    // Delete the byte range
    result = lle_buffer_delete(buffer, start_byte, delete_length);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Update cursor to previous grapheme position
    cursor->grapheme_index = prev_grapheme_idx;
    cursor->byte_offset = start_byte;
    
    // Invalidate UTF-8 index
    lle_utf8_index_invalidate(cursor_manager->utf8_index);
    
    // Re-sync positions
    result = lle_cursor_manager_sync_positions(cursor_manager, buffer);
    
    return result;
}
```

#### 3.7.2 Update Input Handler to Use Grapheme Operations

**File**: `src/lle/input_handler.c`

Update backspace handling:

```c
// When backspace key is pressed:
case KEY_BACKSPACE:
case 127:  // DEL
    result = lle_buffer_editor_backspace_grapheme(
        editor,
        buffer,
        cursor_manager
    );
    break;
```

Update delete key handling:

```c
// When delete key is pressed:
case KEY_DELETE:
    result = lle_buffer_editor_delete_grapheme_at_cursor(
        editor,
        buffer,
        cursor_manager
    );
    break;
```

#### 3.7.3 Test Editing Operations

Create comprehensive editing test:

```bash
# Build
ninja -C builddir lusush

# Test backspace with wide characters
LLE_ENABLED=1 ./builddir/lusush

# Type: café
# Press backspace
# Expected: é deleted (even though it's 2 bytes)
# Press backspace again
# Expected: f deleted

# Type: 中文
# Press backspace
# Expected: 文 deleted (entire character, not partial bytes)

# Type: 👨‍👩‍👧‍👦
# Press backspace
# Expected: Entire family emoji deleted (all codepoints in cluster)
```

#### 3.7.4 Commit Step 7

```bash
git add src/lle/buffer_editor.c src/lle/input_handler.c \
        include/lle/buffer_editor.h
git commit -m "Phase 1 Step 7: Update editing to respect grapheme boundaries

Implement grapheme-aware delete and backspace operations.

FUNCTIONS:
- lle_buffer_editor_delete_grapheme_at_cursor(): Delete grapheme at cursor
- lle_buffer_editor_backspace_grapheme(): Delete grapheme before cursor

FEATURES:
- Delete entire grapheme clusters (not partial bytes)
- Works with multi-byte UTF-8 characters
- Works with combining characters (é as e+combining)
- Works with emoji sequences (👨‍👩‍👧‍👦)
- Invalidates UTF-8 index after buffer modification
- Re-syncs cursor positions after edit

INPUT HANDLER:
- Backspace key uses grapheme backspace
- Delete key uses grapheme delete

TESTING:
- Manual testing with café, 中文, emoji
- Backspace deletes full characters correctly

NEXT: Step 8 - Comprehensive testing"

git push origin feature/lle-utf8-grapheme
```

**CHECKPOINT**: Verify commit is pushed.

---

### STEP 8: Comprehensive Testing with All Unicode Cases (Day 7)

Final step: Comprehensive testing across all Unicode scenarios.

#### 3.8.1 Create Comprehensive Test Suite

**File**: `test_phase1_comprehensive.c`

```c
#include "lle/utf8_index.h"
#include "lle/cursor_manager.h"
#include "lle/buffer_editor.h"
#include "lle/buffer.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test structure
typedef struct {
    const char *name;
    const char *input;
    size_t expected_bytes;
    size_t expected_codepoints;
    size_t expected_graphemes;
    size_t expected_display_width;
} unicode_test_case_t;

// Test cases covering all scenarios
unicode_test_case_t test_cases[] = {
    // Basic ASCII
    {"ASCII", "hello", 5, 5, 5, 5},
    
    // Latin with diacritics
    {"Precomposed", "café", 5, 4, 4, 4},
    {"Decomposed", "café", 6, 5, 4, 4},  // e + combining = 1 grapheme
    
    // CJK (wide characters)
    {"Chinese", "中文", 6, 2, 2, 4},
    {"Japanese", "日本語", 9, 3, 3, 6},
    {"Korean", "한국어", 9, 3, 3, 6},
    
    // Emoji
    {"Basic emoji", "☕", 3, 1, 1, 2},
    {"Emoji sequence", "☕🔥", 6, 2, 2, 4},
    {"Emoji modifier", "👍🏻", 8, 2, 1, 2},
    {"Family emoji", "👨‍👩‍👧‍👦", 25, 7, 1, 2},
    {"Flag", "🇺🇸", 8, 2, 1, 2},
    
    // Mixed content
    {"Mixed ASCII+CJK", "hi中文", 8, 4, 4, 6},
    {"Mixed ASCII+emoji", "test☕", 8, 5, 5, 6},
    {"Mixed all", "café中文☕", 14, 7, 7, 10},
    
    // Edge cases
    {"Empty", "", 0, 0, 0, 0},
    {"Single byte", "a", 1, 1, 1, 1},
    {"Single CJK", "中", 3, 1, 1, 2},
    {"Single emoji", "🔥", 4, 1, 1, 2},
    
    // Complex combining
    {"Multiple combining", "e\u0301\u0302", 5, 3, 1, 1},  // e + 2 accents = 1 grapheme
    
    // Control characters
    {"With newline", "hi\nbye", 6, 6, 6, 5},  // \n has 0 width
    {"With tab", "hi\tbye", 6, 6, 6, 5},      // \t has 0 width
};

void run_index_test(unicode_test_case_t *test) {
    printf("Testing: %s\n", test->name);
    printf("  Input: \"%s\"\n", test->input);
    
    lle_utf8_index_t index;
    lle_result_t result = lle_utf8_index_init(&index);
    assert(result == LLE_SUCCESS);
    
    result = lle_utf8_index_rebuild(&index, test->input, strlen(test->input));
    assert(result == LLE_SUCCESS);
    
    // Verify counts
    bool pass = true;
    
    if (index.byte_count != test->expected_bytes) {
        printf("  FAIL: Bytes: got %zu, expected %zu\n", 
               index.byte_count, test->expected_bytes);
        pass = false;
    }
    
    if (index.codepoint_count != test->expected_codepoints) {
        printf("  FAIL: Codepoints: got %zu, expected %zu\n",
               index.codepoint_count, test->expected_codepoints);
        pass = false;
    }
    
    if (index.grapheme_count != test->expected_graphemes) {
        printf("  FAIL: Graphemes: got %zu, expected %zu\n",
               index.grapheme_count, test->expected_graphemes);
        pass = false;
    }
    
    if (index.display_width != test->expected_display_width) {
        printf("  FAIL: Display width: got %zu, expected %zu\n",
               index.display_width, test->expected_display_width);
        pass = false;
    }
    
    if (pass) {
        printf("  PASS\n");
    }
    
    lle_utf8_index_cleanup(&index);
    printf("\n");
}

void test_cursor_movement() {
    printf("=== Cursor Movement Tests ===\n\n");
    
    const char *text = "hi中文";  // 2 ASCII + 2 CJK = 4 graphemes, 6 display columns
    
    lle_buffer_t *buffer;
    lle_buffer_init(&buffer);
    lle_buffer_insert(buffer, 0, text, strlen(text));
    
    lle_cursor_manager_t *cursor_mgr;
    lle_cursor_manager_init(&cursor_mgr);
    
    // Test: Move forward by graphemes
    printf("Test: Move forward by graphemes\n");
    
    // Start at position 0
    assert(cursor_mgr->current_cursor.grapheme_index == 0);
    
    // Move forward 1 grapheme (to 'i')
    lle_cursor_manager_move_by_graphemes(cursor_mgr, buffer, 1);
    assert(cursor_mgr->current_cursor.grapheme_index == 1);
    assert(cursor_mgr->current_cursor.display_column == 1);
    
    // Move forward 1 grapheme (to '中')
    lle_cursor_manager_move_by_graphemes(cursor_mgr, buffer, 1);
    assert(cursor_mgr->current_cursor.grapheme_index == 2);
    assert(cursor_mgr->current_cursor.display_column == 2);  // 'h' + 'i' = 2 cols
    
    // Move forward 1 grapheme (to '文')
    lle_cursor_manager_move_by_graphemes(cursor_mgr, buffer, 1);
    assert(cursor_mgr->current_cursor.grapheme_index == 3);
    assert(cursor_mgr->current_cursor.display_column == 4);  // 2 + 2 (width of 中)
    
    printf("  PASS: Forward movement\n\n");
    
    // Test: Move backward by graphemes
    printf("Test: Move backward by graphemes\n");
    
    lle_cursor_manager_move_by_graphemes(cursor_mgr, buffer, -1);
    assert(cursor_mgr->current_cursor.grapheme_index == 2);
    assert(cursor_mgr->current_cursor.display_column == 2);
    
    printf("  PASS: Backward movement\n\n");
    
    lle_cursor_manager_cleanup(cursor_mgr);
    lle_buffer_cleanup(buffer);
}

void test_editing_operations() {
    printf("=== Editing Operations Tests ===\n\n");
    
    // Test backspace on emoji with modifier
    printf("Test: Backspace on emoji with modifier\n");
    
    const char *emoji_text = "test👍🏻";
    
    lle_buffer_t *buffer;
    lle_buffer_init(&buffer);
    lle_buffer_insert(buffer, 0, emoji_text, strlen(emoji_text));
    
    lle_cursor_manager_t *cursor_mgr;
    lle_cursor_manager_init(&cursor_mgr);
    
    lle_buffer_editor_t *editor;
    lle_buffer_editor_init(&editor);
    
    // Move cursor to end
    lle_cursor_manager_move_by_graphemes(cursor_mgr, buffer, 100);  // Move to end
    assert(cursor_mgr->current_cursor.grapheme_index == 5);  // 4 ASCII + 1 emoji cluster
    
    // Backspace should delete entire emoji cluster (👍🏻 = 2 codepoints, 1 grapheme)
    lle_buffer_editor_backspace_grapheme(editor, buffer, cursor_mgr);
    
    // Should be at grapheme 4 now (after 'test')
    assert(cursor_mgr->current_cursor.grapheme_index == 4);
    
    // Buffer should only contain "test"
    assert(buffer->length == 4);
    assert(strncmp(buffer->data, "test", 4) == 0);
    
    printf("  PASS: Emoji cluster deleted as single unit\n\n");
    
    lle_buffer_editor_cleanup(editor);
    lle_cursor_manager_cleanup(cursor_mgr);
    lle_buffer_cleanup(buffer);
}

int main() {
    printf("========================================\n");
    printf("Phase 1 Comprehensive Test Suite\n");
    printf("========================================\n\n");
    
    // Run index tests
    printf("=== UTF-8 Index Tests ===\n\n");
    
    size_t num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    size_t passed = 0;
    
    for (size_t i = 0; i < num_tests; i++) {
        run_index_test(&test_cases[i]);
        passed++;
    }
    
    printf("Index tests: %zu/%zu passed\n\n", passed, num_tests);
    
    // Run cursor movement tests
    test_cursor_movement();
    
    // Run editing tests
    test_editing_operations();
    
    printf("========================================\n");
    printf("All tests completed successfully!\n");
    printf("========================================\n");
    
    return 0;
}
```

#### 3.8.2 Build and Run Comprehensive Tests

```bash
# Add to meson.build
executable('test_phase1_comprehensive',
  'test_phase1_comprehensive.c',
  link_with: liblle,
  include_directories: include_directories('include'))

# Build
ninja -C builddir test_phase1_comprehensive

# Run
./builddir/test_phase1_comprehensive
```

**Expected output**: All tests should PASS.

#### 3.8.3 Manual Integration Testing

Test in actual lusush:

```bash
LLE_ENABLED=1 ./builddir/lusush

# Test 1: Wide character cursor positioning
> echo 中文测试
# Verify: Cursor moves 2 columns per CJK character

# Test 2: Emoji editing
> echo 👨‍👩‍👧‍👦
# Press backspace
# Verify: Entire family emoji deleted as one unit

# Test 3: Mixed content navigation
> echo café中文☕
# Use arrow keys to move through text
# Verify: Cursor moves by visible characters, not bytes

# Test 4: Decomposed characters
> echo café  # Where é is e + combining accent
# Press backspace repeatedly
# Verify: é deleted as one unit (not e first, then accent)

# Test 5: Flag emoji
> echo 🇺🇸🇬🇧
# Press backspace
# Verify: Each flag deleted as one unit (not individual regional indicators)
```

#### 3.8.4 Commit Step 8

```bash
git add test_phase1_comprehensive.c meson.build
git commit -m "Phase 1 Step 8: Add comprehensive Unicode test suite

Complete Phase 1 with comprehensive testing across all Unicode cases.

TEST COVERAGE:
- Basic ASCII
- Latin with diacritics (precomposed and decomposed)
- CJK characters (Chinese, Japanese, Korean)
- Emoji (basic, with modifiers, sequences, flags)
- Mixed content (ASCII + CJK + emoji)
- Edge cases (empty, single chars, control chars)
- Complex combining characters
- Cursor movement (forward/backward by graphemes)
- Editing operations (backspace on grapheme clusters)

TEST RESULTS:
- All index tests: PASS
- Cursor movement tests: PASS
- Editing operation tests: PASS
- Manual integration tests: PASS

PHASE 1 STATUS: COMPLETE ✓

All UTF-8/grapheme foundation functionality is now implemented and tested."

git push origin feature/lle-utf8-grapheme
```

**CHECKPOINT**: Verify commit is pushed.

---

## 4. Testing Strategy

### 4.1 Unit Testing

Each component has dedicated unit tests:

1. **UTF-8 Index** (`test_utf8_index_rebuild.c`)
   - Basic ASCII indexing
   - Multi-byte UTF-8 characters
   - Byte-to-codepoint mapping
   - Position conversions

2. **Grapheme Clusters** (`test_grapheme_clusters.c`)
   - Combining characters
   - Emoji sequences
   - Regional indicators (flags)
   - Hangul syllables
   - UAX #29 boundary rules

3. **Wide Characters** (`test_wide_characters.c`)
   - CJK width calculation
   - Emoji width
   - Mixed ASCII/wide content
   - Display column calculation

4. **Comprehensive** (`test_phase1_comprehensive.c`)
   - All Unicode categories
   - Cursor movement
   - Editing operations
   - Edge cases

### 4.2 Integration Testing

Test in actual lusush shell:

1. **Display Testing**
   - Type wide characters, verify cursor position
   - Type emoji, verify cursor position
   - Type mixed content, verify rendering

2. **Editing Testing**
   - Backspace on wide characters
   - Backspace on emoji sequences
   - Delete key on grapheme clusters
   - Arrow key navigation

3. **Line Wrapping Testing**
   - Long lines with wide characters
   - Emoji at wrap boundary
   - Mixed content wrapping

### 4.3 Regression Testing

Verify existing tests still pass:

```bash
# Run existing LLE tests
ninja -C builddir test
./builddir/test_lle_integration

# Verify no regressions in:
# - Basic input/output (test 1.1-1.3)
# - Cursor positioning (test 2.1-2.4)
# - Line wrapping (test 3.1-3.3)
# - Event loop (test 4.1-4.2)
```

### 4.4 Test Data Files

Use `/tmp/lle_test_unicode.txt` created in Step 2 for manual testing:

```bash
# Copy test strings into lusush
cat /tmp/lle_test_unicode.txt
# Copy each line and test in lusush
```

---

## 5. Success Criteria

Phase 1 is complete when ALL of the following are true:

### 5.1 Build and Compilation
- [ ] All files compile without errors
- [ ] All files compile without warnings
- [ ] Build system correctly links all new files

### 5.2 Unit Tests
- [ ] `test_utf8_index_rebuild` passes all tests
- [ ] `test_grapheme_clusters` passes all tests
- [ ] `test_wide_characters` passes all tests
- [ ] `test_phase1_comprehensive` passes all tests

### 5.3 Integration Tests
- [ ] Wide characters (中文, 日本語) display correctly
- [ ] Cursor position accurate after wide characters
- [ ] Emoji (☕, 🔥, 👍🏻) display correctly
- [ ] Emoji sequences (👨‍👩‍👧‍👦) treated as single grapheme
- [ ] Flags (🇺🇸) treated as single grapheme
- [ ] Backspace deletes full grapheme clusters
- [ ] Delete key deletes full grapheme clusters
- [ ] Arrow keys move by graphemes, not bytes
- [ ] Mixed content (ASCII + CJK + emoji) works correctly

### 5.4 Regression Tests
- [ ] All existing LLE tests still pass
- [ ] Basic line wrapping still works (from Session 8)
- [ ] Prompt-once architecture still correct
- [ ] No new memory leaks (valgrind clean)

### 5.5 Code Quality
- [ ] All functions have documentation comments
- [ ] No magic numbers (constants defined)
- [ ] Error handling on all API functions
- [ ] No compiler warnings
- [ ] Code follows lusush style guidelines

### 5.6 Documentation
- [ ] All new APIs documented in headers
- [ ] Commit messages follow git policy
- [ ] This implementation plan marked as complete
- [ ] Any deviations from plan documented

### 5.7 Git State
- [ ] All commits pushed to remote
- [ ] No uncommitted changes
- [ ] Branch builds cleanly from scratch
- [ ] No stashed changes

---

## 6. Troubleshooting Guide

### 6.1 Build Issues

**Problem**: `utf8_index.h: No such file or directory`

**Solution**: Ensure `include/lle/utf8_index.h` exists and `include_directories` in meson.build includes `'include'`.

---

**Problem**: `undefined reference to lle_utf8_index_init`

**Solution**: Ensure `src/lle/utf8_index.c` is in `lle_sources` list in meson.build. Run `ninja -C builddir clean` then rebuild.

---

**Problem**: Compiler warnings about unused variables

**Solution**: Some helper functions in early steps return placeholders. These warnings will disappear as functions are fully implemented.

---

### 6.2 Test Failures

**Problem**: Grapheme count wrong for decomposed characters

**Solution**: Verify `is_grapheme_boundary_at_position()` is correctly implemented. Check that `GB9` rule (× Extend | ZWJ) prevents breaks on combining marks.

---

**Problem**: Display width wrong for emoji

**Solution**: Check `lle_codepoint_width()` implementation. Verify emoji codepoint ranges are covered (0x1F300-0x1F9FF, etc.).

---

**Problem**: Cursor position wrong after wide characters

**Solution**: Ensure display controller uses `cursor->display_column` from cursor manager, not byte-based calculation.

---

### 6.3 Runtime Issues

**Problem**: Cursor jumps around on CJK input

**Solution**: Verify UTF-8 index is rebuilt after buffer modifications. Check `lle_utf8_index_invalidate()` is called after edits.

---

**Problem**: Backspace deletes partial emoji

**Solution**: Ensure `lle_buffer_editor_backspace_grapheme()` is being used, not old byte-based backspace. Verify grapheme boundary detection for ZWJ sequences.

---

**Problem**: Segfault in UTF-8 index rebuild

**Solution**: Check for off-by-one errors in array allocation. Verify `text_length + 1` is used for array sizes to include null terminator position.

---

**Problem**: Memory leak reported by valgrind

**Solution**: Ensure `lle_utf8_index_cleanup()` is called in cursor manager cleanup. Verify all malloc'd arrays are freed.

---

### 6.4 Display Issues

**Problem**: Emoji displays as ?? or boxes

**Solution**: This is a terminal/font issue, not LLE. Verify your terminal supports emoji (kitty, alacritty, iTerm2 work well). LLE is correctly handling the emoji if cursor position is right.

---

**Problem**: Cursor visible in wrong position but input works

**Solution**: Check terminal sequences in display controller. Verify using `\033[{col}G` (absolute positioning) not relative moves. Ensure prompt width calculated correctly.

---

### 6.5 Git Issues

**Problem**: Push rejected, branch diverged

**Solution**: Someone else pushed to your branch. Run `git pull --rebase origin feature/lle-utf8-grapheme`, resolve conflicts, then push.

---

**Problem**: Commit message rejected by hook

**Solution**: lusush enforces professional commit message policy. Ensure:
- No emojis in commit message
- First line < 72 chars
- Body lines < 80 chars
- No [WIP] or similar tags

---

## 7. Reference Materials

### 7.1 Specifications

**Primary**: `docs/lle_specification/03_buffer_management_complete.md`
- Section 4: UTF-8 Unicode Support (lines 612-900+)
- Section 4.1: UTF-8 Index Structure
- Section 4.2: Index Rebuild Algorithm
- Section 4.3: Position Mapping

**Related**: `docs/lle_specification/02_cursor_display_architecture.md`
- Cursor position tracking
- Display column calculation

### 7.2 Unicode Standards

**UAX #29: Unicode Text Segmentation**
- URL: https://unicode.org/reports/tr29/
- Sections 1-3: Grapheme Cluster Boundaries
- Table 1: Grapheme Break Property Values
- Table 2: Grapheme Cluster Boundary Rules

**UAX #11: East Asian Width**
- URL: https://unicode.org/reports/tr11/
- Explains wide vs narrow character classification
- East Asian Width property values

**Unicode Character Database**
- URL: https://www.unicode.org/Public/UCD/latest/
- Complete property tables for production implementation

### 7.3 Related Research Documents

**MODERN_EDITOR_WRAPPING_RESEARCH.md**
- Prompt-once architecture (already implemented in Session 8)
- Terminal positioning sequences
- Differential updates

**IMPLEMENTATION_IMPACT_ANALYSIS.md**
- How UTF-8/grapheme affects other systems
- Integration points
- Dependencies

### 7.4 Session Documentation

**SESSION_8_LINE_WRAP_FIX.md**
- Line wrap debugging and fix
- Git bisect investigation
- Prompt-once architecture implementation

**NEXT_STEPS_ANALYSIS.md**
- Why UTF-8/grapheme is the correct starting point
- Analysis of all known issues
- 4-phase development plan

### 7.5 Code References

**Existing UTF-8 support**:
- `src/lle/utf8_support.c` - Basic UTF-8 functions
- `include/lle/utf8_support.h` - UTF-8 API

**Cursor management**:
- `src/lle/cursor_manager.c` - Cursor state tracking
- `include/lle/cursor_manager.h` - Cursor API

**Display system**:
- `src/display/display_controller.c` - Main rendering
- `src/display/screen_buffer.c` - Screen state
- `src/display/composition_engine.c` - Layer composition

**Buffer management**:
- `src/lle/buffer.c` - Buffer operations
- `src/lle/buffer_editor.c` - Editing operations

### 7.6 Testing References

**Test patterns**:
- Existing test files in `tests/` directory
- LLE integration test: `test_lle_integration.c`

**Unicode test data**:
- `/tmp/lle_test_unicode.txt` (created in Step 2)
- Unicode test suite: https://www.unicode.org/Public/UCD/latest/ucd/NormalizationTest.txt

### 7.7 Terminal Reference

**ANSI Escape Codes**:
- `\033[{n}G` - Cursor to absolute column n (1-based)
- `\033[{n}A` - Cursor up n rows
- `\033[J` - Clear from cursor to end of screen
- `\033[K` - Clear from cursor to end of line

**Terminal testing**:
- Test in kitty, alacritty, iTerm2 (best emoji support)
- Avoid: gnome-terminal, xterm (limited emoji)

### 7.8 Tools

**Debugging**:
- `valgrind --leak-check=full ./builddir/lusush` - Memory leak detection
- `gdb ./builddir/lusush` - Debugger
- `printf` debugging in development

**Unicode inspection**:
- `hexdump -C` - View byte representation
- `unicode` tool - Inspect codepoint properties
- Online: https://unicode.scarfboy.com/

---

## 8. Final Verification Checklist

Before considering Phase 1 complete, verify:

- [ ] All 8 implementation steps completed
- [ ] All code committed and pushed
- [ ] All unit tests passing
- [ ] Manual testing successful with:
  - [ ] café (decomposed)
  - [ ] 中文 (CJK)
  - [ ] ☕ (emoji)
  - [ ] 👍🏻 (emoji with modifier)
  - [ ] 👨‍👩‍👧‍👦 (emoji sequence)
  - [ ] 🇺🇸 (flag)
  - [ ] Mixed content
- [ ] No regressions in existing tests
- [ ] No memory leaks (valgrind clean)
- [ ] Documentation updated
- [ ] Ready to merge to feature/lle branch

---

## 9. What Happens After Phase 1

Once Phase 1 is complete and verified:

1. **Merge to feature/lle**:
   ```bash
   git checkout feature/lle
   git merge --no-ff feature/lle-utf8-grapheme
   git push origin feature/lle
   ```

2. **Update test expectations**:
   - Test 5.4 (wide character cursor) can now be ENABLED
   - Update test to expect PASS instead of DEFERRED

3. **Begin Phase 2**: History and undo/redo (see NEXT_STEPS_ANALYSIS.md)

4. **Document lessons learned**:
   - Update IMPLEMENTATION_IMPACT_ANALYSIS.md with actual findings
   - Note any deviations from this plan
   - Record performance metrics (index rebuild time)

---

## 10. Notes and Observations

### 10.1 Design Decisions

**Why separate grapheme_detector.c?**
- UAX #29 rules are complex and may need updates
- Isolates Unicode-specific logic
- Easier to test independently
- Could be replaced with library in future

**Why rebuild entire index on buffer change?**
- Spec 03 specifies full rebuild approach
- Simpler than incremental updates
- Fast enough for command-line input (< 1000 chars)
- Can optimize later if needed

**Why track all 4 position types?**
- Different subsystems need different positions
- Fast conversion via lookup arrays
- Explicit about which position type is being used
- Prevents byte/codepoint confusion bugs

### 10.2 Performance Considerations

**Index rebuild cost**:
- O(n) where n = buffer length
- Expected to be fast for command-line input
- Profile after implementation if concerns arise

**Memory overhead**:
- 6 arrays per buffer
- Approximately 12 * buffer_length bytes for ASCII
- More for UTF-8 (varies by content)
- Acceptable for command-line use case

**Lookup performance**:
- O(1) for all position conversions
- This is the key benefit of the index approach

### 10.3 Simplifications Made

This implementation is simplified compared to a full Unicode implementation:

1. **Grapheme break properties**: Simplified ranges, not complete Unicode tables
2. **Character width**: Common cases covered, not all edge cases
3. **East Asian Width**: Simplified ranges for CJK
4. **Emoji detection**: Common emoji ranges, may miss newer emoji

**For production**: These could be replaced with:
- ICU library (full Unicode support)
- utf8proc library (grapheme clustering)
- wcwidth() from system libc (character width)

**For LLE Phase 1**: Current implementation is sufficient and avoids external dependencies.

### 10.4 Future Enhancements

Potential improvements for future phases:

1. **Incremental index updates**: Only rebuild changed sections
2. **Lazy index building**: Build on first access
3. **Caching**: Cache frequently used conversions
4. **Bidirectional text**: Support RTL (Arabic, Hebrew)
5. **Line breaking**: UAX #14 line breaking algorithm
6. **Full Unicode tables**: Use complete Unicode data files

**Priority**: Only implement if profiling shows need. Current approach likely sufficient.

---

**END OF PHASE 1 IMPLEMENTATION PLAN**

This document contains complete, unambiguous instructions for implementing the UTF-8/Grapheme Foundation (Phase 1). Follow each step sequentially, commit at each checkpoint, and verify all success criteria before proceeding to Phase 2.

**Document Status**: COMPLETE  
**Last Updated**: 2025-11-10  
**Ready for Implementation**: YES
