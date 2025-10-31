/*
 * terminal_display_generator.c - Display Content Generation (Spec 02 Subsystem 3)
 * 
 * Converts internal state (authoritative model) to display content for
 * submission to Lusush display system. NEVER queries terminal state.
 * 
 * Key Responsibilities:
 * - Generate complete display content from internal buffer state
 * - Calculate line wrapping based on terminal geometry
 * - Determine cursor visual position from buffer position
 * - Apply display optimizations based on capabilities
 * 
 * Spec 02: Terminal Abstraction - Subsystem 3
 */

#include "lle/terminal_abstraction.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ============================================================================
 * DISPLAY CONTENT STRUCTURE OPERATIONS
 * ============================================================================ */

/*
 * Create display content structure
 */
lle_result_t lle_display_content_create(lle_display_content_t **content,
                                       size_t line_capacity) {
    if (!content) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (line_capacity < 1) {
        line_capacity = 10;  /* Minimum capacity */
    }
    
    lle_display_content_t *dc = calloc(1, sizeof(lle_display_content_t));
    if (!dc) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Allocate lines array */
    dc->lines = calloc(line_capacity, sizeof(lle_display_line_t));
    if (!dc->lines) {
        free(dc);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize each line */
    for (size_t i = 0; i < line_capacity; i++) {
        dc->lines[i].capacity = 256;
        dc->lines[i].content = calloc(256, 1);
        if (!dc->lines[i].content) {
            /* Clean up previously allocated lines */
            for (size_t j = 0; j < i; j++) {
                free(dc->lines[j].content);
            }
            free(dc->lines);
            free(dc);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        dc->lines[i].length = 0;
        dc->lines[i].contains_cursor = false;
    }
    
    dc->line_count = 0;
    dc->cursor_line = 0;
    dc->cursor_column = 0;
    dc->cursor_visible = true;
    dc->is_complete_refresh = true;
    dc->content_version = 0;
    dc->generation_time = lle_get_current_time_microseconds();
    
    *content = dc;
    return LLE_SUCCESS;
}

/*
 * Destroy display content structure
 */
void lle_display_content_destroy(lle_display_content_t *content) {
    if (!content) {
        return;
    }
    
    /* Free all lines */
    if (content->lines) {
        for (size_t i = 0; i < content->line_count; i++) {
            if (content->lines[i].content) {
                free(content->lines[i].content);
            }
        }
        free(content->lines);
    }
    
    free(content);
}

/* ============================================================================
 * DISPLAY GENERATOR OPERATIONS
 * ============================================================================ */

/*
 * Initialize display generator
 */
lle_result_t lle_display_generator_init(lle_display_generator_t **generator,
                                       lle_terminal_capabilities_t *caps,
                                       lle_internal_state_t *state) {
    if (!generator || !caps || !state) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_display_generator_t *gen = calloc(1, sizeof(lle_display_generator_t));
    if (!gen) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    gen->capabilities = caps;
    gen->internal_state = state;
    
    /* Create initial display content */
    lle_result_t result = lle_display_content_create(&gen->current_content, 10);
    if (result != LLE_SUCCESS) {
        free(gen);
        return result;
    }
    
    gen->previous_content = NULL;
    
    /* Set default generation parameters */
    gen->params.force_full_refresh = false;
    gen->params.optimize_for_speed = true;
    gen->params.max_display_lines = caps->terminal_height > 0 ? caps->terminal_height : 24;
    
    *generator = gen;
    return LLE_SUCCESS;
}

/*
 * Destroy display generator
 */
void lle_display_generator_destroy(lle_display_generator_t *generator) {
    if (!generator) {
        return;
    }
    
    if (generator->current_content) {
        lle_display_content_destroy(generator->current_content);
    }
    
    if (generator->previous_content) {
        lle_display_content_destroy(generator->previous_content);
    }
    
    free(generator);
}

/*
 * Ensure line has sufficient capacity
 */
static lle_result_t ensure_line_capacity(lle_display_line_t *line, size_t required) {
    if (line->capacity >= required) {
        return LLE_SUCCESS;
    }
    
    /* Calculate new capacity (grow by 1.5x) */
    size_t new_capacity = line->capacity;
    while (new_capacity < required) {
        new_capacity = new_capacity + (new_capacity / 2);
    }
    
    char *new_content = realloc(line->content, new_capacity);
    if (!new_content) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    line->content = new_content;
    line->capacity = new_capacity;
    
    return LLE_SUCCESS;
}

/*
 * Append text to display line
 */
static lle_result_t append_to_line(lle_display_line_t *line, 
                                   const char *text,
                                   size_t length) {
    if (!line || !text || length == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Ensure capacity */
    lle_result_t result = ensure_line_capacity(line, line->length + length + 1);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Append text */
    memcpy(line->content + line->length, text, length);
    line->length += length;
    line->content[line->length] = '\0';
    
    return LLE_SUCCESS;
}

/*
 * Calculate display line count for buffer with line wrapping
 */
static size_t calculate_display_lines(const char *buffer,
                                      size_t buffer_length,
                                      size_t terminal_width,
                                      size_t prompt_width) {
    if (!buffer || terminal_width == 0) {
        return 1;
    }
    
    size_t line_count = 1;
    size_t current_column = prompt_width;
    
    for (size_t i = 0; i < buffer_length; i++) {
        char c = buffer[i];
        
        if (c == '\n') {
            line_count++;
            current_column = 0;
        } else if (c == '\t') {
            current_column += 8 - (current_column % 8);
        } else {
            current_column++;
        }
        
        /* Line wrap */
        if (current_column >= terminal_width) {
            line_count++;
            current_column = 0;
        }
    }
    
    return line_count;
}

/*
 * Generate display content from internal state
 * 
 * CRITICAL: This function converts authoritative internal state to
 * display content. It NEVER queries terminal state.
 */
lle_result_t lle_display_generator_generate_content(lle_display_generator_t *generator,
                                                   lle_display_content_t **content) {
    if (!generator || !content) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_internal_state_t *state = generator->internal_state;
    lle_terminal_capabilities_t *caps = generator->capabilities;
    
    /* Calculate required display lines */
    size_t required_lines = calculate_display_lines(
        state->command_buffer->data,
        state->command_buffer->length,
        state->terminal_width,
        state->prompt_width
    );
    
    /* Create new display content */
    lle_display_content_t *new_content = NULL;
    lle_result_t result = lle_display_content_create(&new_content, required_lines + 5);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Generate display lines from buffer */
    size_t current_line = 0;
    size_t current_column = state->prompt_width;
    size_t buffer_pos = 0;
    
    /* Initialize first line */
    new_content->lines[0].length = 0;
    
    /* Process each character in buffer */
    while (buffer_pos < state->command_buffer->length) {
        char c = state->command_buffer->data[buffer_pos];
        
        /* Check if cursor is at this position */
        if (buffer_pos == state->cursor_position) {
            new_content->cursor_line = current_line;
            new_content->cursor_column = current_column;
            new_content->lines[current_line].contains_cursor = true;
            new_content->lines[current_line].cursor_column = current_column;
        }
        
        /* Handle special characters */
        if (c == '\n') {
            /* Newline - advance to next line */
            current_line++;
            current_column = 0;
            
            if (current_line < required_lines + 5) {
                new_content->lines[current_line].length = 0;
            }
        } else if (c == '\t') {
            /* Tab - expand to spaces */
            size_t spaces = 8 - (current_column % 8);
            for (size_t i = 0; i < spaces && current_column < state->terminal_width; i++) {
                result = append_to_line(&new_content->lines[current_line], " ", 1);
                if (result != LLE_SUCCESS) {
                    lle_display_content_destroy(new_content);
                    return result;
                }
                current_column++;
            }
        } else {
            /* Regular character - append to current line */
            result = append_to_line(&new_content->lines[current_line], &c, 1);
            if (result != LLE_SUCCESS) {
                lle_display_content_destroy(new_content);
                return result;
            }
            current_column++;
        }
        
        /* Handle line wrapping */
        if (current_column >= state->terminal_width) {
            current_line++;
            current_column = 0;
            
            if (current_line < required_lines + 5) {
                new_content->lines[current_line].length = 0;
            }
        }
        
        buffer_pos++;
    }
    
    /* Handle cursor at end of buffer */
    if (buffer_pos == state->cursor_position) {
        new_content->cursor_line = current_line;
        new_content->cursor_column = current_column;
        new_content->lines[current_line].contains_cursor = true;
        new_content->lines[current_line].cursor_column = current_column;
    }
    
    /* Set display content metadata */
    new_content->line_count = current_line + 1;
    new_content->cursor_visible = true;
    new_content->is_complete_refresh = state->command_buffer->needs_full_refresh ||
                                       generator->params.force_full_refresh;
    new_content->generation_time = lle_get_current_time_microseconds();
    new_content->content_version = (uint32_t)state->modification_count;
    
    /* Store previous content for delta calculation (future optimization) */
    if (generator->previous_content) {
        lle_display_content_destroy(generator->previous_content);
    }
    generator->previous_content = generator->current_content;
    generator->current_content = new_content;
    
    *content = new_content;
    return LLE_SUCCESS;
}
