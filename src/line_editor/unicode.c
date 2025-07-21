#include "unicode.h"
#include <string.h>

/**
 * @file unicode.c
 * @brief Lusush Line Editor - UTF-8 Unicode Support Implementation
 *
 * Implementation of UTF-8 text analysis and navigation functionality.
 * Provides robust handling of Unicode text with proper validation and
 * character boundary detection for cursor positioning.
 */

lle_utf8_info_t lle_utf8_analyze(const char *text) {
    lle_utf8_info_t info = {0, 0, false};
    
    if (!text) {
        return info;
    }
    
    const uint8_t *bytes = (const uint8_t *)text;
    size_t byte_pos = 0;
    size_t char_count = 0;
    bool valid = true;
    
    while (bytes[byte_pos] != '\0') {
        size_t char_bytes = lle_utf8_char_bytes(text, byte_pos);
        
        if (char_bytes == 0) {
            valid = false;
            break;
        }
        
        // Validate continuation bytes
        for (size_t i = 1; i < char_bytes; i++) {
            if (bytes[byte_pos + i] == '\0' || 
                !lle_utf8_is_continuation(bytes[byte_pos + i])) {
                valid = false;
                break;
            }
        }
        
        if (!valid) {
            break;
        }
        
        byte_pos += char_bytes;
        char_count++;
    }
    
    info.byte_length = strlen(text);
    info.char_length = valid ? char_count : 0;
    info.valid_utf8 = valid;
    
    return info;
}

size_t lle_utf8_char_at(const char *text, size_t char_index) {
    if (!text || char_index == SIZE_MAX) {
        return SIZE_MAX;
    }
    
    if (char_index == 0) {
        // For index 0, return 0 only if string is not empty
        return (text[0] != '\0') ? 0 : SIZE_MAX;
    }
    
    size_t byte_pos = 0;
    size_t current_char = 0;
    
    while (text[byte_pos] != '\0' && current_char < char_index) {
        size_t char_bytes = lle_utf8_char_bytes(text, byte_pos);
        
        if (char_bytes == 0) {
            return SIZE_MAX; // Invalid UTF-8
        }
        
        byte_pos += char_bytes;
        current_char++;
    }
    
    // Check if we found the requested character
    if (current_char == char_index && text[byte_pos] != '\0') {
        return byte_pos;
    }
    
    // Character index out of bounds
    return SIZE_MAX;
}

size_t lle_utf8_next_char(const char *text, size_t byte_pos) {
    if (!text) {
        return byte_pos;
    }
    
    // Check if already at end
    if (text[byte_pos] == '\0') {
        return byte_pos;
    }
    
    size_t char_bytes = lle_utf8_char_bytes(text, byte_pos);
    
    if (char_bytes == 0) {
        // Invalid UTF-8, advance one byte
        return byte_pos + 1;
    }
    
    size_t next_pos = byte_pos + char_bytes;
    
    // Make sure we don't go past end of string
    if (text[next_pos] == '\0') {
        return next_pos;
    }
    
    return next_pos;
}

size_t lle_utf8_prev_char(const char *text, size_t byte_pos) {
    if (!text || byte_pos == 0) {
        return 0;
    }
    
    // Move back one byte at a time until we find a non-continuation byte
    size_t pos = byte_pos - 1;
    
    // Skip continuation bytes (maximum 3 to avoid infinite loop)
    for (int i = 0; i < 3 && pos > 0; i++) {
        if (!lle_utf8_is_continuation((uint8_t)text[pos])) {
            break;
        }
        if (pos > 0) {
            pos--;
        }
    }
    
    // Validate that we found a valid character start
    size_t char_bytes = lle_utf8_char_bytes(text, pos);
    
    if (char_bytes == 0 || pos + char_bytes < byte_pos) {
        // Invalid UTF-8 or didn't find proper character boundary
        return byte_pos > 0 ? byte_pos - 1 : 0;
    }
    
    return pos;
}

size_t lle_utf8_char_bytes(const char *text, size_t byte_pos) {
    if (!text) {
        return 0;
    }
    
    uint8_t first_byte = (uint8_t)text[byte_pos];
    
    if (first_byte == '\0') {
        return 0;
    }
    
    return lle_utf8_expected_length(first_byte);
}

bool lle_utf8_is_continuation(uint8_t byte) {
    return (byte & LLE_UTF8_CONTINUATION_MASK) == LLE_UTF8_CONTINUATION_PATTERN;
}

size_t lle_utf8_expected_length(uint8_t first_byte) {
    // ASCII (0xxxxxxx)
    if ((first_byte & 0x80) == 0) {
        return 1;
    }
    
    // 2-byte sequence (110xxxxx)
    if ((first_byte & 0xE0) == 0xC0) {
        return 2;
    }
    
    // 3-byte sequence (1110xxxx)
    if ((first_byte & 0xF0) == 0xE0) {
        return 3;
    }
    
    // 4-byte sequence (11110xxx)
    if ((first_byte & 0xF8) == 0xF0) {
        return 4;
    }
    
    // Invalid first byte
    return 0;
}

size_t lle_utf8_count_chars(const char *text, size_t max_bytes) {
    if (!text || max_bytes == 0) {
        return 0;
    }
    
    size_t byte_pos = 0;
    size_t char_count = 0;
    
    while (byte_pos < max_bytes && text[byte_pos] != '\0') {
        size_t char_bytes = lle_utf8_char_bytes(text, byte_pos);
        
        if (char_bytes == 0) {
            // Invalid UTF-8, stop counting
            break;
        }
        
        // Check if we have enough bytes for this character
        if (byte_pos + char_bytes > max_bytes) {
            break;
        }
        
        // Validate continuation bytes
        bool valid = true;
        for (size_t i = 1; i < char_bytes; i++) {
            if (byte_pos + i >= max_bytes || 
                text[byte_pos + i] == '\0' ||
                !lle_utf8_is_continuation((uint8_t)text[byte_pos + i])) {
                valid = false;
                break;
            }
        }
        
        if (!valid) {
            break;
        }
        
        byte_pos += char_bytes;
        char_count++;
    }
    
    return char_count;
}

size_t lle_utf8_bytes_for_chars(const char *text, size_t max_chars) {
    if (!text || max_chars == 0) {
        return 0;
    }
    
    size_t byte_pos = 0;
    size_t char_count = 0;
    
    while (text[byte_pos] != '\0' && char_count < max_chars) {
        size_t char_bytes = lle_utf8_char_bytes(text, byte_pos);
        
        if (char_bytes == 0) {
            // Invalid UTF-8, stop
            break;
        }
        
        // Validate continuation bytes
        bool valid = true;
        for (size_t i = 1; i < char_bytes; i++) {
            if (text[byte_pos + i] == '\0' ||
                !lle_utf8_is_continuation((uint8_t)text[byte_pos + i])) {
                valid = false;
                break;
            }
        }
        
        if (!valid) {
            break;
        }
        
        byte_pos += char_bytes;
        char_count++;
    }
    
    return byte_pos;
}