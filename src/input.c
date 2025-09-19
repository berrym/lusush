/*
 * input.c - Lusush Input System using GNU Readline
 * 
 * This module provides unified input handling for both interactive and 
 * non-interactive modes, with complete GNU readline integration for
 * interactive sessions.
 */

#include "../include/input.h"
#include "../include/readline_integration.h"
#include "../include/lusush.h"
#include "../include/config.h"
#include "../include/errors.h"
#include "../include/posix_history.h"
#include "../include/prompt.h"
#include "../include/symtable.h"
#include "../include/init.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Input state for multiline parsing
typedef struct {
    int quote_count;
    int double_quote_count;
    int backtick_count;
    int paren_count;
    int brace_count;
    int bracket_count;
    bool in_single_quote;
    bool in_double_quote;
    bool in_backtick;
    bool escaped;
    bool in_here_doc;
    char *here_doc_delimiter;
    bool has_continuation;
    bool in_command_substitution;
    bool in_arithmetic;
    bool in_function_definition;
    bool in_case_statement;
    bool in_if_statement;
    bool in_while_loop;
    bool in_for_loop;
    bool in_until_loop;
    int compound_command_depth;
} input_state_t;

// Global state
static input_state_t global_state = {0};
static bool state_initialized = false;

// Forward declarations
static void init_input_state(input_state_t *state);
static void cleanup_input_state(input_state_t *state);
static void analyze_line(const char *line, input_state_t *state);
static bool is_input_complete(input_state_t *state);
static const char *get_continuation_prompt(input_state_t *state);
static bool is_control_keyword(const char *word);
static bool is_terminator(const char *line);

// Public function to get current continuation prompt
const char *lusush_get_current_continuation_prompt(void);
static char *convert_multiline_for_history(const char *input);

// ============================================================================
// INPUT STATE MANAGEMENT
// ============================================================================

static void init_input_state(input_state_t *state) {
    memset(state, 0, sizeof(input_state_t));
    state->here_doc_delimiter = NULL;
}

static void cleanup_input_state(input_state_t *state) {
    if (state->here_doc_delimiter) {
        free(state->here_doc_delimiter);
        state->here_doc_delimiter = NULL;
    }
    memset(state, 0, sizeof(input_state_t));
}

// ============================================================================
// MULTILINE INPUT ANALYSIS
// ============================================================================

static bool is_control_keyword(const char *word) {
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

static bool is_terminator(const char *line) {
    // Skip whitespace
    while (*line && isspace(*line)) line++;
    
    // Check for terminators
    return (strncmp(line, "fi", 2) == 0 ||
            strncmp(line, "done", 4) == 0 ||
            strncmp(line, "esac", 4) == 0 ||
            strcmp(line, "}") == 0);
}

static void analyze_line(const char *line, input_state_t *state) {
    if (!line || !state) return;
    
    const char *p = line;
    char word[256] = {0};
    int word_pos = 0;
    bool at_word_start = true;
    
    while (*p) {
        char c = *p;
        
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
            at_word_start = false;
        } else if (c == '{' || c == '}') {
            // Handle { and } as single-character keywords
            if (word_pos > 0) {
                // Process any accumulated word first
                word[word_pos] = '\0';
                
                // Check for control keywords
                if (is_control_keyword(word)) {
                    
                    if (strcmp(word, "if") == 0) {
                        state->in_if_statement = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "while") == 0) {
                        state->in_while_loop = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "for") == 0) {
                        state->in_for_loop = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "until") == 0) {
                        state->in_until_loop = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "case") == 0) {
                        state->in_case_statement = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "function") == 0) {
                        state->in_function_definition = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "fi") == 0) {
                        state->in_if_statement = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                    } else if (strcmp(word, "done") == 0) {
                        state->in_while_loop = false;
                        state->in_for_loop = false;
                        state->in_until_loop = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                    } else if (strcmp(word, "esac") == 0) {
                        state->in_case_statement = false;
                        state->has_continuation = false;
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
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
                }
            } else if (c == '}') {
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
                if (state->compound_command_depth == 0) {
                    state->in_function_definition = false;
                }
            }
        } else {
            if (word_pos > 0) {
                word[word_pos] = '\0';
                
                // Check for control keywords
                if (is_control_keyword(word)) {
                    
                    if (strcmp(word, "if") == 0) {
                        state->in_if_statement = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "while") == 0) {
                        state->in_while_loop = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "for") == 0) {
                        state->in_for_loop = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "until") == 0) {
                        state->in_until_loop = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "case") == 0) {
                        state->in_case_statement = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "function") == 0) {
                        state->in_function_definition = true;
                        state->compound_command_depth++;
                    } else if (strcmp(word, "{") == 0) {
                        state->compound_command_depth++;
                    } else if (strcmp(word, "fi") == 0) {
                        state->in_if_statement = false;
                        state->has_continuation = false;  // Clear continuation flag when closing if
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                    } else if (strcmp(word, "done") == 0) {
                        state->in_while_loop = false;
                        state->in_for_loop = false;
                        state->in_until_loop = false;
                        state->has_continuation = false;  // Clear continuation flag when closing loop
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                    } else if (strcmp(word, "esac") == 0) {
                        state->in_case_statement = false;
                        state->has_continuation = false;  // Clear continuation flag when closing case
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                    } else if (strcmp(word, "}") == 0) {
                        if (state->compound_command_depth > 0) {
                            state->compound_command_depth--;
                        }
                        if (state->compound_command_depth == 0) {
                            state->in_function_definition = false;
                        }
                    }
                }
                
                word_pos = 0;
                memset(word, 0, sizeof(word));
            }
        }
        
        // Check for line continuation
        if (c == '\\' && *(p + 1) == '\0') {
            state->has_continuation = true;
        }
        
        p++;
    }
    
    // Handle remaining word
    // Check final word at end of line if any
    if (word_pos > 0) {
        word[word_pos] = '\0';
        
        if (is_control_keyword(word)) {
            // Handle keywords found at end of line
            if (strcmp(word, "then") == 0 || strcmp(word, "do") == 0) {
                state->has_continuation = true;
            } else if (strcmp(word, "done") == 0) {
                state->in_while_loop = false;
                state->in_for_loop = false;
                state->in_until_loop = false;
                state->has_continuation = false;  // Clear continuation flag when closing loop
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
            } else if (strcmp(word, "esac") == 0) {
                state->in_case_statement = false;
                state->has_continuation = false;  // Clear continuation flag when closing case
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
            } else if (strcmp(word, "}") == 0) {
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
                if (state->compound_command_depth == 0) {
                    state->in_function_definition = false;
                }
            } else if (strcmp(word, "fi") == 0) {
                state->in_if_statement = false;
                state->has_continuation = false;  // Clear continuation flag when closing if
                if (state->compound_command_depth > 0) {
                    state->compound_command_depth--;
                }
            }
        }
    }
}

static bool is_input_complete(input_state_t *state) {
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

static const char *get_continuation_prompt(input_state_t *state) {
    if (!state) return "> ";
    
    // Use PS2 from symbol table, with fallback
    const char *ps2 = symtable_get_global_default("PS2", "> ");
    
    // Could customize based on state if desired
    if (state->in_single_quote || state->in_double_quote) {
        return "quote> ";
    } else if (state->in_function_definition) {
        return "function> ";
    } else if (state->in_if_statement) {
        return "if> ";
    } else if (state->in_while_loop || state->in_for_loop || state->in_until_loop) {
        return "loop> ";
    } else if (state->in_case_statement) {
        return "case> ";
    }
    
    return ps2;
}

// Public function to get current continuation prompt
const char *lusush_get_current_continuation_prompt(void) {
    if (!state_initialized) {
        return "$ ";  // Return primary prompt if not in multiline mode
    }
    
    // Check for any active multiline state indicators
    bool in_multiline = (global_state.in_single_quote || 
                        global_state.in_double_quote ||
                        global_state.in_backtick ||
                        global_state.paren_count > 0 ||
                        global_state.brace_count > 0 ||
                        global_state.bracket_count > 0 ||
                        global_state.in_if_statement ||
                        global_state.in_while_loop ||
                        global_state.in_for_loop ||
                        global_state.in_until_loop ||
                        global_state.in_case_statement ||
                        global_state.in_function_definition ||
                        global_state.compound_command_depth > 0);
    
    if (!in_multiline) {
        return "$ ";  // Return primary prompt
    }
    
    // Return appropriate continuation prompt
    return get_continuation_prompt(&global_state);
}

static char *convert_multiline_for_history(const char *input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    char *converted = malloc(len + 1);
    if (!converted) return NULL;
    
    char *dst = converted;
    const char *src = input;
    
    while (*src) {
        if (*src == '\n') {
            *dst++ = ' ';
        } else {
            *dst++ = *src;
        }
        src++;
    }
    
    *dst = '\0';
    return converted;
}

// ============================================================================
// PUBLIC INPUT FUNCTIONS
// ============================================================================

void free_input_buffers(void) {
    cleanup_input_state(&global_state);
    state_initialized = false;
}

char *get_input(FILE *in) {
    // For non-interactive input, read a line directly (single line only)
    if (!in) in = stdin;
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, in);
    
    if (read == -1) {
        free(line);
        return NULL;
    }
    
    // Remove trailing newline
    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }
    
    return line;
}

// Helper function to check if state needs continuation
static bool needs_continuation(input_state_t *state) {
    if (!state) return false;
    
    // Need continuation if we're in any compound structure
    if (state->compound_command_depth > 0) {
        return true;
    }
    
    // Need continuation if we're in any specific construct
    if (state->in_function_definition ||
        state->in_case_statement ||
        state->in_if_statement ||
        state->in_while_loop ||
        state->in_for_loop ||
        state->in_until_loop) {
        return true;
    }
    
    // Need continuation if we have pending quotes or escapes
    if (state->in_single_quote ||
        state->in_double_quote ||
        state->escaped ||
        state->has_continuation) {
        return true;
    }
    
    // Need continuation if we're in a here document
    if (state->in_here_doc) {
        return true;
    }
    
    // Need continuation if we have unmatched brackets
    if (state->paren_count > 0 ||
        state->bracket_count > 0 ||
        state->brace_count > 0) {
        return true;
    }
    
    return false;
}

char *ln_gets(void) {
    if (!is_interactive_shell()) {
        return get_input(stdin);
    }
    
    // Initialize state if needed
    if (!state_initialized) {
        init_input_state(&global_state);
        state_initialized = true;
    }
    
    static char *accumulated_input = NULL;
    static size_t accumulated_size = 0;
    static size_t accumulated_capacity = 0;
    
    char *line = NULL;
    bool first_line = (accumulated_size == 0);
    
    while (true) {
        errno = 0;
        
        // Get appropriate prompt
        const char *prompt;
        if (first_line) {
            prompt = NULL; // Let readline system generate themed prompt
        } else {
            prompt = get_continuation_prompt(&global_state);
        }
        
        // Get line using readline
        line = lusush_readline_with_prompt(prompt);
        if (!line) {
            // EOF or error
            if (accumulated_input && *accumulated_input) {
                // Return accumulated input and reset
                char *result = strdup(accumulated_input);
                free(accumulated_input);
                accumulated_input = NULL;
                accumulated_size = 0;
                accumulated_capacity = 0;
                init_input_state(&global_state);
                return result;
            }
            return NULL;
        }
        
        // Analyze this line to update state
        analyze_line(line, &global_state);
        
        // Handle accumulation
        size_t line_len = strlen(line);
        size_t needed_size = accumulated_size + line_len + 2; // +1 for newline, +1 for null
        
        if (needed_size > accumulated_capacity) {
            size_t new_capacity = accumulated_capacity ? accumulated_capacity * 2 : 1024;
            while (new_capacity < needed_size) {
                new_capacity *= 2;
            }
            
            char *tmp = realloc(accumulated_input, new_capacity);
            if (!tmp) {
                error_syscall("error: realloc in ln_gets");
                free(accumulated_input);
                accumulated_input = NULL;
                accumulated_size = 0;
                accumulated_capacity = 0;
                free(line);
                return NULL;
            }
            accumulated_input = tmp;
            accumulated_capacity = new_capacity;
        }
        
        if (accumulated_size == 0) {
            // First line
            strcpy(accumulated_input, line);
            accumulated_size = line_len;
        } else {
            // Append with newline
            strcat(accumulated_input, "\n");
            strcat(accumulated_input, line);
            accumulated_size += line_len + 1;
        }
        
        // Free individual line (readline allocates it)
        free(line);
        line = NULL;
        
        // Check if input is complete
        if (is_input_complete(&global_state)) {
            char *result = accumulated_input;
            accumulated_input = NULL;
            accumulated_size = 0;
            accumulated_capacity = 0;
            
            // Reset state for next input
            cleanup_input_state(&global_state);
            init_input_state(&global_state);
            
            // Note: History is handled by readline system automatically
            return result;
        }
        
        first_line = false;
    }
}

char *get_input_complete(FILE *in) {
    // For non-interactive mode, accumulate lines for complete constructs
    if (!in) in = stdin;
    
    char *accumulated = NULL;
    size_t accumulated_len = 0;
    input_state_t state = {0};
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while ((read = getline(&line, &len, in)) != -1) {
        // Remove trailing newline for analysis
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }
        
        // Analyze this line to update state
        analyze_line(line, &state);
        
        // Accumulate the line
        if (accumulated == NULL) {
            accumulated = malloc(read + 2); // +2 for newline and null terminator
            if (!accumulated) {
                free(line);
                return NULL;
            }
            strcpy(accumulated, line);
            accumulated_len = read;
        } else {
            size_t new_len = accumulated_len + read + 2; // +2 for newline and null terminator
            char *new_accumulated = realloc(accumulated, new_len);
            if (!new_accumulated) {
                free(accumulated);
                free(line);
                return NULL;
            }
            accumulated = new_accumulated;
            strcat(accumulated, "\n");
            strcat(accumulated, line);
            accumulated_len = new_len - 1;
        }
        
        // Check if we have a complete construct
        if (!needs_continuation(&state)) {
            break;
        }
    }

    free(line);
    return accumulated;
}

char *get_unified_input(FILE *in) {
    if (is_interactive_shell()) {
        // Interactive mode - use readline-based input with multiline support
        return ln_gets();
    } else {
        // Non-interactive mode - use file input
        return get_input_complete(in);
    }
}

// Legacy compatibility
char *ln_gets_complete(void) {
    return ln_gets();
}