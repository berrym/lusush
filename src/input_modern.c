/**
 * Modern Input System Implementation
 * 
 * Provides clean, efficient input handling with multiline support,
 * history integration, and proper POSIX compliance.
 * Designed to work seamlessly with the modern tokenizer and parser.
 */

#include "../include/input_modern.h"
#include "../include/linenoise/linenoise.h"
#include "../include/prompt.h"
#include "../include/strings.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

// Default buffer size for input
#define INPUT_BUFFER_INITIAL_SIZE 1024
#define INPUT_BUFFER_MAX_SIZE (1024 * 1024)  // 1MB max
#define INPUT_LINE_MAX 4096

// Default prompts
#define DEFAULT_PROMPT "$ "
#define DEFAULT_CONTINUATION_PROMPT "> "

// Forward declarations
static bool analyze_line_completeness(input_context_t *context, const char *line);
static void update_quote_state(input_context_t *context, const char *line);
static void update_control_depth(input_context_t *context, const char *line);
static void update_bracket_depth(input_context_t *context, const char *line);
static bool is_line_continuation(const char *line);
static char *convert_multiline_for_history(const char *input);
static void set_error(input_context_t *context, const char *message);
static bool ensure_buffer_capacity(input_context_t *context, size_t additional_size);

// Create new input context
input_context_t *input_context_new(FILE *input_stream, bool interactive) {
    input_context_t *context = malloc(sizeof(input_context_t));
    if (!context) {
        return NULL;
    }
    
    // Initialize core state
    memset(context, 0, sizeof(input_context_t));
    context->interactive = interactive;
    context->multiline_enabled = true;
    context->input_stream = input_stream ? input_stream : stdin;
    
    // Initialize buffer
    context->buffer_size = INPUT_BUFFER_INITIAL_SIZE;
    context->buffer = malloc(context->buffer_size);
    if (!context->buffer) {
        free(context);
        return NULL;
    }
    context->buffer[0] = '\0';
    context->buffer_used = 0;
    
    // Set default prompts
    context->prompt = strdup(DEFAULT_PROMPT);
    context->continuation_prompt = strdup(DEFAULT_CONTINUATION_PROMPT);
    
    // Enable history by default for interactive mode
    context->history_enabled = interactive;
    
    return context;
}

// Free input context
void input_context_free(input_context_t *context) {
    if (!context) {
        return;
    }
    
    if (context->buffer) {
        free(context->buffer);
    }
    if (context->prompt) {
        free(context->prompt);
    }
    if (context->continuation_prompt) {
        free(context->continuation_prompt);
    }
    if (context->here_doc_delimiter) {
        free(context->here_doc_delimiter);
    }
    if (context->history_line) {
        free(context->history_line);
    }
    if (context->error_message) {
        free(context->error_message);
    }
    
    free(context);
}

// Main input function - returns complete command line ready for parsing
char *input_get_command_line(input_context_t *context) {
    if (!context) {
        return NULL;
    }
    
    // Clear previous state
    input_reset_state(context);
    input_clear_buffer(context);
    input_clear_error(context);
    
    char *line = NULL;
    bool command_complete = false;
    
    while (!command_complete) {
        // Get next line of input
        if (context->interactive) {
            line = input_get_line_interactive(context);
        } else {
            line = input_get_line_non_interactive(context);
        }
        
        // Handle EOF or error
        if (!line) {
            if (context->buffer_used > 0) {
                // Return partial buffer if we have content
                char *result = strdup(context->buffer);
                input_clear_buffer(context);
                return result;
            }
            return NULL;
        }
        
        // Skip empty lines and comments at the start
        if (context->buffer_used == 0 && input_is_empty_or_comment(line)) {
            free(line);
            continue;
        }
        
        // Append line to buffer
        if (!input_append_to_buffer(context, line)) {
            free(line);
            set_error(context, "Buffer overflow - command too large");
            return NULL;
        }
        
        // Update multiline state
        input_update_state(context, line);
        
        // Check if command is complete
        command_complete = input_is_complete_command(context, context->buffer);
        
        free(line);
    }
    
    // Prepare result
    char *result = strdup(context->buffer);
    
    // Prepare history entry if enabled
    if (context->history_enabled && result) {
        if (context->history_line) {
            free(context->history_line);
        }
        context->history_line = input_prepare_for_history(context, result);
    }
    
    return result;
}

// Check if command is complete (doesn't need continuation)
bool input_is_complete_command(input_context_t *context, const char *line) {
    if (!context || !line) {
        return true;
    }
    
    // If multiline is disabled, always complete
    if (!context->multiline_enabled) {
        return true;
    }
    
    // Check various completion conditions
    if (context->in_single_quote || context->in_double_quote) {
        return false;
    }
    
    if (context->control_depth > 0) {
        return false;
    }
    
    if (context->brace_depth > 0 || context->paren_depth > 0 || context->bracket_depth > 0) {
        return false;
    }
    
    if (context->in_here_doc) {
        return false;
    }
    
    if (context->in_function_def) {
        return false;
    }
    
    if (context->line_continuation) {
        return false;
    }
    
    return true;
}

// Check if line needs continuation
bool input_needs_continuation(input_context_t *context, const char *line) {
    return !input_is_complete_command(context, line);
}

// Update multiline state based on new line
void input_update_state(input_context_t *context, const char *line) {
    if (!context || !line) {
        return;
    }
    
    update_quote_state(context, line);
    update_control_depth(context, line);
    update_bracket_depth(context, line);
    
    // Check for line continuation
    context->line_continuation = is_line_continuation(line);
    
    // Check for function definition
    if (strstr(line, "function ") || strstr(line, "() {")) {
        context->in_function_def = true;
    }
    
    // Check if function definition ends
    if (context->in_function_def && context->brace_depth == 0 && 
        !context->in_single_quote && !context->in_double_quote) {
        context->in_function_def = false;
    }
}

// Reset multiline state
void input_reset_state(input_context_t *context) {
    if (!context) {
        return;
    }
    
    context->control_depth = 0;
    context->brace_depth = 0;
    context->paren_depth = 0;
    context->bracket_depth = 0;
    context->in_single_quote = false;
    context->in_double_quote = false;
    context->in_here_doc = false;
    context->escaped = false;
    context->in_function_def = false;
    context->in_arithmetic = false;
    context->in_command_subst = false;
    context->command_subst_depth = 0;
    context->line_continuation = false;
    
    if (context->here_doc_delimiter) {
        free(context->here_doc_delimiter);
        context->here_doc_delimiter = NULL;
    }
}

// Set prompt string
void input_set_prompt(input_context_t *context, const char *prompt) {
    if (!context) {
        return;
    }
    
    if (context->prompt) {
        free(context->prompt);
    }
    context->prompt = prompt ? strdup(prompt) : strdup(DEFAULT_PROMPT);
}

// Set continuation prompt string
void input_set_continuation_prompt(input_context_t *context, const char *prompt) {
    if (!context) {
        return;
    }
    
    if (context->continuation_prompt) {
        free(context->continuation_prompt);
    }
    context->continuation_prompt = prompt ? strdup(prompt) : strdup(DEFAULT_CONTINUATION_PROMPT);
}

// Get current prompt (normal or continuation)
const char *input_get_current_prompt(input_context_t *context) {
    if (!context) {
        return DEFAULT_PROMPT;
    }
    
    // Use continuation prompt if we have partial input
    if (context->buffer_used > 0) {
        return context->continuation_prompt;
    }
    
    return context->prompt;
}

// Enable/disable history
void input_set_history_enabled(input_context_t *context, bool enabled) {
    if (context) {
        context->history_enabled = enabled;
    }
}

// Prepare command for history storage
char *input_prepare_for_history(input_context_t *context, const char *command) {
    if (!context || !command) {
        return NULL;
    }
    
    return convert_multiline_for_history(command);
}

// Clear input buffer
void input_clear_buffer(input_context_t *context) {
    if (!context || !context->buffer) {
        return;
    }
    
    context->buffer[0] = '\0';
    context->buffer_used = 0;
}

// Append line to buffer
bool input_append_to_buffer(input_context_t *context, const char *line) {
    if (!context || !line) {
        return false;
    }
    
    size_t line_len = strlen(line);
    size_t needed = context->buffer_used + line_len + 2; // +2 for \n and \0
    
    if (!ensure_buffer_capacity(context, needed)) {
        return false;
    }
    
    // Add newline if buffer is not empty
    if (context->buffer_used > 0) {
        context->buffer[context->buffer_used++] = '\n';
    }
    
    // Append line
    strcpy(context->buffer + context->buffer_used, line);
    context->buffer_used += line_len;
    
    return true;
}

// Get buffer content
const char *input_get_buffer_content(input_context_t *context) {
    if (!context || !context->buffer) {
        return "";
    }
    
    return context->buffer;
}

// Check if context is interactive
bool input_is_interactive(input_context_t *context) {
    return context ? context->interactive : false;
}

// Check if context has error
bool input_has_error(input_context_t *context) {
    return context ? context->has_error : false;
}

// Get error message
const char *input_get_error_message(input_context_t *context) {
    if (!context || !context->error_message) {
        return "Unknown error";
    }
    
    return context->error_message;
}

// Clear error state
void input_clear_error(input_context_t *context) {
    if (!context) {
        return;
    }
    
    context->has_error = false;
    if (context->error_message) {
        free(context->error_message);
        context->error_message = NULL;
    }
}

// Check if word is a control keyword
bool input_is_control_keyword(const char *word) {
    if (!word) {
        return false;
    }
    
    const char *keywords[] = {
        "if", "then", "else", "elif", "fi",
        "while", "do", "done", "until",
        "for", "in", "case", "esac",
        "function", "{", "}", NULL
    };
    
    for (int i = 0; keywords[i]; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// Check if word opens a control structure
bool input_is_opening_control(const char *word) {
    if (!word) {
        return false;
    }
    
    const char *opening[] = {
        "if", "while", "until", "for", "case", "function", "{", NULL
    };
    
    for (int i = 0; opening[i]; i++) {
        if (strcmp(word, opening[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// Check if word closes a control structure
bool input_is_closing_control(const char *word) {
    if (!word) {
        return false;
    }
    
    const char *closing[] = {
        "fi", "done", "esac", "}", NULL
    };
    
    for (int i = 0; closing[i]; i++) {
        if (strcmp(word, closing[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// Check if word indicates continuation
bool input_is_continuation_word(const char *word) {
    if (!word) {
        return false;
    }
    
    const char *continuation[] = {
        "then", "do", "else", "elif", NULL
    };
    
    for (int i = 0; continuation[i]; i++) {
        if (strcmp(word, continuation[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// Trim whitespace from line
char *input_trim_whitespace(const char *line) {
    if (!line) {
        return NULL;
    }
    
    // Skip leading whitespace
    while (isspace(*line)) {
        line++;
    }
    
    if (*line == '\0') {
        return strdup("");
    }
    
    // Find end of string
    const char *end = line + strlen(line) - 1;
    while (end > line && isspace(*end)) {
        end--;
    }
    
    // Create trimmed string
    size_t len = end - line + 1;
    char *result = malloc(len + 1);
    if (result) {
        strncpy(result, line, len);
        result[len] = '\0';
    }
    
    return result;
}

// Remove comments from line
char *input_remove_comments(const char *line) {
    if (!line) {
        return NULL;
    }
    
    // Find first # not in quotes
    bool in_single = false, in_double = false;
    const char *p = line;
    
    while (*p) {
        if (*p == '\'' && !in_double) {
            in_single = !in_single;
        } else if (*p == '"' && !in_single) {
            in_double = !in_double;
        } else if (*p == '#' && !in_single && !in_double) {
            // Found comment
            size_t len = p - line;
            char *result = malloc(len + 1);
            if (result) {
                strncpy(result, line, len);
                result[len] = '\0';
            }
            return result;
        }
        p++;
    }
    
    return strdup(line);
}

// Check if line is empty or comment only
bool input_is_empty_or_comment(const char *line) {
    if (!line) {
        return true;
    }
    
    char *trimmed = input_trim_whitespace(line);
    if (!trimmed) {
        return true;
    }
    
    bool result = (trimmed[0] == '\0' || trimmed[0] == '#');
    free(trimmed);
    
    return result;
}

// Get line in interactive mode
char *input_get_line_interactive(input_context_t *context) {
    if (!context) {
        return NULL;
    }
    
    const char *prompt = input_get_current_prompt(context);
    char *line = linenoise(prompt);
    
    if (line && context->history_enabled && strlen(line) > 0) {
        linenoiseHistoryAdd(line);
    }
    
    return line;
}

// Get line in non-interactive mode
char *input_get_line_non_interactive(input_context_t *context) {
    if (!context || !context->input_stream) {
        return NULL;
    }
    
    char *line = malloc(INPUT_LINE_MAX);
    if (!line) {
        return NULL;
    }
    
    if (fgets(line, INPUT_LINE_MAX, context->input_stream) == NULL) {
        free(line);
        return NULL;
    }
    
    // Remove trailing newline
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    
    return line;
}

// Debug state information
void input_debug_state(input_context_t *context) {
    if (!context) {
        return;
    }
    
    printf("DEBUG: Input State:\n");
    printf("  Interactive: %s\n", context->interactive ? "yes" : "no");
    printf("  Control depth: %d\n", context->control_depth);
    printf("  Brace depth: %d\n", context->brace_depth);
    printf("  Paren depth: %d\n", context->paren_depth);
    printf("  Bracket depth: %d\n", context->bracket_depth);
    printf("  In quotes: single=%s double=%s\n", 
           context->in_single_quote ? "yes" : "no",
           context->in_double_quote ? "yes" : "no");
    printf("  Buffer used: %zu/%zu\n", context->buffer_used, context->buffer_size);
    printf("  Complete: %s\n", 
           input_is_complete_command(context, context->buffer) ? "yes" : "no");
}

// Set debug mode
void input_set_debug_mode(input_context_t *context, bool debug) {
    // This could be implemented to enable debug output
    (void)context;
    (void)debug;
}

// --- Static helper functions ---

// Analyze line for completeness
static bool analyze_line_completeness(input_context_t *context, const char *line) {
    // This is handled by the main state update functions
    (void)context;
    (void)line;
    return true;
}

// Update quote state based on line
static void update_quote_state(input_context_t *context, const char *line) {
    if (!context || !line) {
        return;
    }
    
    const char *p = line;
    while (*p) {
        if (context->escaped) {
            context->escaped = false;
        } else if (*p == '\\') {
            context->escaped = true;
        } else if (*p == '\'' && !context->in_double_quote) {
            context->in_single_quote = !context->in_single_quote;
        } else if (*p == '"' && !context->in_single_quote) {
            context->in_double_quote = !context->in_double_quote;
        }
        p++;
    }
}

// Update control structure depth
static void update_control_depth(input_context_t *context, const char *line) {
    if (!context || !line || context->in_single_quote || context->in_double_quote) {
        return;
    }
    
    // Simple word-based analysis
    char *line_copy = strdup(line);
    if (!line_copy) {
        return;
    }
    
    char *word = strtok(line_copy, " \t\n");
    while (word) {
        if (input_is_opening_control(word)) {
            context->control_depth++;
        } else if (input_is_closing_control(word)) {
            if (context->control_depth > 0) {
                context->control_depth--;
            }
        }
        word = strtok(NULL, " \t\n");
    }
    
    free(line_copy);
}

// Update bracket depth
static void update_bracket_depth(input_context_t *context, const char *line) {
    if (!context || !line) {
        return;
    }
    
    const char *p = line;
    while (*p) {
        if (context->escaped) {
            context->escaped = false;
        } else if (*p == '\\') {
            context->escaped = true;
        } else if (!context->in_single_quote && !context->in_double_quote) {
            switch (*p) {
                case '{': context->brace_depth++; break;
                case '}': if (context->brace_depth > 0) context->brace_depth--; break;
                case '(': context->paren_depth++; break;
                case ')': if (context->paren_depth > 0) context->paren_depth--; break;
                case '[': context->bracket_depth++; break;
                case ']': if (context->bracket_depth > 0) context->bracket_depth--; break;
            }
        }
        p++;
    }
}

// Check if line ends with continuation
static bool is_line_continuation(const char *line) {
    if (!line) {
        return false;
    }
    
    size_t len = strlen(line);
    return (len > 0 && line[len - 1] == '\\');
}

// Convert multiline input for history storage
static char *convert_multiline_for_history(const char *input) {
    if (!input || !*input) {
        return NULL;
    }
    
    size_t input_len = strlen(input);
    char *result = malloc(input_len + 1);
    if (!result) {
        return NULL;
    }
    
    const char *src = input;
    char *dst = result;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;
    bool last_was_space = false;
    
    while (*src) {
        char ch = *src;
        
        // Handle escaping
        if (escaped) {
            if (ch == '\n') {
                // Backslash-newline continuation: replace with single space
                if (!last_was_space) {
                    *dst++ = ' ';
                    last_was_space = true;
                }
            } else {
                *dst++ = '\\';
                *dst++ = ch;
                last_was_space = false;
            }
            escaped = false;
            src++;
            continue;
        }
        
        if (ch == '\\') {
            escaped = true;
            src++;
            continue;
        }
        
        // Handle quotes
        if (ch == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            *dst++ = ch;
            last_was_space = false;
        } else if (ch == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            *dst++ = ch;
            last_was_space = false;
        } else if (ch == '\n') {
            // Convert newlines to spaces
            if (in_single_quote || in_double_quote) {
                // Inside quotes - preserve as literal newline
                *dst++ = '\\';
                *dst++ = 'n';
                last_was_space = false;
            } else {
                // Outside quotes - convert to space
                if (!last_was_space) {
                    *dst++ = ' ';
                    last_was_space = true;
                }
            }
        } else if (isspace(ch)) {
            // Collapse multiple spaces
            if (!last_was_space) {
                *dst++ = ' ';
                last_was_space = true;
            }
        } else {
            *dst++ = ch;
            last_was_space = false;
        }
        
        src++;
    }
    
    // Remove trailing whitespace
    while (dst > result && isspace(*(dst-1))) {
        dst--;
    }
    
    *dst = '\0';
    
    // If the result is empty, return NULL
    if (dst == result) {
        free(result);
        return NULL;
    }
    
    return result;
}

// Set error message
static void set_error(input_context_t *context, const char *message) {
    if (!context) {
        return;
    }
    
    context->has_error = true;
    if (context->error_message) {
        free(context->error_message);
    }
    context->error_message = message ? strdup(message) : NULL;
}

// Ensure buffer has enough capacity
static bool ensure_buffer_capacity(input_context_t *context, size_t needed_size) {
    if (!context || !context->buffer) {
        return false;
    }
    
    if (needed_size <= context->buffer_size) {
        return true;
    }
    
    if (needed_size > INPUT_BUFFER_MAX_SIZE) {
        return false;
    }
    
    // Double the buffer size until it's big enough
    size_t new_size = context->buffer_size;
    while (new_size < needed_size) {
        new_size *= 2;
        if (new_size > INPUT_BUFFER_MAX_SIZE) {
            new_size = INPUT_BUFFER_MAX_SIZE;
            break;
        }
    }
    
    char *new_buffer = realloc(context->buffer, new_size);
    if (!new_buffer) {
        return false;
    }
    
    context->buffer = new_buffer;
    context->buffer_size = new_size;
    
    return true;
}