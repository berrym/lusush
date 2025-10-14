// src/lle/foundation/buffer/buffer_util.c
//
// LLE Gap Buffer Utility Functions (search, line operations)

#include "buffer.h"
#include <string.h>

lle_buffer_pos_t lle_buffer_find_char(const lle_buffer_t *buffer,
                                      lle_buffer_pos_t start,
                                      char ch) {
    if (!buffer || !buffer->data) {
        return LLE_BUFFER_NPOS;
    }
    
    size_t size = lle_buffer_size(buffer);
    if (start >= size) {
        return LLE_BUFFER_NPOS;
    }
    
    // Search before gap
    if (start < buffer->gap_start) {
        for (size_t i = start; i < buffer->gap_start; i++) {
            if (buffer->data[i] == ch) {
                return i;
            }
        }
        start = buffer->gap_start;
    }
    
    // Search after gap
    size_t gap_sz = buffer->gap_end - buffer->gap_start;
    for (size_t i = start; i < size; i++) {
        size_t actual_idx = i + gap_sz;
        if (buffer->data[actual_idx] == ch) {
            return i;
        }
    }
    
    return LLE_BUFFER_NPOS;
}

lle_buffer_pos_t lle_buffer_find_string(const lle_buffer_t *buffer,
                                        lle_buffer_pos_t start,
                                        const char *str,
                                        size_t len) {
    if (!buffer || !buffer->data || !str || len == 0) {
        return LLE_BUFFER_NPOS;
    }
    
    size_t size = lle_buffer_size(buffer);
    if (start >= size || start + len > size) {
        return LLE_BUFFER_NPOS;
    }
    
    // Simple search implementation
    // TODO: Could optimize with Boyer-Moore or similar
    for (size_t pos = start; pos <= size - len; pos++) {
        bool match = true;
        for (size_t i = 0; i < len; i++) {
            char ch;
            if (lle_buffer_get_char(buffer, pos + i, &ch) != LLE_BUFFER_OK) {
                return LLE_BUFFER_NPOS;
            }
            if (ch != str[i]) {
                match = false;
                break;
            }
        }
        if (match) {
            return pos;
        }
    }
    
    return LLE_BUFFER_NPOS;
}

lle_buffer_pos_t lle_buffer_line_start(const lle_buffer_t *buffer,
                                       lle_buffer_pos_t pos) {
    if (!buffer || !buffer->data) {
        return 0;
    }
    
    size_t size = lle_buffer_size(buffer);
    if (pos >= size) {
        pos = size > 0 ? size - 1 : 0;
    }
    
    // Search backward for newline
    while (pos > 0) {
        char ch;
        if (lle_buffer_get_char(buffer, pos - 1, &ch) != LLE_BUFFER_OK) {
            break;
        }
        if (ch == '\n') {
            break;
        }
        pos--;
    }
    
    return pos;
}

lle_buffer_pos_t lle_buffer_line_end(const lle_buffer_t *buffer,
                                     lle_buffer_pos_t pos) {
    if (!buffer || !buffer->data) {
        return 0;
    }
    
    size_t size = lle_buffer_size(buffer);
    if (pos >= size) {
        return size;
    }
    
    // Search forward for newline
    while (pos < size) {
        char ch;
        if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
            break;
        }
        if (ch == '\n') {
            break;
        }
        pos++;
    }
    
    return pos;
}

size_t lle_buffer_line_count(const lle_buffer_t *buffer) {
    if (!buffer || !buffer->data) {
        return 0;
    }
    
    size_t size = lle_buffer_size(buffer);
    if (size == 0) {
        return 0;
    }
    
    size_t count = 1;  // At least one line if non-empty
    
    // Count newlines
    for (size_t pos = 0; pos < size; pos++) {
        char ch;
        if (lle_buffer_get_char(buffer, pos, &ch) == LLE_BUFFER_OK) {
            if (ch == '\n') {
                count++;
            }
        }
    }
    
    return count;
}

size_t lle_buffer_get_line_number(const lle_buffer_t *buffer,
                                  lle_buffer_pos_t pos) {
    if (!buffer || !buffer->data) {
        return 0;
    }
    
    size_t size = lle_buffer_size(buffer);
    if (pos >= size) {
        pos = size > 0 ? size - 1 : 0;
    }
    
    size_t line = 0;
    
    // Count newlines before position
    for (size_t i = 0; i < pos; i++) {
        char ch;
        if (lle_buffer_get_char(buffer, i, &ch) == LLE_BUFFER_OK) {
            if (ch == '\n') {
                line++;
            }
        }
    }
    
    return line;
}
