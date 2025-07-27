#include "../include/input.h"

#include "../include/errors.h"
#include "../include/history.h"
#include "../include/init.h"
#include "../include/linenoise_replacement.h"
#include "../include/lusush.h"
#include "../include/prompt.h"
#include "../include/strings.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// Input buffer management
static char *buf = NULL, *buf2 = NULL;
size_t linecap = MAXLINE + 1;
ssize_t linelen;

// Enhanced input state tracking
typedef struct {
    int control_depth;        // Depth of control structures (if/for/while/case)
    int brace_depth;          // Depth of brace groups { }
    int paren_depth;          // Depth of parentheses groups ( )
    int bracket_depth;        // Depth of bracket groups [ ]
    bool in_single_quote;     // Inside single quotes
    bool in_double_quote;     // Inside double quotes
    bool in_here_doc;         // Inside here document
    char *here_doc_delimiter; // Here document delimiter
    bool escaped;             // Last character was backslash
    bool in_function_def;     // Inside function definition
    bool in_arithmetic;       // Inside arithmetic expansion $(( ))
    bool in_command_subst;    // Inside command substitution $( )
    int command_subst_depth;  // Depth of nested command substitutions
    bool line_continuation;   // Explicit line continuation with backslash
} input_state_t;

// Initialize input state
static void init_input_state(input_state_t *state) {
    memset(state, 0, sizeof(input_state_t));
    state->here_doc_delimiter = NULL;
}

// Free input state resources
static void cleanup_input_state(input_state_t *state) {
    if (state->here_doc_delimiter) {
        free(state->here_doc_delimiter);
        state->here_doc_delimiter = NULL;
    }
}

// Convert complex input to single-line format for history storage
// This makes complex commands more manageable in history recall
// Helper function to identify line types
static bool is_control_keyword(const char *line) {
    return (strcmp(line, "do") == 0 || strcmp(line, "then") == 0 ||
            strcmp(line, "else") == 0 || strncmp(line, "elif", 4) == 0);
}

static bool is_terminator(const char *line) {
    return (strcmp(line, "done") == 0 || strcmp(line, "fi") == 0 ||
            strcmp(line, "esac") == 0 || strcmp(line, "}") == 0);
}

static bool is_control_structure_start(const char *line) {
    return (strncmp(line, "for", 3) == 0 || strncmp(line, "while", 5) == 0 ||
            strncmp(line, "if", 2) == 0 || strncmp(line, "case", 4) == 0 ||
            strncmp(line, "until", 5) == 0);
}

static bool is_regular_command(const char *line) {
    return (!is_control_keyword(line) && !is_terminator(line) && 
            !is_control_structure_start(line));
}

static char *convert_multiline_for_history(const char *input) {
    if (!input || !*input) {
        return NULL;
    }

    size_t input_len = strlen(input);
    // Allocate extra space for potential semicolons
    char *result = malloc(input_len * 2 + 1);
    if (!result) {
        return NULL;
    }

    // Split into lines and build array
    char *input_copy = strdup(input);
    if (!input_copy) {
        free(result);
        return NULL;
    }

    // First pass: collect all non-empty lines
    char *lines[256];  // Support up to 256 lines
    int line_count = 0;
    char *line = strtok(input_copy, "\n");
    
    while (line && line_count < 255) {
        // Trim leading and trailing whitespace
        while (*line && isspace(*line)) line++;
        char *end = line + strlen(line) - 1;
        while (end > line && isspace(*end)) *end-- = '\0';

        // Skip empty lines
        if (*line != '\0') {
            lines[line_count++] = line;
        }
        line = strtok(NULL, "\n");
    }

    // Second pass: build result with proper semicolons
    char *dst = result;
    for (int i = 0; i < line_count; i++) {
        // Add space or semicolon before line if not first
        if (i > 0) {
            bool need_semicolon = false;
            
            // Check if current line is a control keyword needing semicolon before
            if (is_control_keyword(lines[i])) {
                need_semicolon = true;
            }
            // Check if previous line was a control keyword needing semicolon after
            else if (is_control_keyword(lines[i-1])) {
                // Only if current line is not a terminator
                if (!is_terminator(lines[i])) {
                    need_semicolon = true;
                }
            }
            // Need semicolon before terminator if previous line was a command
            else if (is_terminator(lines[i]) && !is_control_keyword(lines[i-1])) {
                need_semicolon = true;
            }
            // Need semicolon between consecutive regular commands
            else if (is_regular_command(lines[i]) && is_regular_command(lines[i-1])) {
                need_semicolon = true;
            }
            // Need semicolon between control structure start and regular command
            else if (is_regular_command(lines[i]) && is_control_structure_start(lines[i-1])) {
                need_semicolon = true;
            }
            // Need semicolon between regular command and control structure start
            else if (is_control_structure_start(lines[i]) && is_regular_command(lines[i-1])) {
                need_semicolon = true;
            }
            
            if (need_semicolon) {
                *dst++ = ';';
                *dst++ = ' ';
            } else {
                *dst++ = ' ';
            }
        }

        // Copy the line
        strcpy(dst, lines[i]);
        dst += strlen(lines[i]);
    }

    *dst = '\0';
    free(input_copy);

    // If the result is empty, return NULL
    if (dst == result) {
        free(result);
        return NULL;
    }

    return result;
}

// Free getline input buffers
void free_input_buffers(void) {
    free_str(buf);
    free_str(buf2);
}

// Check if we're at the start of a here document
static bool check_here_doc_start(const char *line, input_state_t *state) {
    if (!line) {
        return false;
    }

    // Look for << or <<- operators
    const char *pos = line;
    while (*pos) {
        if (*pos == '<' && *(pos + 1) == '<') {
            pos += 2;
            if (*pos == '-') {
                pos++; // Handle <<- variant
            }

            // Skip whitespace
            while (*pos && isspace(*pos)) {
                pos++;
            }

            // Extract delimiter
            const char *delim_start = pos;
            const char *delim_end = pos;

            // Handle quoted delimiters
            if (*pos == '"' || *pos == '\'' || *pos == '\\') {
                char quote_char = *pos;
                pos++;
                delim_start = pos;
                while (*pos && *pos != quote_char) {
                    pos++;
                }
                delim_end = pos;
                if (*pos) {
                    pos++; // Skip closing quote
                }
            } else {
                // Unquoted delimiter - read until whitespace or special char
                while (*pos && !isspace(*pos) && *pos != '|' && *pos != '&' &&
                       *pos != ';' && *pos != '(' && *pos != ')') {
                    pos++;
                }
                delim_end = pos;
            }

            if (delim_end > delim_start) {
                state->in_here_doc = true;
                state->here_doc_delimiter = malloc(delim_end - delim_start + 1);
                if (state->here_doc_delimiter) {
                    strncpy(state->here_doc_delimiter, delim_start,
                            delim_end - delim_start);
                    state->here_doc_delimiter[delim_end - delim_start] = '\0';
                }
                return true;
            }
        }
        pos++;
    }
    return false;
}

// Check if current line ends the here document
static bool check_here_doc_end(const char *line, input_state_t *state) {
    if (!state->in_here_doc || !state->here_doc_delimiter || !line) {
        return false;
    }

    // Trim whitespace from line
    while (*line && isspace(*line)) {
        line++;
    }

    size_t line_len = strlen(line);
    while (line_len > 0 && isspace(line[line_len - 1])) {
        line_len--;
    }

    size_t delim_len = strlen(state->here_doc_delimiter);

    // Check if line matches delimiter exactly
    if (line_len == delim_len &&
        strncmp(line, state->here_doc_delimiter, delim_len) == 0) {
        state->in_here_doc = false;
        free(state->here_doc_delimiter);
        state->here_doc_delimiter = NULL;
        return true;
    }

    return false;
}

// Analyze a line to update input state incrementally
static void analyze_line(const char *line, input_state_t *state) {
    if (!line) {
        return;
    }

    // If we're in a here document, just check for end
    if (state->in_here_doc) {
        check_here_doc_end(line, state);
        return;
    }

    // Check for here document start
    if (check_here_doc_start(line, state)) {
        return; // We're now in a here document
    }

    // Check for line continuation at the end of the line
    const char *line_end = line + strlen(line);
    while (line_end > line && isspace(*(line_end - 1))) {
        line_end--;
    }
    if (line_end > line && *(line_end - 1) == '\\') {
        state->line_continuation = true;
        // Continue analyzing the rest of the line even with continuation
    } else {
        state->line_continuation = false;
    }

    const char *pos = line;
    state->escaped = false;

    while (*pos) {
        char ch = *pos;

        // Handle escaping
        if (state->escaped) {
            state->escaped = false;
            pos++;
            continue;
        }

        if (ch == '\\') {
            state->escaped = true;
            pos++;
            continue;
        }

        // Handle quotes
        if (ch == '\'' && !state->in_double_quote && !state->escaped) {
            state->in_single_quote = !state->in_single_quote;
        } else if (ch == '"' && !state->in_single_quote && !state->escaped) {
            state->in_double_quote = !state->in_double_quote;
        }

        // Skip analysis inside quotes (except for double quote expansions)
        if (state->in_single_quote) {
            pos++;
            continue;
        }

        // Handle special constructs (only if not quoted)
        if (!state->in_single_quote && !state->in_double_quote) {
            // Parentheses and brackets
            if (ch == '(') {
                // Check for arithmetic expansion $(( or command substitution $(
                if (pos > line && *(pos - 1) == '$') {
                    if (pos + 1 < line + strlen(line) && *(pos + 1) == '(') {
                        // Arithmetic expansion $((
                        state->in_arithmetic = true;
                        pos += 2; // Skip both parens
                        continue;
                    } else {
                        // Command substitution $(
                        state->in_command_subst = true;
                        state->command_subst_depth++;
                    }
                }
                state->paren_depth++;
            } else if (ch == ')') {
                if (state->in_arithmetic && pos + 1 < line + strlen(line) &&
                    *(pos + 1) == ')') {
                    // End of arithmetic expansion
                    state->in_arithmetic = false;
                    pos++; // Skip the second paren
                } else if (state->in_command_subst &&
                           state->command_subst_depth > 0) {
                    state->command_subst_depth--;
                    if (state->command_subst_depth == 0) {
                        state->in_command_subst = false;
                    }
                }
                if (state->paren_depth > 0) {
                    state->paren_depth--;
                }
            } else if (ch == '[') {
                state->bracket_depth++;
            } else if (ch == ']') {
                if (state->bracket_depth > 0) {
                    state->bracket_depth--;
                }
            } else if (ch == '{') {
                state->brace_depth++;
            } else if (ch == '}') {
                if (state->brace_depth > 0) {
                    state->brace_depth--;
                }
                // Check if this closes a function definition
                if (state->in_function_def && state->brace_depth == 0) {
                    state->in_function_def = false;
                }
            }
        }

        // Handle keywords (only if not in quotes or expansions)
        if (!state->in_single_quote && !state->in_double_quote &&
            !state->in_arithmetic && !state->in_command_subst) {

            // Check for word boundaries
            bool at_word_start =
                (pos == line || (!isalnum(*(pos - 1)) && *(pos - 1) != '_'));

            if (at_word_start) {
                size_t remaining = strlen(pos);

                if (remaining >= 2 && strncmp(pos, "if", 2) == 0 &&
                    (remaining == 2 || (!isalnum(pos[2]) && pos[2] != '_'))) {
                    state->control_depth++;
                    pos += 1; // Will be incremented at end of loop
                } else if (remaining >= 2 && strncmp(pos, "fi", 2) == 0 &&
                           (remaining == 2 ||
                            (!isalnum(pos[2]) && pos[2] != '_'))) {
                    if (state->control_depth > 0) {
                        state->control_depth--;
                    }
                    pos += 1;
                } else if (remaining >= 3 && strncmp(pos, "for", 3) == 0 &&
                           (remaining == 3 ||
                            (!isalnum(pos[3]) && pos[3] != '_'))) {
                    state->control_depth++;
                    pos += 2;
                } else if (remaining >= 5 && strncmp(pos, "while", 5) == 0 &&
                           (remaining == 5 ||
                            (!isalnum(pos[5]) && pos[5] != '_'))) {
                    state->control_depth++;
                    pos += 4;
                } else if (remaining >= 5 && strncmp(pos, "until", 5) == 0 &&
                           (remaining == 5 ||
                            (!isalnum(pos[5]) && pos[5] != '_'))) {
                    state->control_depth++;
                    pos += 4;
                } else if (remaining >= 4 && strncmp(pos, "done", 4) == 0 &&
                           (remaining == 4 ||
                            (!isalnum(pos[4]) && pos[4] != '_'))) {
                    if (state->control_depth > 0) {
                        state->control_depth--;
                    }
                    pos += 3;
                } else if (remaining >= 4 && strncmp(pos, "case", 4) == 0 &&
                           (remaining == 4 ||
                            (!isalnum(pos[4]) && pos[4] != '_'))) {
                    state->control_depth++;
                    pos += 3;
                } else if (remaining >= 4 && strncmp(pos, "esac", 4) == 0 &&
                           (remaining == 4 ||
                            (!isalnum(pos[4]) && pos[4] != '_'))) {
                    if (state->control_depth > 0) {
                        state->control_depth--;
                    }
                    pos += 3;
                } else if (remaining >= 8 && strncmp(pos, "function", 8) == 0 &&
                           (remaining == 8 ||
                            (!isalnum(pos[8]) && pos[8] != '_'))) {
                    state->in_function_def = true;
                    pos += 7;
                }
            }
        }

        pos++;
    }
}

// Check if input is syntactically complete based on current state
static bool is_input_complete(const input_state_t *state) {
    // Input is complete if:
    // - No unclosed quotes
    // - No unclosed control structures
    // - No unclosed parentheses/brackets/braces
    // - Not in here document
    // - No line continuation
    bool complete = !state->in_single_quote && !state->in_double_quote &&
                    !state->in_here_doc && !state->line_continuation &&
                    state->control_depth == 0 && state->brace_depth == 0 &&
                    state->paren_depth == 0 && state->bracket_depth == 0 &&
                    !state->in_arithmetic && !state->in_command_subst &&
                    !state->in_function_def;

    return complete;
}

// Get appropriate prompt based on input state
static const char *get_continuation_prompt(const input_state_t *state) {
    if (state->in_here_doc) {
        return symtable_get_global_default("PS3", "heredoc> ");
    } else if (state->in_single_quote) {
        return symtable_get_global_default("PS4", "quote> ");
    } else if (state->in_double_quote) {
        return symtable_get_global_default("PS4", "dquote> ");
    } else if (state->control_depth > 0) {
        return symtable_get_global_default("PS2", "> ");
    } else if (state->brace_depth > 0 || state->paren_depth > 0 ||
               state->bracket_depth > 0) {
        return symtable_get_global_default("PS2", "> ");
    } else if (state->in_function_def) {
        return symtable_get_global_default("PS2", "function> ");
    } else if (state->in_arithmetic || state->in_command_subst) {
        return symtable_get_global_default("PS2", "> ");
    } else {
        return symtable_get_global_default("PS2", "> ");
    }
}

// Basic get_input for single line reading (used internally)
static char *get_single_line(FILE *in) {
    static char *line_buf = NULL;
    static size_t line_cap = MAXLINE + 1;

    if (getline(&line_buf, &line_cap, in) == -1) {
        if (feof(in) || ferror(in)) {
            return NULL;
        }
    }

    return line_buf;
}

// Enhanced get_input for non-interactive mode
char *get_input(FILE *in) {
    if (is_interactive_shell()) {
        // Interactive mode - delegate to linenoise version
        return ln_gets();
    }

    // For non-interactive mode, use the complete input system
    return get_input_complete(in);
}

// Enhanced linenoise input with comprehensive command processing
char *ln_gets(void) {
    static char *accumulated_input = NULL;
    static size_t accumulated_size = 0;
    static size_t accumulated_capacity = 0;
    static input_state_t state;
    static bool state_initialized = false;

    // Initialize state on first call
    if (!state_initialized) {
        init_input_state(&state);
        state_initialized = true;
    }

    char *line = NULL;
    bool first_line = (accumulated_size == 0);

    while (true) {
        errno = 0;

        // Get appropriate prompt
        const char *prompt;
        if (first_line) {
            build_prompt();
            prompt = symtable_get_global_default("PS1", "% ");
        } else {
            prompt = get_continuation_prompt(&state);
        }

        // Get line from user
        line = linenoise(prompt);
        if (!line) {
            if (errno == ENOENT) {
                // Ctrl-D (EOF) - return accumulated input if any, or NULL
                if (accumulated_input && *accumulated_input) {
                    // Return accumulated input and reset for next command
                    char *result = strdup(accumulated_input);
                    free(accumulated_input);
                    accumulated_input = NULL;
                    accumulated_size = 0;
                    accumulated_capacity = 0;
                    init_input_state(&state);
                    return result;
                }
                return NULL;
            } else if (errno == EAGAIN) {
                // Ctrl-C (SIGINT) - continue reading, don't exit shell
                continue;
            }
            // Other error or EOF - return accumulated input if any
            if (accumulated_input && *accumulated_input) {
                char *result = accumulated_input;
                accumulated_input = NULL;
                accumulated_size = 0;
                accumulated_capacity = 0;
                cleanup_input_state(&state);
                init_input_state(&state);
                return result;
            }
            return NULL;
        }

        // Analyze this line to update state
        analyze_line(line, &state);

        // Handle accumulation
        size_t line_len = strlen(line);
        size_t needed_size =
            accumulated_size + line_len + 2; // +1 for newline, +1 for null

        if (needed_size > accumulated_capacity) {
            size_t new_capacity =
                accumulated_capacity ? accumulated_capacity * 2 : 1024;
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

        // Free individual line
        free(line);
        line = NULL;

        // Check if input is complete
        if (is_input_complete(&state)) {
            char *result = accumulated_input;
            accumulated_input = NULL;
            accumulated_size = 0;
            accumulated_capacity = 0;

            // Reset state for next input
            cleanup_input_state(&state);
            init_input_state(&state);

            // Add to history if non-empty
            if (*result) {
                // Convert complex input to single-line format for better
                // history handling
                char *history_line = convert_multiline_for_history(result);
                if (history_line) {
                    history_add(history_line);
                    history_save();
                    free(history_line);
                } else {
                    // Fallback to original if conversion fails
                    history_add(result);
                    history_save();
                }
            }

            return result;
        }

        first_line = false;
    }
}

// Enhanced get_input_complete for comprehensive command processing in
// non-interactive mode
char *get_input_complete(FILE *in) {
    static char *accumulated_input = NULL;
    static size_t accumulated_size = 0;
    static size_t accumulated_capacity = 0;
    static input_state_t state;
    static bool state_initialized = false;

    // Initialize state on first call
    if (!state_initialized) {
        init_input_state(&state);
        state_initialized = true;
    }

    while (true) {
        char *line = get_single_line(in);

        if (!line) {
            // EOF or error - return accumulated input if any
            if (accumulated_input && *accumulated_input) {
                char *result = accumulated_input;
                accumulated_input = NULL;
                accumulated_size = 0;
                accumulated_capacity = 0;
                cleanup_input_state(&state);
                init_input_state(&state);
                return result;
            }

            return NULL;
        }

        // Strip trailing whitespace and newline
        str_strip_trailing_whitespace(line);

        // Handle backslash continuation
        bool is_continuation = false;
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\\') {
            line[line_len - 1] = '\0'; // Remove backslash
            line_len--;
            is_continuation = true;
        }

        // Analyze this line to update state
        analyze_line(line, &state);
        if (is_continuation) {
            state.line_continuation = true;
        }

        // Handle accumulation
        size_t needed_size = accumulated_size + line_len +
                             2; // +1 for space/newline, +1 for null
        static bool previous_line_was_continuation = false;

        if (needed_size > accumulated_capacity) {
            size_t new_capacity =
                accumulated_capacity ? accumulated_capacity * 2 : 1024;
            while (new_capacity < needed_size) {
                new_capacity *= 2;
            }

            char *tmp = realloc(accumulated_input, new_capacity);
            if (!tmp) {
                error_syscall("error: realloc in get_input_complete");
                free(accumulated_input);
                accumulated_input = NULL;
                accumulated_size = 0;
                accumulated_capacity = 0;
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
            // Append with space if previous line was continuation, newline
            // otherwise
            if (previous_line_was_continuation) {
                strcat(accumulated_input, " ");
                accumulated_size += 1;
            } else {
                strcat(accumulated_input, "\n");
                accumulated_size += 1;
            }
            strcat(accumulated_input, line);
            accumulated_size += line_len;
        }

        // Update continuation tracking for next iteration
        previous_line_was_continuation = is_continuation;

        // Check if input is complete (not a continuation and all constructs
        // closed)
        bool complete = !state.line_continuation && is_input_complete(&state);

        if (complete) {
            char *result = accumulated_input;
            accumulated_input = NULL;
            accumulated_size = 0;
            accumulated_capacity = 0;

            // Reset state for next input
            cleanup_input_state(&state);
            init_input_state(&state);
            previous_line_was_continuation = false;

            return result;
        }

        // Reset line continuation flag after processing
        if (is_continuation) {
            state.line_continuation = false;
        }
    }
}

// Unified input function providing consistent input behavior
char *get_unified_input(FILE *in) {
    if (is_interactive_shell()) {
        // Interactive mode - use enhanced linenoise (matches shell type
        // detection)
        return ln_gets();
    } else {
        // Non-interactive mode - use enhanced file input
        return get_input_complete(in);
    }
}

// Legacy compatibility function (deprecated - use get_unified_input instead)
char *ln_gets_complete(void) { return ln_gets(); }
