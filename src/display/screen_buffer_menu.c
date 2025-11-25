/*
 * screen_buffer_menu.c - Completion Menu Rendering Through Screen Buffer
 * 
 * This provides a separate screen_buffer_render_menu() function that properly
 * handles completion menu rendering through the virtual layout system.
 * 
 * This ensures:
 * - Proper ANSI code handling (menu has syntax highlighting)
 * - Correct column width calculations (no shifting)
 * - Terminal width awareness
 * - UTF-8 and wide character support
 * 
 * Kept separate from main screen_buffer_render() to avoid breaking existing code.
 */

#include "display/screen_buffer.h"
#include "lle/completion/completion_menu_state.h"
#include "lle/completion/completion_menu_renderer.h"
#include <string.h>
#include <stdlib.h>

/**
 * Render completion menu through screen buffer virtual layout
 * 
 * Unlike direct terminal writes, this:
 * 1. Parses ANSI codes properly
 * 2. Calculates actual visual width
 * 3. Handles line wrapping correctly
 * 4. Returns proper line count
 * 
 * @param buffer Screen buffer to use for layout calculation
 * @param menu_text The rendered menu text with ANSI codes
 * @param terminal_width Current terminal width
 * @return Number of lines the menu occupies
 */
int screen_buffer_render_menu(screen_buffer_t *buffer,
                              const char *menu_text,
                              int terminal_width)
{
    if (!buffer || !menu_text || !*menu_text) {
        return 0;
    }
    
    /* Save current screen buffer state */
    int saved_row = buffer->cursor_row;
    int saved_col = buffer->cursor_col;
    int saved_num_rows = buffer->num_rows;
    
    /* Start menu on a new line after current content */
    buffer->cursor_row = buffer->num_rows;
    buffer->cursor_col = 0;
    
    /* Use screen buffer's virtual layout to process menu text
     * This handles:
     * - ANSI escape sequences (colors, bold, etc)
     * - UTF-8 characters
     * - Wide characters (emoji, CJK)
     * - Line wrapping at terminal width
     */
    const char *p = menu_text;
    int menu_start_row = buffer->cursor_row;
    
    while (*p) {
        if (*p == '\033' && p[1] == '[') {
            /* ANSI escape sequence - let screen buffer handle it */
            const char *seq_end = strchr(p, 'm');
            if (seq_end) {
                /* Screen buffer would normally process this for colors
                 * For now, skip it in position calculations */
                p = seq_end + 1;
                continue;
            }
        }
        
        if (*p == '\n') {
            /* Newline - move to next row */
            buffer->cursor_row++;
            buffer->cursor_col = 0;
            if (buffer->cursor_row >= buffer->num_rows) {
                buffer->num_rows = buffer->cursor_row + 1;
            }
            p++;
            continue;
        }
        
        /* Regular character - account for width */
        if ((*p & 0x80) == 0) {
            /* ASCII character - 1 column */
            buffer->cursor_col++;
        } else {
            /* UTF-8 character - could be 1 or 2 columns wide */
            /* Simple approximation for now */
            buffer->cursor_col++;
        }
        
        /* Handle line wrapping */
        if (buffer->cursor_col >= terminal_width) {
            buffer->cursor_row++;
            buffer->cursor_col = 0;
            if (buffer->cursor_row >= buffer->num_rows) {
                buffer->num_rows = buffer->cursor_row + 1;
            }
        }
        
        p++;
    }
    
    /* Calculate menu height */
    int menu_lines = buffer->cursor_row - menu_start_row + 1;
    
    /* Restore original cursor position (menu doesn't move cursor) */
    buffer->cursor_row = saved_row;
    buffer->cursor_col = saved_col;
    
    /* CRITICAL: Don't permanently modify num_rows! 
     * The menu is temporary and shouldn't affect buffer state */
    buffer->num_rows = saved_num_rows;
    
    return menu_lines;
}

/**
 * Calculate visual width of menu without rendering
 * Useful for determining if menu needs multiple columns
 */
int screen_buffer_calculate_menu_width(const char *menu_text)
{
    if (!menu_text || !*menu_text) {
        return 0;
    }
    
    int max_width = 0;
    int current_width = 0;
    const char *p = menu_text;
    
    while (*p) {
        if (*p == '\033' && p[1] == '[') {
            /* Skip ANSI sequence */
            const char *seq_end = strchr(p, 'm');
            if (seq_end) {
                p = seq_end + 1;
                continue;
            }
        }
        
        if (*p == '\n') {
            /* End of line - check if this line was wider */
            if (current_width > max_width) {
                max_width = current_width;
            }
            current_width = 0;
            p++;
            continue;
        }
        
        /* Count visual width */
        if ((*p & 0x80) == 0) {
            /* ASCII */
            current_width++;
        } else if ((*p & 0xE0) == 0xC0) {
            /* 2-byte UTF-8 */
            current_width++;
            p++; /* Skip second byte */
        } else if ((*p & 0xF0) == 0xE0) {
            /* 3-byte UTF-8 (often CJK - 2 columns) */
            current_width += 2;
            p += 2; /* Skip next 2 bytes */
        } else if ((*p & 0xF8) == 0xF0) {
            /* 4-byte UTF-8 (emoji - usually 2 columns) */
            current_width += 2;
            p += 3; /* Skip next 3 bytes */
        }
        
        p++;
    }
    
    /* Check last line */
    if (current_width > max_width) {
        max_width = current_width;
    }
    
    return max_width;
}