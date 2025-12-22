/*
 * input_continuation.c - Shared Multiline Input Continuation System
 * 
 * This module provides shared multiline parsing functionality for both the
 * Line Editing Engine (LLE) and the main input system.
 * 
 * UTF-8 Support:
 * This module properly handles multi-byte UTF-8 characters. While shell
 * syntax characters (quotes, brackets, etc.) are all ASCII, we must
 * properly skip over UTF-8 multi-byte sequences to avoid misinterpreting
 * continuation bytes as syntax characters.
 */

#include "../include/input_continuation.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// UTF-8 HELPER (self-contained, no LLE dependency)
// ============================================================================

/**
 * Get the byte length of a UTF-8 character from its first byte.
 * 
 * UTF-8 encoding:
 *   0xxxxxxx = 1 byte  (ASCII, 0x00-0x7F)
 *   110xxxxx = 2 bytes (0xC0-0xDF)
 *   1110xxxx = 3 bytes (0xE0-0xEF)
 *   11110xxx = 4 bytes (0xF0-0xF7)
 * 
 * Returns 1 for ASCII, 2-4 for multi-byte, 1 for invalid (safe fallback).
 */
static int utf8_char_len(const char *p) {
    if (!p || !*p) return 0;
    
    unsigned char c = (unsigned char)*p;
    
    // ASCII (0x00-0x7F) - single byte
    if (c < 0x80) return 1;
    
    // Continuation byte (0x80-0xBF) - invalid as first byte, skip 1
    if (c < 0xC0) return 1;
    
    // 2-byte sequence (0xC0-0xDF)
    if (c < 0xE0) {
        // Validate: next byte must be continuation (10xxxxxx)
        if (((unsigned char)p[1] & 0xC0) == 0x80) {
            return 2;
        }
        return 1;  // Invalid, skip 1
    }
    
    // 3-byte sequence (0xE0-0xEF)
    if (c < 0xF0) {
        // Validate: next 2 bytes must be continuations
        if (((unsigned char)p[1] & 0xC0) == 0x80 &&
            ((unsigned char)p[2] & 0xC0) == 0x80) {
            return 3;
        }
        return 1;  // Invalid, skip 1
    }
    
    // 4-byte sequence (0xF0-0xF7)
    if (c < 0xF8) {
        // Validate: next 3 bytes must be continuations
        if (((unsigned char)p[1] & 0xC0) == 0x80 &&
            ((unsigned char)p[2] & 0xC0) == 0x80 &&
            ((unsigned char)p[3] & 0xC0) == 0x80) {
            return 4;
        }
        return 1;  // Invalid, skip 1
    }
    
    // Invalid first byte (0xF8-0xFF), skip 1
    return 1;
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

void continuation_state_init(continuation_state_t *state) {
    if (!state) return;
    memset(state, 0, sizeof(continuation_state_t));
    state->here_doc_delimiter = NULL;
}

void continuation_state_cleanup(continuation_state_t *state) {
    if (!state) return;
    
    if (state->here_doc_delimiter) {
        free(state->here_doc_delimiter);
        state->here_doc_delimiter = NULL;
    }
    memset(state, 0, sizeof(continuation_state_t));
}

// ============================================================================
// CONTEXT STACK MANAGEMENT
// ============================================================================

/**
 * Push a context onto the stack
 */
static void context_stack_push(continuation_state_t *state, continuation_context_type_t ctx) {
    if (!state) return;
    if (state->context_stack_depth >= CONTINUATION_MAX_CONTEXT_DEPTH) return;
    
    state->context_stack[state->context_stack_depth++] = ctx;
}

/**
 * Pop a context from the stack if it matches the expected type
 * Returns true if popped successfully
 */
static bool context_stack_pop(continuation_state_t *state, continuation_context_type_t expected) {
    if (!state || state->context_stack_depth <= 0) return false;
    
    // Check if top matches expected (for proper nesting validation)
    continuation_context_type_t top = state->context_stack[state->context_stack_depth - 1];
    if (top == expected) {
        state->context_stack_depth--;
        return true;
    }
    
    // For loops, accept any loop type (for/while/until all end with 'done')
    if (expected == CONTEXT_FOR || expected == CONTEXT_WHILE || expected == CONTEXT_UNTIL) {
        if (top == CONTEXT_FOR || top == CONTEXT_WHILE || top == CONTEXT_UNTIL) {
            state->context_stack_depth--;
            return true;
        }
    }
    
    return false;
}

/**
 * Pop any loop context from the stack (for 'done' keyword)
 */
static bool context_stack_pop_loop(continuation_state_t *state) {
    if (!state || state->context_stack_depth <= 0) return false;
    
    continuation_context_type_t top = state->context_stack[state->context_stack_depth - 1];
    if (top == CONTEXT_FOR || top == CONTEXT_WHILE || top == CONTEXT_UNTIL) {
        state->context_stack_depth--;
        return true;
    }
    return false;
}

/**
 * Get the current (topmost) context
 */
static continuation_context_type_t context_stack_top(const continuation_state_t *state) {
    if (!state || state->context_stack_depth <= 0) return CONTEXT_NONE;
    return state->context_stack[state->context_stack_depth - 1];
}

// ============================================================================
// KEYWORD DETECTION
// ============================================================================

bool continuation_is_control_keyword(const char *word) {
    if (!word) return false;
    
    const char *keywords[] = {
        "if", "then", "else", "elif", "fi",
        "case", "esac", "while", "until", "do", "done",
        "for", "in", "function", "select", "{", "}"
    };
    
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool continuation_is_terminator(const char *line) {
    if (!line) return false;
    
    // Skip whitespace
    while (*line && isspace(*line)) line++;
    
    // Check for terminators
    return (strncmp(line, "fi", 2) == 0 ||
            strncmp(line, "done", 4) == 0 ||
            strncmp(line, "esac", 4) == 0 ||
            strcmp(line, "}") == 0);
}

// ============================================================================
// LINE ANALYSIS
// ============================================================================

void continuation_analyze_line(const char *line, continuation_state_t *state) {
    if (!line || !state) return;
    
    const char *p = line;
    char word[256] = {0};
    int word_pos = 0;
    
    while (*p) {
        unsigned char uc = (unsigned char)*p;
        char c = *p;
        
        // UTF-8 multi-byte sequence handling:
        // If this is a non-ASCII byte (high bit set), it's part of a UTF-8
        // multi-byte character. Skip the entire sequence since shell syntax
        // characters are all ASCII. This prevents misinterpreting UTF-8
        // continuation bytes as shell metacharacters.
        if (uc >= 0x80) {
            int char_len = utf8_char_len(p);
            
            // If we're collecting a word, flush it first (UTF-8 chars break words
            // for keyword detection purposes - keywords are ASCII only)
            if (word_pos > 0) {
                word[word_pos] = '\0';
                // Keywords are ASCII-only, so no need to check here
                word_pos = 0;
                memset(word, 0, sizeof(word));
            }
            
            // Skip the entire UTF-8 sequence
            p += (char_len > 0) ? char_len : 1;
            continue;
        }
        
        // Handle escape sequences
        if (state->escaped) {
            state->escaped = false;
            p++;
            continue;
        }
        
        if (c == '\\') {
            state->escaped = true;
            p++;
            continue;
        }
        
        // Handle quotes
        if (c == '\'' && !state->in_double_quote && !state->in_backtick) {
            state->in_single_quote = !state->in_single_quote;
            if (state->in_single_quote) {
                state->quote_count++;
            }
        } else if (c == '"' && !state->in_single_quote) {
            state->in_double_quote = !state->in_double_quote;
            if (state->in_double_quote) {
                state->double_quote_count++;
            }
        } else if (c == '`' && !state->in_single_quote) {
            state->in_backtick = !state->in_backtick;
            if (state->in_backtick) {
                state->backtick_count++;
            }
        }
        
        // Skip if we're in quotes
        if (state->in_single_quote || state->in_double_quote || state->in_backtick) {
            p++;
            continue;
        }
        
        // Handle parentheses, braces, brackets
        if (c == '(') {
            state->paren_count++;
        } else if (c == ')') {
            state->paren_count--;
        } else if (c == '{') {
            state->brace_count++;
        } else if (c == '}') {
            state->brace_count--;
        } else if (c == '[') {
            state->bracket_count++;
        } else if (c == ']') {
            state->bracket_count--;
        }
        
        // Handle here document detection
        if (c == '<' && *(p+1) == '<' && !state->in_here_doc) {
            // Found <<, look for delimiter
            const char *delim_start = p + 2;
            
            // Skip optional '-' for <<-
            if (*delim_start == '-') {
                delim_start++;
            }
            
            // Skip whitespace
            while (*delim_start == ' ' || *delim_start == '\t') {
                delim_start++;
            }
            
            // Extract delimiter (up to end of line or whitespace)
            const char *delim_end = delim_start;
            while (*delim_end && *delim_end != '\n' && *delim_end != ' ' && *delim_end != '\t') {
                delim_end++;
            }
            
            if (delim_end > delim_start) {
                // Handle quoted delimiters - strip surrounding quotes
                const char *actual_delim_start = delim_start;
                const char *actual_delim_end = delim_end;
                
                // Check for single or double quotes
                if ((*delim_start == '\'' || *delim_start == '"') && 
                    delim_end > delim_start + 1 && 
                    *(delim_end - 1) == *delim_start) {
                    // Strip quotes
                    actual_delim_start++;
                    actual_delim_end--;
                }
                
                // Found a delimiter, enter here document mode
                state->in_here_doc = true;
                if (state->here_doc_delimiter) {
                    free(state->here_doc_delimiter);
                }
                size_t delim_len = actual_delim_end - actual_delim_start;
                state->here_doc_delimiter = malloc(delim_len + 1);
                if (state->here_doc_delimiter) {
                    strncpy(state->here_doc_delimiter, actual_delim_start, delim_len);
                    state->here_doc_delimiter[delim_len] = '\0';
                }
            }
        }
        
        // Check if current line is a here document delimiter (ends here doc)
        if (state->in_here_doc && state->here_doc_delimiter) {
            // Check if this entire line matches the delimiter
            const char *line_start = line;
            while (*line_start == ' ' || *line_start == '\t') {
                line_start++; // Skip leading whitespace
            }
            
            if (strncmp(line_start, state->here_doc_delimiter, strlen(state->here_doc_delimiter)) == 0) {
                // Check if delimiter is followed by end of line or whitespace
                const char *after_delim = line_start + strlen(state->here_doc_delimiter);
                if (*after_delim == '\0' || *after_delim == '\n' || *after_delim == ' ' || *after_delim == '\t') {
                    // This line is the delimiter, end here document
                    state->in_here_doc = false;
                    free(state->here_doc_delimiter);
                    state->here_doc_delimiter = NULL;
                }
            }
        }
        
        // Collect words for keyword analysis
        if (isalnum(c) || c == '_') {
            if (word_pos < (int)sizeof(word) - 1) {
                word[word_pos++] = c;
            }
        } else if (c == '{' || c == '}') {
            // Handle { and } as single-character keywords
            if (word_pos > 0) {
                // Process any accumulated word first
                word[word_pos] = '\0';
                
                // Check for control keywords
                if (continuation_is_control_keyword(word)) {
                    
                    if (strcmp(word, "if") == 0) {
                        state->in_if_statement = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_IF);
                    } else if (strcmp(word, "while") == 0) {
                        state->in_while_loop = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_WHILE);
                    } else if (strcmp(word, "for") == 0) {
                        state->in_for_loop = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_FOR);
                    } else if (strcmp(word, "until") == 0) {
                        state->in_until_loop = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_UNTIL);
                    } else if (strcmp(word, "case") == 0) {
                        state->in_case_statement = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_CASE);
                    } else if (strcmp(word, "function") == 0) {
                        state->in_function_definition = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_FUNCTION);
                    } else if (strcmp(word, "fi") == 0) {
                        state->in_if_statement = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                        context_stack_pop(state, CONTEXT_IF);
                    } else if (strcmp(word, "done") == 0) {
                        state->in_while_loop = false;
                        state->in_for_loop = false;
                        state->in_until_loop = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                        context_stack_pop_loop(state);
                    } else if (strcmp(word, "esac") == 0) {
                        state->in_case_statement = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                        context_stack_pop(state, CONTEXT_CASE);
                    }
                }
                
                word_pos = 0;
                memset(word, 0, sizeof(word));
            }
            
            // Now handle the { or } character as a single-character keyword
            if (c == '{') {
                // Only increment depth if not already in a function definition
                // (function keyword already incremented depth)
                if (!state->in_function_definition) {
                    state->compound_command_depth++;
                    context_stack_push(state, CONTEXT_BRACE_GROUP);
                }
            } else if (c == '}') {
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
                context_stack_pop(state, CONTEXT_BRACE_GROUP);
                // Also try popping function context
                if (state->context_stack_depth > 0 && 
                    context_stack_top(state) == CONTEXT_FUNCTION) {
                    context_stack_pop(state, CONTEXT_FUNCTION);
                }
                if (state->compound_command_depth == 0) {
                    state->in_function_definition = false;
                }
            }
        } else {
            if (word_pos > 0) {
                word[word_pos] = '\0';
                
                // Check for control keywords
                if (continuation_is_control_keyword(word)) {
                    
                    if (strcmp(word, "if") == 0) {
                        state->in_if_statement = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_IF);
                    } else if (strcmp(word, "while") == 0) {
                        state->in_while_loop = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_WHILE);
                    } else if (strcmp(word, "for") == 0) {
                        state->in_for_loop = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_FOR);
                    } else if (strcmp(word, "until") == 0) {
                        state->in_until_loop = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_UNTIL);
                    } else if (strcmp(word, "case") == 0) {
                        state->in_case_statement = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_CASE);
                    } else if (strcmp(word, "function") == 0) {
                        state->in_function_definition = true;
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_FUNCTION);
                    } else if (strcmp(word, "{") == 0) {
                        state->compound_command_depth++;
                        context_stack_push(state, CONTEXT_BRACE_GROUP);
                    } else if (strcmp(word, "fi") == 0) {
                        state->in_if_statement = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                        context_stack_pop(state, CONTEXT_IF);
                    } else if (strcmp(word, "done") == 0) {
                        state->in_while_loop = false;
                        state->in_for_loop = false;
                        state->in_until_loop = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                        context_stack_pop_loop(state);
                    } else if (strcmp(word, "esac") == 0) {
                        state->in_case_statement = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                        context_stack_pop(state, CONTEXT_CASE);
                    } else if (strcmp(word, "}") == 0) {
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                        context_stack_pop(state, CONTEXT_BRACE_GROUP);
                        if (state->compound_command_depth == 0) {
                            state->in_function_definition = false;
                        }
                    }
                }
                
                word_pos = 0;
                memset(word, 0, sizeof(word));
            }
        }
        
        p++;
    }
    
    // Check if line ends with unescaped backslash (line continuation)
    // Must check after loop completes to handle escaped flag correctly
    if (state->escaped) {
        state->has_continuation = true;
        state->escaped = false;  // Reset for next line
    }
    
    // Check if line ends with pipe character (requires continuation)
    // Need to check backwards from end, skipping whitespace
    const char *end = line + strlen(line);
    while (end > line && isspace(*(end - 1))) {
        end--;
    }
    if (end > line && *(end - 1) == '|') {
        // Line ends with pipe - needs continuation
        state->has_continuation = true;
    }
    
    // Handle remaining word at end of line
    if (word_pos > 0) {
        word[word_pos] = '\0';
        
        if (continuation_is_control_keyword(word)) {
            // Handle keywords found at end of line
            if (strcmp(word, "then") == 0 || strcmp(word, "do") == 0) {
                state->has_continuation = true;
            } else if (strcmp(word, "done") == 0) {
                state->in_while_loop = false;
                state->in_for_loop = false;
                state->in_until_loop = false;
                state->has_continuation = false;
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
                context_stack_pop_loop(state);
            } else if (strcmp(word, "esac") == 0) {
                state->in_case_statement = false;
                state->has_continuation = false;
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
                context_stack_pop(state, CONTEXT_CASE);
            } else if (strcmp(word, "}") == 0) {
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
                context_stack_pop(state, CONTEXT_BRACE_GROUP);
                if (state->compound_command_depth == 0) {
                    state->in_function_definition = false;
                }
            } else if (strcmp(word, "fi") == 0) {
                state->in_if_statement = false;
                state->has_continuation = false;
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
                context_stack_pop(state, CONTEXT_IF);
            }
        }
    }
}

// ============================================================================
// COMPLETION CHECKING
// ============================================================================

bool continuation_is_complete(const continuation_state_t *state) {
    if (!state) return true;
    
    // Check for unmatched quotes
    if (state->in_single_quote || state->in_double_quote || state->in_backtick) {
        return false;
    }
    
    // Check for unmatched parentheses, braces, brackets
    if (state->paren_count > 0 || state->brace_count > 0 || state->bracket_count > 0) {
        return false;
    }
    
    // Check for incomplete compound commands
    if (state->compound_command_depth > 0) {
        return false;
    }
    
    // Check for line continuation
    if (state->has_continuation) {
        return false;
    }
    
    // Check for incomplete control structures
    if (state->in_if_statement || state->in_while_loop || state->in_for_loop ||
        state->in_until_loop || state->in_case_statement || state->in_function_definition) {
        return false;
    }
    
    // Check for here documents
    if (state->in_here_doc) {
        return false;
    }
    
    return true;
}

bool continuation_needs_continuation(const continuation_state_t *state) {
    return !continuation_is_complete(state);
}

// ============================================================================
// PROMPT GENERATION
// ============================================================================

const char *continuation_get_prompt(const continuation_state_t *state) {
    if (!state) return "> ";
    
    // Use PS2 from symbol table, with fallback
    const char *ps2 = symtable_get_global_default("PS2", "> ");
    
    // Quote state takes highest priority (not tracked in context stack)
    if (state->in_single_quote || state->in_double_quote) {
        return "quote> ";
    }
    
    // Use context stack for proper nested construct tracking
    // The top of the stack represents the innermost active construct
    continuation_context_type_t current_ctx = context_stack_top(state);
    
    switch (current_ctx) {
        case CONTEXT_IF:
            return "if> ";
        case CONTEXT_WHILE:
            return "while> ";
        case CONTEXT_FOR:
            return "for> ";
        case CONTEXT_UNTIL:
            return "until> ";
        case CONTEXT_CASE:
            return "case> ";
        case CONTEXT_FUNCTION:
            return "func> ";
        case CONTEXT_BRACE_GROUP:
            return "brace> ";
        case CONTEXT_NONE:
        default:
            break;
    }
    
    // Fallback to legacy boolean flags for backwards compatibility
    // (in case context stack isn't being used consistently)
    if (state->in_function_definition) {
        return "func> ";
    } else if (state->in_if_statement) {
        return "if> ";
    } else if (state->in_while_loop) {
        return "while> ";
    } else if (state->in_for_loop) {
        return "for> ";
    } else if (state->in_until_loop) {
        return "until> ";
    } else if (state->in_case_statement) {
        return "case> ";
    }
    
    return ps2;
}
