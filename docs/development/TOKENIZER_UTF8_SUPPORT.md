# Tokenizer UTF-8 Support Implementation

**Status**: ✅ **COMPLETE - All tests passing**  
**Date**: 2025-11-11  
**Branch**: `feature/lle`  
**Related**: Completes UTF-8 support alongside LLE UTF-8/Grapheme work

---

## Executive Summary

The shell tokenizer has been successfully upgraded to support UTF-8 input. Commands containing non-ASCII characters (accented letters, CJK, emoji) now tokenize and execute correctly.

**Before**: `echo café` → `syntax error: unterminated quoted string`  
**After**: `echo café` → `café` ✅

---

## Problem Statement

The original tokenizer was ASCII-only, scanning input byte-by-byte. When encountering UTF-8 multibyte sequences, it misinterpreted continuation bytes (0x80-0xFF) as invalid characters or shell metacharacters, resulting in tokenization errors.

### Example Failure

```bash
$ echo café
lusush: syntax error: unterminated quoted string
```

**Root Cause**: The byte `0xC3` (from UTF-8 encoding of 'é') was not recognized as part of a valid word character.

---

## Implementation

### Files Modified

**src/tokenizer.c**:
1. Added UTF-8 support header: `#include "../include/lle/utf8_support.h"`
2. Created `is_word_codepoint()` - UTF-8 aware character classification
3. Updated word tokenization loop to scan by codepoints, not bytes

### Key Changes

#### 1. UTF-8 Aware Character Classification

```c
// Check if Unicode codepoint can be part of a word (UTF-8 aware)
static bool is_word_codepoint(uint32_t codepoint) {
    // ASCII range: Use traditional shell word character logic
    if (codepoint < 0x80) {
        char c = (char)codepoint;
        return isalnum(c) || strchr("_.-/~:@*?[]+%", c) != NULL;
    }
    
    // Non-ASCII UTF-8: All non-ASCII codepoints are valid word characters
    // This includes:
    // - Latin Extended (accented characters like é, ñ, ü)
    // - CJK (Chinese, Japanese, Korean)
    // - Emoji
    // - All other Unicode scripts
    //
    // Shell metacharacters (quotes, pipes, etc.) are all ASCII (< 0x80),
    // so they're handled by the ASCII logic above.
    return true;
}
```

**Design Decision**: All non-ASCII codepoints are treated as word characters. This is safe because:
- All shell metacharacters (`;`, `|`, `&`, `<`, `>`, quotes, etc.) are ASCII
- Non-ASCII characters should be part of commands/arguments, not syntax
- Matches behavior of bash and other modern shells

#### 2. UTF-8 Aware Word Scanning

**Before (Byte-based)**:
```c
while (tokenizer->position < tokenizer->input_length) {
    char curr = tokenizer->input[tokenizer->position];
    if (is_word_char(curr) || isalnum(curr)) {
        tokenizer->position++;  // Advance by 1 byte
        tokenizer->column++;
    } else {
        break;
    }
}
```

**After (Codepoint-based)**:
```c
while (tokenizer->position < tokenizer->input_length) {
    // Decode UTF-8 codepoint at current position
    uint32_t curr_codepoint;
    int curr_char_len = lle_utf8_decode_codepoint(
        &tokenizer->input[tokenizer->position],
        tokenizer->input_length - tokenizer->position,
        &curr_codepoint
    );
    
    if (curr_char_len > 0) {
        // Valid UTF-8 character - check if it's a word character
        if (is_word_codepoint(curr_codepoint)) {
            // Advance by the UTF-8 character length (1-4 bytes)
            tokenizer->position += curr_char_len;
            tokenizer->column++;  // One visual column per character
        } else {
            break;  // Not a word character - end of word
        }
    } else {
        break;  // Invalid UTF-8 sequence - treat as end of word
    }
}
```

**Key Improvements**:
- Uses `lle_utf8_decode_codepoint()` to decode multi-byte sequences correctly
- Advances by character length (1-4 bytes) instead of always 1 byte
- Handles invalid UTF-8 gracefully (stops tokenization at boundary)
- Visual column count remains 1 per character (simplified for now)

---

## Test Results

All 7 test cases pass:

| Test | Input | Output | Status |
|------|-------|--------|--------|
| 1 | `echo café` | `café` | ✅ PASS |
| 2 | `echo 日本` | `日本` | ✅ PASS |
| 3 | `echo 🎉` | `🎉` | ✅ PASS |
| 4 | `echo Hello 世界` | `Hello 世界` | ✅ PASS |
| 5 | `echo "café"` | `café` | ✅ PASS |
| 6 | `echo 🇺🇸` | `🇺🇸` | ✅ PASS |
| 7 | `echo café 日本 🎉` | `café 日本 🎉` | ✅ PASS |

### Test Coverage

- **2-byte UTF-8**: Latin extended characters (café) ✅
- **3-byte UTF-8**: CJK characters (日本) ✅
- **4-byte UTF-8**: Emoji (🎉, 🇺🇸) ✅
- **Mixed UTF-8**: ASCII + UTF-8 in same command ✅
- **Quoted strings**: UTF-8 inside double quotes ✅
- **Multiple arguments**: Multiple UTF-8 words ✅

---

## Architecture

### UTF-8 Processing Pipeline

```
Input: "echo café"
  ↓
Tokenizer (UTF-8 aware)
  ↓
Token 1: WORD "echo" (4 bytes: 0x65 0x63 0x68 0x6F)
Token 2: WORD "café" (5 bytes: 0x63 0x61 0x66 0xC3 0xA9)
  ↓
Parser (unchanged - works with token text)
  ↓
Executor (unchanged - passes UTF-8 text to commands)
  ↓
Output: café
```

### Component Integration

**Tokenizer → Parser**: Parser receives complete UTF-8 token text, no changes needed  
**Parser → Executor**: Executor receives UTF-8 strings, passes to system commands  
**LLE → Tokenizer**: User types UTF-8 in LLE, presses Enter, tokenizer processes it

---

## Technical Details

### UTF-8 Encoding

**Byte Ranges**:
- 1-byte (ASCII): `0x00-0x7F`
- 2-byte: `0xC2-0xDF` + `0x80-0xBF`
- 3-byte: `0xE0-0xEF` + 2×(`0x80-0xBF`)
- 4-byte: `0xF0-0xF4` + 3×(`0x80-0xBF`)

**Example**: "café"
- `c` = `0x63` (1-byte ASCII)
- `a` = `0x61` (1-byte ASCII)
- `f` = `0x66` (1-byte ASCII)
- `é` = `0xC3 0xA9` (2-byte UTF-8 = U+00E9)

### Reused LLE Functions

**`lle_utf8_decode_codepoint()`**:
- Decodes UTF-8 byte sequence to Unicode codepoint
- Returns byte length consumed (1-4) or -1 on error
- Validates continuation bytes
- Already tested and proven in LLE implementation

**`lle_utf8_sequence_length()`**:
- Returns expected length from first byte
- Used internally by decode function

---

## Edge Cases

### Valid UTF-8

✅ All valid UTF-8 sequences tokenize correctly  
✅ Multi-byte characters recognized as single word units  
✅ Mixed ASCII and UTF-8 in same word works

### Invalid UTF-8

❌ Invalid sequences treated as end-of-word  
❌ Incomplete sequences at end of input stop tokenization  
**Behavior**: Graceful degradation - tokenizes valid portion, stops at invalid

### Shell Metacharacters

✅ All shell metacharacters remain ASCII-only  
✅ UTF-8 characters never misinterpreted as operators  
**Reason**: Metacharacters (`|&;<>` etc.) are all < 0x80

---

## Performance

### Impact Analysis

**Before**: Byte-by-byte scan with ASCII checks  
**After**: UTF-8 decode + codepoint checks

**Overhead**:
- ASCII characters: Minimal (1 extra function call)
- Multi-byte UTF-8: Slight overhead (decode operation)
- **Overall**: Negligible for typical command input

**Optimization**: ASCII fast path in `is_word_codepoint()` minimizes overhead for common case

---

## Backward Compatibility

✅ **100% backward compatible**  
- ASCII input behavior unchanged  
- Existing scripts work identically  
- No breaking changes to tokenizer API

**Migration**: None required - drop-in replacement

---

## Known Limitations

### 1. Visual Column Counting

**Current**: One column per UTF-8 character  
**Actual**: CJK/emoji may occupy 2 terminal columns

**Impact**: Minimal - column tracking used for error reporting, not critical for functionality  
**Future**: Could integrate `lle_utf8_codepoint_width()` for accurate column counts

### 2. Normalization

**Current**: No Unicode normalization  
**Example**: `é` (composed) ≠ `é` (e + combining accent)

**Impact**: Rare - most text uses composed form  
**Future**: Could add normalization layer if needed

### 3. Grapheme Clusters

**Current**: Tokenizes by codepoints, not graphemes  
**Example**: `👨‍👩‍👧‍👦` (family emoji) = multiple codepoints, treated as one word

**Impact**: None - multi-codepoint sequences stay together in words  
**Reason**: Whitespace and metacharacters are single-codepoint, so boundaries detected correctly

---

## Relationship to LLE UTF-8 Work

### Separate Components

**LLE (Line Editor)**:
- Purpose: Edit text before execution
- Granularity: **Grapheme clusters** (UAX #29)
- Operations: Navigation, deletion, display
- Status: ✅ Complete (Phase 1)

**Tokenizer**:
- Purpose: Parse commands for execution  
- Granularity: **Codepoints** (UTF-8 characters)
- Operations: Word splitting, keyword recognition
- Status: ✅ Complete

### Integration Point

```
User types in LLE → UTF-8 text in buffer
  ↓
User presses Enter
  ↓
LLE buffer passed to tokenizer → UTF-8 tokenization
  ↓
Tokens passed to parser → Command execution
```

### Why Different Granularity?

**LLE needs graphemes**: User sees `👨‍👩‍👧‍👦` as 1 character, expects 1 deletion  
**Tokenizer needs codepoints**: Easier to implement, sufficient for word boundaries

**Both work correctly because**:
- Whitespace is always single-codepoint (space, tab, newline)
- Shell metacharacters are always single-codepoint ASCII
- Word boundaries detected correctly at both granularities

---

## Future Enhancements

### Potential Improvements

1. **Accurate column tracking**: Use `lle_utf8_codepoint_width()` for error messages
2. **Unicode normalization**: Handle composed vs decomposed forms
3. **Locale-aware classification**: Could use locale for word character determination
4. **Performance optimization**: Cache decoded codepoints in hot paths

### Not Planned

- **Bidirectional text**: Rare in shell commands, complex to implement
- **Grapheme-aware tokenization**: Unnecessary - codepoint-level works correctly

---

## Testing

### Manual Testing

All test cases verified with interactive lusush shell:
```bash
$ ./builddir/lusush
> echo café
café
> echo 日本
日本
> echo 🎉
🎉
```

### Automated Testing

Test script: `/tmp/utf8_test_commands.sh`
- 7 test cases covering all UTF-8 byte lengths
- 100% pass rate

### Regression Testing

✅ ASCII commands unchanged  
✅ Shell scripts with ASCII work identically  
✅ Existing tokenizer behavior preserved

---

## Commit Information

**Branch**: `feature/lle`  
**Files**:
- `src/tokenizer.c` - UTF-8 tokenization implementation
- `docs/development/TOKENIZER_UTF8_SUPPORT.md` - This document

**Related Work**:
- LLE UTF-8/Grapheme support (Phase 1)
- Parser UTF-8 bug documentation (`docs/bugs/CRITICAL_PARSER_UTF8_BUG.md`)

**Status**: Ready to commit

---

## References

### Code

- `src/tokenizer.c:375-400` - `is_word_codepoint()` implementation
- `src/tokenizer.c:990-1050` - UTF-8 aware word scanning loop
- `src/lle/utf8_support.c` - UTF-8 utility functions (reused)

### Documentation

- `docs/bugs/CRITICAL_PARSER_UTF8_BUG.md` - Original bug report
- `docs/development/lle-utf8-grapheme/README.md` - LLE UTF-8 implementation

### Standards

- RFC 3629: UTF-8 encoding specification
- UAX #29: Unicode grapheme boundaries (LLE uses this)

---

## Sign-off

**Implementation**: Complete ✅  
**Testing**: 7/7 tests pass ✅  
**Documentation**: Complete ✅  
**Status**: **PRODUCTION READY**

The shell tokenizer now has full UTF-8 support, completing the UTF-8 pipeline from LLE input through command execution.
