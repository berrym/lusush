#include "../include/input.h"

#include "../include/errors.h"
#include "../include/history.h"
#include "../include/init.h"
#include "../include/linenoise/linenoise.h"
#include "../include/lusush.h"
#include "../include/prompt.h"
#include "../include/scanner.h"
#include "../include/strings.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// getline input buffers
static char *buf = NULL, *buf2 = NULL;
size_t linecap = MAXLINE + 1;
ssize_t linelen;

// Free getline input buffers
void free_input_buffers(void) {
    free_str(buf);
    free_str(buf2);
}

// Get a line of input using linenoise
char *ln_gets(void) {
    char *line = NULL, *next = NULL, *tmp = NULL;

    while (true) {
        errno = 0;

        build_prompt();

        if (line == NULL) {
            line = linenoise(get_shell_varp("PS1", "% "));
        }

        if (line == NULL) {
            return NULL;
        }

        if (errno == ENOENT) {
            return NULL;
        }

        // Handle line continuations
        str_strip_trailing_whitespace(line);
        if (line[strlen(line) - 1] == '\\') {
            line[strlen(line) - 1] = '\0';
            if (next == NULL) {
                next = alloc_str(strlen(line) + 1, true);
                strcpy(next, line);
            }
            line = linenoise(get_shell_varp("PS2", "> "));
            if (line == NULL) {
                return NULL;
            }

            if (errno == ENOENT) {
                return NULL;
            }

            tmp =
                realloc(next, (strlen(next) + strlen(line) + 1) * sizeof(char));
            if (tmp == NULL) {
                error_syscall("error: `ln_gets`");
            }
            next = tmp;
            strcat(next, line);
            line = next;
        } else {
            break;
        }
    }

    // If the line has any text in it, save it in history
    if (*line) {
        history_add(line);
        history_save();
    }

    // Return the line
    return line;
}

// Get a line of input
char *get_input(FILE *in) {
    // Read a line from either a file or standard input
    if (shell_type() != NORMAL_SHELL) {
        buf = ln_gets();
    } else {
        buf = alloc_str(MAXLINE + 1, true);
        buf2 = alloc_str(MAXLINE + 1, true);

        // Read a line of input
        while ((linelen = getline(&buf2, &linecap, in))) {
            if (feof(in)) {
                return NULL;
            }

            if (ferror(in)) {
                error_return("error: `get_input`");
                return NULL;
            }

            if (!*buf) {
                strcpy(buf, buf2);
            } else {
                char *tmp =
                    realloc(buf, (strlen(buf) + linelen + 1) * sizeof(char));
                if (tmp == NULL) {
                    error_syscall("error: `get_line`");
                }
                buf = tmp;
                strcat(buf, buf2);
            }

            str_strip_trailing_whitespace(buf);
            if (buf[strlen(buf) - 1] == '\\') {
                buf[strlen(buf) - 1] = '\0';
            } else {
                break;
            }
        }
    }

    return buf;
}

// Function to check if a command line is syntactically complete
// Returns 1 if complete, 0 if needs more input, -1 if error
static int is_command_complete(const char *input) {
    if (!input || !*input) {
        return 1; // Empty input is complete
    }
    
    // Create a temporary source to scan the tokens
    source_t temp_src;
    temp_src.buf = (char *)input;
    temp_src.bufsize = strlen(input);
    temp_src.pos = INIT_SRC_POS;
    temp_src.pos_old = INIT_SRC_POS;
    temp_src.curline = 1;
    temp_src.curchar = 1;
    temp_src.curlinestart = 0;
    temp_src.wstart = 0;
    
    int control_depth = 0;
    int brace_depth = 0;
    int paren_depth = 0;
    int if_count = 0, for_count = 0, while_count = 0, until_count = 0, case_count = 0;
    token_t *tok;
    
    // Scan through all tokens
    while ((tok = tokenize(&temp_src)) && tok != &eof_token) {
        if (!tok || !tok->text) {
            if (tok) free_token(tok);
            continue;
        }
        
        switch (tok->type) {
            case TOKEN_KEYWORD_IF:
                if_count++;
                control_depth++;
                break;
            case TOKEN_KEYWORD_FI:
                if (if_count > 0) {
                    if_count--;
                    control_depth--;
                }
                break;
            case TOKEN_KEYWORD_FOR:
            case TOKEN_KEYWORD_WHILE: 
            case TOKEN_KEYWORD_UNTIL:
                if (tok->type == TOKEN_KEYWORD_FOR) for_count++;
                else if (tok->type == TOKEN_KEYWORD_WHILE) while_count++;
                else if (tok->type == TOKEN_KEYWORD_UNTIL) until_count++;
                control_depth++;
                break;
            case TOKEN_KEYWORD_DONE:
                if (for_count > 0) {
                    for_count--;
                    control_depth--;
                } else if (while_count > 0) {
                    while_count--;
                    control_depth--;
                } else if (until_count > 0) {
                    until_count--;
                    control_depth--;
                }
                break;
            case TOKEN_KEYWORD_CASE:
                case_count++;
                control_depth++;
                break;
            case TOKEN_KEYWORD_ESAC:
                if (case_count > 0) {
                    case_count--;
                    control_depth--;
                }
                break;
            case TOKEN_KEYWORD_LBRACE: // {
                brace_depth++;
                break;
            case TOKEN_KEYWORD_RBRACE: // }
                brace_depth--;
                break;
            case TOKEN_LEFT_PAREN:
                paren_depth++;
                break;
            case TOKEN_RIGHT_PAREN:
                paren_depth--;
                break;
            default:
                break;
        }
        
        free_token(tok);
    }
    
    // Command is complete if all structures are closed
    return (control_depth == 0 && brace_depth == 0 && paren_depth == 0) ? 1 : 0;
}

// Enhanced get_input that buffers complete syntactic units
char *get_input_complete(FILE *in) {
    static char *accumulated_input = NULL;
    static size_t accumulated_size = 0;
    char *line = get_input(in);

    if (!line) {
        // EOF or error - return any accumulated input
        if (accumulated_input && *accumulated_input) {
            char *result = accumulated_input;
            accumulated_input = NULL;
            accumulated_size = 0;
            return result;
        }
        return NULL;
    }

    // If we have accumulated input, append the new line
    if (accumulated_input) {
        size_t line_len = strlen(line);
        size_t needed = accumulated_size + line_len + 2; // +1 for newline, +1 for null
        char *tmp = realloc(accumulated_input, needed);
        if (!tmp) {
            error_syscall("error: realloc in get_input_complete");
            free(accumulated_input);
            accumulated_input = NULL;
            accumulated_size = 0;
            return line; // Return what we have
        }
        accumulated_input = tmp;
        strcat(accumulated_input, "\n");
        strcat(accumulated_input, line);
        accumulated_size = needed - 1;
    } else {
        // First line - start accumulating
        accumulated_size = strlen(line) + 1;
        accumulated_input = malloc(accumulated_size);
        if (!accumulated_input) {
            error_syscall("error: malloc in get_input_complete");
            return line; // Return what we have
        }
        strcpy(accumulated_input, line);
    }

    // Check if the accumulated input is complete
    if (is_command_complete(accumulated_input)) {
        char *result = accumulated_input;
        accumulated_input = NULL;
        accumulated_size = 0;
        return result;
    }

    // Need more input - recursively call to get next line
    return get_input_complete(in);
}

// Enhanced linenoise input that uses is_command_complete() for proper multiline support
char *ln_gets_complete(void) {
    static char *accumulated_input = NULL;
    static size_t accumulated_size = 0;
    char *line = NULL;
    char *prompt = NULL;
    bool first_line = true;

    while (true) {
        errno = 0;

        // Use PS1 for first line, PS2 for continuation
        if (first_line) {
            build_prompt();
            prompt = get_shell_varp("PS1", "% ");
        } else {
            prompt = get_shell_varp("PS2", "> ");
        }

        line = linenoise(prompt);
        if (!line) {
            if (errno == ENOENT) {
                return NULL;
            }
            // EOF or error - return accumulated input if any
            if (accumulated_input && *accumulated_input) {
                char *result = accumulated_input;
                accumulated_input = NULL;
                accumulated_size = 0;
                return result;
            }
            return NULL;
        }

        // Strip trailing whitespace
        str_strip_trailing_whitespace(line);

        // Handle accumulation
        if (accumulated_input) {
            size_t line_len = strlen(line);
            size_t needed = accumulated_size + line_len + 2; // +1 for newline, +1 for null
            char *tmp = realloc(accumulated_input, needed);
            if (!tmp) {
                error_syscall("error: realloc in ln_gets_complete");
                free(accumulated_input);
                accumulated_input = NULL;
                accumulated_size = 0;
                free(line);
                return NULL;
            }
            accumulated_input = tmp;
            strcat(accumulated_input, "\n");
            strcat(accumulated_input, line);
            accumulated_size = needed - 1;
        } else {
            // First line - start accumulating
            accumulated_size = strlen(line) + 1;
            accumulated_input = malloc(accumulated_size);
            if (!accumulated_input) {
                error_syscall("error: malloc in ln_gets_complete");
                return line; // Return what we have
            }
            strcpy(accumulated_input, line);
        }

        // Free the individual line since it's now in accumulated_input
        free(line);
        line = NULL;

        // Check if the accumulated input is syntactically complete
        if (is_command_complete(accumulated_input)) {
            char *result = accumulated_input;
            accumulated_input = NULL;
            accumulated_size = 0;
            
            // Add to history if non-empty
            if (*result) {
                history_add(result);
                history_save();
            }
            
            return result;
        }

        // Need more input - continue loop with PS2 prompt
        first_line = false;
    }
}

// Unified input function that provides consistent behavior for interactive and non-interactive modes
char *get_unified_input(FILE *in) {
    if (shell_type() != NORMAL_SHELL) {
        // Interactive mode - use enhanced linenoise with completion detection
        return ln_gets_complete();
    } else {
        // Non-interactive mode - use existing enhanced input
        return get_input_complete(in);
    }
}
