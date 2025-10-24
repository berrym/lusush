/*
 * utf8_support.c - Complete UTF-8 Unicode Support Implementation
 * 
 * Provides comprehensive UTF-8 encoding/decoding, validation, and
 * manipulation functions.
 */

#include "../../include/lle/utf8_support.h"
#include "../../include/lle/unicode_grapheme.h"
#include <string.h>

/*
 * Get the length of a UTF-8 sequence from its first byte
 */
int lle_utf8_sequence_length(unsigned char first_byte) {
    // Single-byte character (ASCII): 0xxxxxxx
    if ((first_byte & 0x80) == 0x00) {
        return 1;
    }
    
    // Two-byte sequence: 110xxxxx
    if ((first_byte & 0xE0) == 0xC0) {
        return 2;
    }
    
    // Three-byte sequence: 1110xxxx
    if ((first_byte & 0xF0) == 0xE0) {
        return 3;
    }
    
    // Four-byte sequence: 11110xxx
    if ((first_byte & 0xF8) == 0xF0) {
        return 4;
    }
    
    // Invalid UTF-8 start byte
    return 0;
}

/*
 * Validate a complete UTF-8 sequence
 */
bool lle_utf8_is_valid_sequence(const char *ptr, int length) {
    if (!ptr || length <= 0 || length > 4) {
        return false;
    }
    
    unsigned char first = (unsigned char)ptr[0];
    
    // Validate sequence length matches first byte
    int expected_len = lle_utf8_sequence_length(first);
    if (expected_len != length) {
        return false;
    }
    
    // Single-byte ASCII
    if (length == 1) {
        return true;
    }
    
    // Multi-byte sequences - validate continuation bytes
    for (int i = 1; i < length; i++) {
        unsigned char byte = (unsigned char)ptr[i];
        // Continuation bytes must be 10xxxxxx
        if ((byte & 0xC0) != 0x80) {
            return false;
        }
    }
    
    // Additional validation for overlong encodings and invalid ranges
    uint32_t codepoint = 0;
    
    if (length == 2) {
        // Two-byte: 110xxxxx 10xxxxxx
        codepoint = ((first & 0x1F) << 6) | (ptr[1] & 0x3F);
        // Must be >= 0x80 (not overlong)
        if (codepoint < 0x80) {
            return false;
        }
    } else if (length == 3) {
        // Three-byte: 1110xxxx 10xxxxxx 10xxxxxx
        codepoint = ((first & 0x0F) << 12) | 
                    ((ptr[1] & 0x3F) << 6) | 
                    (ptr[2] & 0x3F);
        // Must be >= 0x800 (not overlong)
        // Must not be in surrogate range (0xD800-0xDFFF)
        if (codepoint < 0x800 || 
            (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            return false;
        }
    } else if (length == 4) {
        // Four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        codepoint = ((first & 0x07) << 18) | 
                    ((ptr[1] & 0x3F) << 12) | 
                    ((ptr[2] & 0x3F) << 6) | 
                    (ptr[3] & 0x3F);
        // Must be >= 0x10000 (not overlong)
        // Must be <= 0x10FFFF (maximum Unicode codepoint)
        if (codepoint < 0x10000 || codepoint > 0x10FFFF) {
            return false;
        }
    }
    
    return true;
}

/*
 * Validate an entire UTF-8 string
 */
bool lle_utf8_is_valid(const char *text, size_t length) {
    if (!text) {
        return false;
    }
    
    const char *ptr = text;
    const char *end = text + length;
    
    while (ptr < end) {
        int seq_len = lle_utf8_sequence_length((unsigned char)*ptr);
        if (seq_len == 0 || ptr + seq_len > end) {
            return false;
        }
        
        if (!lle_utf8_is_valid_sequence(ptr, seq_len)) {
            return false;
        }
        
        ptr += seq_len;
    }
    
    return true;
}

/*
 * Decode a UTF-8 sequence to a Unicode codepoint
 */
int lle_utf8_decode_codepoint(const char *ptr, size_t max_len, uint32_t *codepoint) {
    if (!ptr || !codepoint || max_len == 0) {
        return -1;
    }
    
    unsigned char first = (unsigned char)ptr[0];
    int seq_len = lle_utf8_sequence_length(first);
    
    if (seq_len == 0 || (size_t)seq_len > max_len) {
        return -1;
    }
    
    // Validate the sequence
    if (!lle_utf8_is_valid_sequence(ptr, seq_len)) {
        return -1;
    }
    
    // Decode based on length
    if (seq_len == 1) {
        // ASCII
        *codepoint = first;
    } else if (seq_len == 2) {
        // Two-byte sequence
        *codepoint = ((first & 0x1F) << 6) | (ptr[1] & 0x3F);
    } else if (seq_len == 3) {
        // Three-byte sequence
        *codepoint = ((first & 0x0F) << 12) | 
                     ((ptr[1] & 0x3F) << 6) | 
                     (ptr[2] & 0x3F);
    } else if (seq_len == 4) {
        // Four-byte sequence
        *codepoint = ((first & 0x07) << 18) | 
                     ((ptr[1] & 0x3F) << 12) | 
                     ((ptr[2] & 0x3F) << 6) | 
                     (ptr[3] & 0x3F);
    }
    
    return seq_len;
}

/*
 * Encode a Unicode codepoint to UTF-8
 */
int lle_utf8_encode_codepoint(uint32_t codepoint, char *buffer) {
    if (!buffer) {
        return 0;
    }
    
    // Validate codepoint range
    if (codepoint > 0x10FFFF || 
        (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return 0;  // Invalid codepoint
    }
    
    if (codepoint <= 0x7F) {
        // Single-byte (ASCII)
        buffer[0] = (char)codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        // Two-byte sequence
        buffer[0] = (char)(0xC0 | (codepoint >> 6));
        buffer[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    } else if (codepoint <= 0xFFFF) {
        // Three-byte sequence
        buffer[0] = (char)(0xE0 | (codepoint >> 12));
        buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    } else {
        // Four-byte sequence
        buffer[0] = (char)(0xF0 | (codepoint >> 18));
        buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
}

/*
 * Count the number of Unicode codepoints in a UTF-8 string
 */
size_t lle_utf8_count_codepoints(const char *text, size_t length) {
    if (!text || length == 0) {
        return 0;
    }
    
    const char *ptr = text;
    const char *end = text + length;
    size_t count = 0;
    
    while (ptr < end) {
        int seq_len = lle_utf8_sequence_length((unsigned char)*ptr);
        if (seq_len == 0 || ptr + seq_len > end) {
            break;  // Invalid UTF-8 or end of string
        }
        
        count++;
        ptr += seq_len;
    }
    
    return count;
}

/*
 * Convert byte offset to codepoint index
 */
int lle_utf8_byte_to_codepoint_index(const char *text, size_t byte_offset, 
                                     size_t *cp_index) {
    if (!text || !cp_index) {
        return -1;
    }
    
    const char *ptr = text;
    size_t index = 0;
    size_t current_byte = 0;
    
    while (current_byte < byte_offset) {
        int seq_len = lle_utf8_sequence_length((unsigned char)*ptr);
        if (seq_len == 0) {
            return -1;  // Invalid UTF-8
        }
        
        current_byte += seq_len;
        if (current_byte <= byte_offset) {
            index++;
            ptr += seq_len;
        }
    }
    
    *cp_index = index;
    return 0;
}

/*
 * Convert codepoint index to byte offset
 */
int lle_utf8_codepoint_to_byte_offset(const char *text, size_t cp_index, 
                                      size_t *byte_offset) {
    if (!text || !byte_offset) {
        return -1;
    }
    
    const char *ptr = text;
    size_t index = 0;
    size_t offset = 0;
    
    while (index < cp_index) {
        int seq_len = lle_utf8_sequence_length((unsigned char)*ptr);
        if (seq_len == 0) {
            return -1;  // Invalid UTF-8 or past end
        }
        
        offset += seq_len;
        ptr += seq_len;
        index++;
    }
    
    *byte_offset = offset;
    return 0;
}

/*
 * Convert codepoint index to grapheme cluster index
 */
int lle_utf8_codepoint_to_grapheme_index(const char *text, size_t cp_index, 
                                         size_t *grapheme_index) {
    if (!text || !grapheme_index) {
        return -1;
    }
    
    const char *ptr = text;
    const char *start = text;
    size_t cp_count = 0;
    size_t grapheme_count = 0;
    
    // Count grapheme clusters up to the specified codepoint
    while (cp_count <= cp_index) {
        int seq_len = lle_utf8_sequence_length((unsigned char)*ptr);
        if (seq_len == 0) {
            return -1;  // Invalid UTF-8
        }
        
        // Check if this is a grapheme boundary
        if (lle_is_grapheme_boundary(ptr, start, ptr + seq_len)) {
            if (cp_count < cp_index) {
                grapheme_count++;
            }
        }
        
        if (cp_count == cp_index) {
            break;
        }
        
        ptr += seq_len;
        cp_count++;
    }
    
    *grapheme_index = grapheme_count;
    return 0;
}

/*
 * Get the display width of a Unicode codepoint
 * 
 * This implements a subset of wcwidth() functionality for common cases.
 * Based on Unicode character width properties.
 */
int lle_utf8_codepoint_width(uint32_t codepoint) {
    // Zero-width characters
    if (codepoint == 0x0000 ||                            // NULL
        codepoint == 0x200B ||                            // Zero-width space
        codepoint == 0x200C ||                            // Zero-width non-joiner
        codepoint == 0x200D ||                            // Zero-width joiner
        (codepoint >= 0x200E && codepoint <= 0x200F) ||  // LRM, RLM
        codepoint == 0xFEFF ||                            // Zero-width no-break space
        (codepoint >= 0x0300 && codepoint <= 0x036F) ||  // Combining diacriticals
        (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) ||  // Combining diacriticals supplement
        (codepoint >= 0x20D0 && codepoint <= 0x20FF) ||  // Combining marks for symbols
        (codepoint >= 0xFE00 && codepoint <= 0xFE0F) ||  // Variation selectors
        (codepoint >= 0xFE20 && codepoint <= 0xFE2F)) {  // Combining half marks
        return 0;
    }
    
    // Control characters
    if (codepoint < 0x20 || (codepoint >= 0x7F && codepoint < 0xA0)) {
        return 0;
    }
    
    // Wide characters (CJK and emoji)
    // CJK Unified Ideographs
    if ((codepoint >= 0x1100 && codepoint <= 0x115F) ||   // Hangul Jamo
        (codepoint >= 0x2329 && codepoint <= 0x232A) ||   // Angle brackets
        (codepoint >= 0x2E80 && codepoint <= 0x2E99) ||   // CJK radicals supplement
        (codepoint >= 0x2E9B && codepoint <= 0x2EF3) ||
        (codepoint >= 0x2F00 && codepoint <= 0x2FD5) ||   // Kangxi radicals
        (codepoint >= 0x2FF0 && codepoint <= 0x2FFB) ||   // Ideographic description
        (codepoint >= 0x3000 && codepoint <= 0x303E) ||   // CJK symbols and punctuation
        (codepoint >= 0x3041 && codepoint <= 0x3096) ||   // Hiragana
        (codepoint >= 0x3099 && codepoint <= 0x30FF) ||   // Katakana
        (codepoint >= 0x3105 && codepoint <= 0x312F) ||   // Bopomofo
        (codepoint >= 0x3131 && codepoint <= 0x318E) ||   // Hangul compatibility jamo
        (codepoint >= 0x3190 && codepoint <= 0x31E3) ||   // CJK strokes and misc
        (codepoint >= 0x31F0 && codepoint <= 0x321E) ||   // Katakana phonetic extensions
        (codepoint >= 0x3220 && codepoint <= 0x3247) ||   // Enclosed CJK letters
        (codepoint >= 0x3250 && codepoint <= 0x4DBF) ||   // CJK unified ideographs extension A
        (codepoint >= 0x4E00 && codepoint <= 0xA48C) ||   // CJK unified ideographs
        (codepoint >= 0xA490 && codepoint <= 0xA4C6) ||   // Yi radicals
        (codepoint >= 0xAC00 && codepoint <= 0xD7A3) ||   // Hangul syllables
        (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||   // CJK compatibility ideographs
        (codepoint >= 0xFE10 && codepoint <= 0xFE19) ||   // Vertical forms
        (codepoint >= 0xFE30 && codepoint <= 0xFE6B) ||   // CJK compatibility forms
        (codepoint >= 0xFF01 && codepoint <= 0xFF60) ||   // Fullwidth forms
        (codepoint >= 0xFFE0 && codepoint <= 0xFFE6) ||   // Fullwidth currency signs
        (codepoint >= 0x1F000 && codepoint <= 0x1F9FF) || // Emoji and pictographs
        (codepoint >= 0x20000 && codepoint <= 0x2FFFD) || // CJK unified ideographs extension B-F
        (codepoint >= 0x30000 && codepoint <= 0x3FFFD)) { // CJK unified ideographs extension G
        return 2;  // Wide character
    }
    
    // Default: normal width
    return 1;
}

/*
 * Get the display width of a UTF-8 string
 */
size_t lle_utf8_string_width(const char *text, size_t length) {
    if (!text || length == 0) {
        return 0;
    }
    
    const char *ptr = text;
    const char *end = text + length;
    size_t total_width = 0;
    
    while (ptr < end) {
        uint32_t codepoint = 0;
        int seq_len = lle_utf8_decode_codepoint(ptr, end - ptr, &codepoint);
        if (seq_len <= 0) {
            break;  // Invalid UTF-8
        }
        
        total_width += lle_utf8_codepoint_width(codepoint);
        ptr += seq_len;
    }
    
    return total_width;
}
