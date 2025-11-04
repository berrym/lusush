/*
 * Lusush Shell - Screen Buffer Management Implementation
 * 
 * Copyright (C) 2021-2025  Michael Berry
 *
 * ARCHITECTURE:
 * 
 * This implements a virtual screen buffer system for reliable terminal updates.
 * The approach is based on how ZLE (zsh), Fish, and Replxx handle display:
 * 
 * 1. Maintain a virtual representation of what's on the terminal screen
 * 2. When content changes, render new state into a new virtual screen
 * 3. Diff old vs new virtual screens to find what changed
 * 4. Generate minimal escape sequences to apply only the changes
 * 
 * This solves line wrapping issues because we never rely on cursor movement
 * tricks or assumptions about terminal state. We always know exactly what's
 * on screen and what needs to change.
 */

#include "display/screen_buffer.h"
#include "lle/utf8_support.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

// ============================================================================
// INITIALIZATION AND CLEANUP
// ============================================================================

void screen_buffer_init(screen_buffer_t *buffer, int terminal_width) {
    if (!buffer) return;
    
    memset(buffer, 0, sizeof(screen_buffer_t));
    buffer->terminal_width = terminal_width > 0 ? terminal_width : 80;
    buffer->num_rows = 0;
    buffer->cursor_row = 0;
    buffer->cursor_col = 0;
}

void screen_buffer_clear(screen_buffer_t *buffer) {
    if (!buffer) return;
    
    for (int i = 0; i < buffer->num_rows; i++) {
        buffer->lines[i].length = 0;
        buffer->lines[i].dirty = false;
        memset(buffer->lines[i].cells, 0, sizeof(buffer->lines[i].cells));
    }
    buffer->num_rows = 0;
    buffer->cursor_row = 0;
    buffer->cursor_col = 0;
}

void screen_buffer_copy(screen_buffer_t *dest, const screen_buffer_t *src) {
    if (!dest || !src) return;
    memcpy(dest, src, sizeof(screen_buffer_t));
}

// ============================================================================
// TEXT WIDTH CALCULATION
// ============================================================================

size_t screen_buffer_visual_width(const char *text, size_t byte_length) {
    if (!text) return 0;
    
    size_t visual_width = 0;
    size_t i = 0;
    bool in_escape = false;
    
    while (i < byte_length && text[i]) {
        unsigned char ch = (unsigned char)text[i];
        
        // Handle ANSI escape sequences (they take 0 columns)
        if (ch == '\033' || ch == '\x1b') {
            in_escape = true;
            i++;
            continue;
        }
        
        if (in_escape) {
            i++;
            // Check for escape sequence terminator
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || 
                ch == 'm' || ch == 'H' || ch == 'J' || ch == 'K' || ch == 'G') {
                in_escape = false;
            }
            continue;
        }
        
        // Skip readline markers \001 and \002
        if (ch == '\001' || ch == '\002') {
            i++;
            continue;
        }
        
        // Handle UTF-8 multi-byte sequences
        if ((ch & 0x80) == 0) {
            // ASCII: 1 byte, 1 column
            visual_width++;
            i++;
        } else if ((ch & 0xE0) == 0xC0) {
            // 2-byte UTF-8
            visual_width++;
            i += 2;
        } else if ((ch & 0xF0) == 0xE0) {
            // 3-byte UTF-8
            visual_width++;
            i += 3;
        } else if ((ch & 0xF8) == 0xF0) {
            // 4-byte UTF-8
            visual_width++;
            i += 4;
        } else {
            // Invalid UTF-8, skip
            i++;
        }
    }
    
    return visual_width;
}

// ============================================================================
// RENDERING
// ============================================================================

/**
 * Helper: Write a character to screen buffer at current position
 * Handles wrapping to next line automatically
 */
static void write_char_to_buffer(
    screen_buffer_t *buffer,
    char ch,
    bool is_prompt,
    int *row,
    int *col
) {
    if (!buffer || !row || !col) return;
    
    // Check if we need to wrap to next line
    if (*col >= buffer->terminal_width) {
        (*row)++;
        *col = 0;
        
        // Ensure we have enough rows
        if (*row >= SCREEN_BUFFER_MAX_ROWS) {
            return;  // Can't write beyond buffer
        }
        
        if (*row >= buffer->num_rows) {
            buffer->num_rows = *row + 1;
        }
    }
    
    // Write character to buffer
    buffer->lines[*row].cells[*col].ch = ch;
    buffer->lines[*row].cells[*col].is_prompt = is_prompt;
    
    if (*col >= buffer->lines[*row].length) {
        buffer->lines[*row].length = *col + 1;
    }
    
    (*col)++;
}

void screen_buffer_render(
    screen_buffer_t *buffer,
    const char *prompt_text,
    const char *command_text,
    size_t cursor_byte_offset
) {
    if (!buffer) return;
    
    // Clear buffer
    screen_buffer_clear(buffer);
    
    int row = 0;
    int col = 0;
    bool cursor_set = false;
    
    // Render prompt - calculate visual width (excluding ANSI codes)
    if (prompt_text) {
        size_t i = 0;
        size_t text_len = strlen(prompt_text);
        bool in_readline_marker = false;
        
        while (i < text_len) {
            unsigned char ch = (unsigned char)prompt_text[i];
            
            // Handle readline markers \001 and \002
            if (ch == '\001') { in_readline_marker = true; i++; continue; }
            if (ch == '\002') { in_readline_marker = false; i++; continue; }
            if (in_readline_marker) { i++; continue; }
            
            // Handle ANSI escape sequences (skip without advancing position)
            if (ch == '\033' || ch == '\x1b') {
                i++;
                if (i < text_len && prompt_text[i] == '[') {
                    i++;
                    while (i < text_len) {
                        char c = prompt_text[i++];
                        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
                            c == 'm' || c == 'H' || c == 'J' || c == 'K' || c == 'G') {
                            break;
                        }
                    }
                }
                continue;
            }
            
            // Decode UTF-8 codepoint for proper width calculation
            uint32_t codepoint;
            int bytes = lle_utf8_decode_codepoint(prompt_text + i, text_len - i, &codepoint);
            
            if (bytes > 0 && codepoint >= 32) {
                int visual_width = lle_utf8_codepoint_width(codepoint);
                if (visual_width > 0) {
                    // For now, just store first byte (proper impl would store UTF-8 sequence)
                    write_char_to_buffer(buffer, prompt_text[i], true, &row, &col);
                    // Advance col by actual width (handles wide chars)
                    if (visual_width == 2) {
                        col++; // Wide character takes 2 columns
                    }
                }
                i += bytes;
            } else {
                i++;
            }
        }
    }
    
    // Save position where command starts (this is where cursor save happens)
    int command_start_row = row;
    int command_start_col = col;
    
    // Render command text using same approach as display_bridge.c
    if (command_text) {
        size_t i = 0;
        size_t text_len = strlen(command_text);
        size_t bytes_processed = 0;  // Actual bytes in raw text (excludes ANSI codes)
        
        while (i < text_len) {
            // Check cursor position BEFORE processing next character
            if (!cursor_set && bytes_processed == cursor_byte_offset) {
                buffer->cursor_row = row;
                buffer->cursor_col = col;
                cursor_set = true;
            }
            
            unsigned char ch = (unsigned char)command_text[i];
            
            // Handle ANSI escape sequences (skip without advancing bytes_processed or position)
            if (ch == '\033' || ch == '\x1b') {
                i++;
                if (i < text_len && command_text[i] == '[') {
                    i++;
                    while (i < text_len) {
                        char c = command_text[i++];
                        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
                            c == 'm' || c == 'H' || c == 'J' || c == 'K' || c == 'G' ||
                            c == 'f' || c == 's' || c == 'u') {
                            break;
                        }
                    }
                }
                // Don't increment bytes_processed - ANSI codes don't count
                continue;
            }
            
            // Handle newlines
            if (ch == '\n') {
                row++;
                col = 0;
                if (row >= buffer->num_rows) {
                    buffer->num_rows = row + 1;
                }
                i++;
                bytes_processed++;
                continue;
            }
            
            // Handle tabs
            if (ch == '\t') {
                size_t tab_width = 8 - (col % 8);
                col += tab_width;
                if (col >= buffer->terminal_width) {
                    row++;
                    col = 0;
                    if (row >= buffer->num_rows) {
                        buffer->num_rows = row + 1;
                    }
                }
                i++;
                bytes_processed++;
                continue;
            }
            
            // Decode UTF-8 codepoint for proper width calculation
            uint32_t codepoint;
            int char_bytes = lle_utf8_decode_codepoint(command_text + i, text_len - i, &codepoint);
            
            if (char_bytes > 0 && codepoint >= 32) {
                int visual_width = lle_utf8_codepoint_width(codepoint);
                
                if (visual_width > 0) {
                    // Store character (simplified - stores first byte only)
                    write_char_to_buffer(buffer, command_text[i], false, &row, &col);
                    
                    // For wide characters, advance col by 2
                    if (visual_width == 2) {
                        col++;
                        if (col >= buffer->terminal_width) {
                            row++;
                            col = 0;
                            if (row >= buffer->num_rows) {
                                buffer->num_rows = row + 1;
                            }
                        }
                    }
                }
                
                i += char_bytes;
                bytes_processed += char_bytes;
            } else {
                i++;
                bytes_processed++;
            }
        }
        
        // If cursor is at end of text
        if (!cursor_set && bytes_processed == cursor_byte_offset) {
            buffer->cursor_row = row;
            buffer->cursor_col = col;
            cursor_set = true;
        }
        
        // Keep cursor positions as ABSOLUTE screen coordinates for screen_buffer_diff/apply_diff
        // (they use ESC[row;colH which expects absolute positions)
    }
    
    // Ensure at least one row
    if (buffer->num_rows == 0) {
        buffer->num_rows = 1;
    }
}

// ============================================================================
// DIFFERENTIAL COMPARISON
// ============================================================================

void screen_buffer_diff(
    const screen_buffer_t *old_buffer,
    const screen_buffer_t *new_buffer,
    screen_diff_t *diff
) {
    if (!old_buffer || !new_buffer || !diff) return;
    
    diff->num_changes = 0;
    
    // Compare line by line
    int max_rows = new_buffer->num_rows > old_buffer->num_rows ? 
                   new_buffer->num_rows : old_buffer->num_rows;
    
    for (int row = 0; row < max_rows && diff->num_changes < SCREEN_BUFFER_MAX_ROWS * 2; row++) {
        // If this row doesn't exist in new buffer, clear it
        if (row >= new_buffer->num_rows) {
            screen_change_t *change = &diff->changes[diff->num_changes++];
            change->type = SCREEN_CHANGE_CLEAR_TO_EOS;
            change->row = row;
            change->col = 0;
            break;  // Everything below is also cleared
        }
        
        const screen_line_t *old_line = row < old_buffer->num_rows ? 
                                        &old_buffer->lines[row] : NULL;
        const screen_line_t *new_line = &new_buffer->lines[row];
        
        // Find first difference in this line
        int first_diff = -1;
        int last_diff = -1;
        
        for (int col = 0; col < new_line->length || (old_line && col < old_line->length); col++) {
            char old_ch = (old_line && col < old_line->length) ? old_line->cells[col].ch : '\0';
            char new_ch = (col < new_line->length) ? new_line->cells[col].ch : '\0';
            
            if (old_ch != new_ch) {
                if (first_diff == -1) {
                    first_diff = col;
                }
                last_diff = col;
            }
        }
        
        // If line changed, generate change
        if (first_diff != -1) {
            screen_change_t *change = &diff->changes[diff->num_changes++];
            change->type = SCREEN_CHANGE_WRITE_TEXT;
            change->row = row;
            change->col = first_diff;
            change->text_len = 0;
            
            // Copy changed text
            for (int col = first_diff; col <= last_diff && col < new_line->length; col++) {
                change->text[change->text_len++] = new_line->cells[col].ch;
            }
            change->text[change->text_len] = '\0';
            
            // If new line is shorter, clear rest
            if (old_line && last_diff < old_line->length - 1 && last_diff == new_line->length - 1) {
                screen_change_t *clear_change = &diff->changes[diff->num_changes++];
                clear_change->type = SCREEN_CHANGE_CLEAR_TO_EOL;
                clear_change->row = row;
                clear_change->col = last_diff + 1;
            }
        }
    }
    
    // Add cursor movement if needed
    if (new_buffer->cursor_row != old_buffer->cursor_row || 
        new_buffer->cursor_col != old_buffer->cursor_col) {
        screen_change_t *change = &diff->changes[diff->num_changes++];
        change->type = SCREEN_CHANGE_MOVE_CURSOR;
        change->row = new_buffer->cursor_row;
        change->col = new_buffer->cursor_col;
    }
}

// ============================================================================
// TERMINAL OUTPUT
// ============================================================================

void screen_buffer_apply_diff(const screen_diff_t *diff, int fd) {
    if (!diff || fd < 0) return;
    
    char escape_buf[256];
    int len;
    
    for (int i = 0; i < diff->num_changes; i++) {
        const screen_change_t *change = &diff->changes[i];
        
        switch (change->type) {
            case SCREEN_CHANGE_WRITE_TEXT:
                // Move to position
                len = snprintf(escape_buf, sizeof(escape_buf), "\033[%d;%dH", 
                              change->row + 1, change->col + 1);
                write(fd, escape_buf, len);
                
                // Write text
                write(fd, change->text, change->text_len);
                break;
                
            case SCREEN_CHANGE_CLEAR_TO_EOL:
                // Move to position
                len = snprintf(escape_buf, sizeof(escape_buf), "\033[%d;%dH", 
                              change->row + 1, change->col + 1);
                write(fd, escape_buf, len);
                
                // Clear to end of line
                write(fd, "\033[K", 3);
                break;
                
            case SCREEN_CHANGE_CLEAR_TO_EOS:
                // Move to position
                len = snprintf(escape_buf, sizeof(escape_buf), "\033[%d;%dH", 
                              change->row + 1, change->col + 1);
                write(fd, escape_buf, len);
                
                // Clear to end of screen
                write(fd, "\033[J", 3);
                break;
                
            case SCREEN_CHANGE_MOVE_CURSOR:
                // Move cursor to final position
                len = snprintf(escape_buf, sizeof(escape_buf), "\033[%d;%dH", 
                              change->row + 1, change->col + 1);
                write(fd, escape_buf, len);
                break;
                
            default:
                break;
        }
    }
    
    fsync(fd);
}
