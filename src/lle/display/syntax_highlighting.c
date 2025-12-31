/**
 * @file syntax_highlighting.c
 * @brief LLE Syntax Highlighting Implementation
 *
 * Provides real-time syntax highlighting for shell command input.
 * Core implementation focusing on:
 * - Shell lexer for tokenization
 * - Command/path existence validation
 * - Theme-based color application
 * - ANSI escape sequence generation
 *
 * SPECIFICATION: docs/lle_specification/11_syntax_highlighting_complete.md
 */

#include "lle/syntax_highlighting.h"
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Include lusush headers for command/alias/builtin checks */
#include "alias.h"
#include "builtins.h"

/* ========================================================================== */
/*                         DEFAULT COLOR SCHEME                               */
/* ========================================================================== */

/* Default colors (solarized-inspired dark theme) */
static const lle_syntax_colors_t default_colors = {
    /* Commands */
    .command_valid = 0x00859900,    /* Green */
    .command_invalid = 0x00DC322F,  /* Red */
    .command_builtin = 0x002AA198,  /* Cyan */
    .command_alias = 0x002AA198,    /* Cyan */
    .command_function = 0x00268BD2, /* Blue */

    /* Keywords */
    .keyword = 0x00CB4B16, /* Orange */

    /* Strings */
    .string = 0x00B58900,        /* Yellow */
    .string_escape = 0x00DC322F, /* Red */

    /* Variables */
    .variable = 0x006C71C4,         /* Violet */
    .variable_special = 0x00D33682, /* Magenta */

    /* Paths */
    .path_valid = 0x00859900,   /* Green */
    .path_invalid = 0x00DC322F, /* Red */

    /* Operators */
    .pipe = 0x00268BD2,           /* Blue */
    .redirect = 0x00D33682,       /* Magenta */
    .operator_other = 0x00839496, /* Base0 (default fg) */

    /* Assignment */
    .assignment = 0x006C71C4,     /* Violet (same as variable) */

    /* Other */
    .comment = 0x00586E75,  /* Base01 (dim) */
    .number = 0x002AA198,   /* Cyan */
    .option = 0x00839496,   /* Base0 */
    .glob = 0x00CB4B16,     /* Orange */
    .argument = 0x00839496, /* Base0 */

    /* Here-documents and here-strings */
    .heredoc_op = 0x00D33682,      /* Magenta (same as redirect) */
    .heredoc_delim = 0x00B58900,   /* Yellow (same as string) */
    .heredoc_content = 0x00B58900, /* Yellow (same as string) */
    .herestring = 0x00B58900,      /* Yellow (same as string) */

    /* Process substitution */
    .procsub = 0x00D33682, /* Magenta */

    /* ANSI-C quoting */
    .string_ansic = 0x00B58900, /* Yellow (same as string) */

    /* Arithmetic expansion */
    .arithmetic = 0x002AA198, /* Cyan */

    /* Errors */
    .error = 0x00DC322F,    /* Red bg */
    .error_fg = 0x00FFFFFF, /* White fg */

    /* Attributes */
    .keyword_bold = 1,
    .command_bold = 1,
    .error_underline = 1,
    .path_underline = 1,
    .comment_dim = 1,
};

/* ========================================================================== */
/*                         COMMAND CACHE                                      */
/* ========================================================================== */

#define CMD_CACHE_SIZE 128
#define CMD_CACHE_TTL 30 /* seconds */

typedef struct cmd_cache_entry {
    char *command;
    lle_syntax_token_type_t type;
    time_t timestamp;
} cmd_cache_entry_t;

typedef struct cmd_cache {
    cmd_cache_entry_t entries[CMD_CACHE_SIZE];
} cmd_cache_t;

static unsigned int hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % CMD_CACHE_SIZE;
}

/* ========================================================================== */
/*                         SHELL KEYWORDS                                     */
/* ========================================================================== */

static const char *shell_keywords[] = {
    "if",     "then",  "else",  "elif", "fi",   "for",    "in",       "do",
    "done",   "while", "until", "case", "esac", "select", "function", "time",
    "coproc", "!",     "{",     "}",    "[[",   "]]",     NULL};

/* Keywords that END blocks - these don't expect a command after them */
static const char *block_ending_keywords[] = {"fi", "done", "esac", "}", "]]",
                                              NULL};

static bool is_shell_keyword(const char *word, size_t len) {
    for (int i = 0; shell_keywords[i]; i++) {
        if (strlen(shell_keywords[i]) == len &&
            strncmp(word, shell_keywords[i], len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_block_ending_keyword(const char *word, size_t len) {
    for (int i = 0; block_ending_keywords[i]; i++) {
        if (strlen(block_ending_keywords[i]) == len &&
            strncmp(word, block_ending_keywords[i], len) == 0) {
            return true;
        }
    }
    return false;
}

/* ========================================================================== */
/*                         LEXER HELPERS                                      */
/* ========================================================================== */

static bool is_word_char(char c) {
    unsigned char uc = (unsigned char)c;

    /* UTF-8 continuation bytes (10xxxxxx) and lead bytes (11xxxxxx) are part of
     * words. This ensures multi-byte UTF-8 characters like 'é' (0xC3 0xA9) are
     * not split. */
    if (uc >= 0x80) {
        return true; /* Any non-ASCII byte is part of a word */
    }

    return isalnum(uc) || c == '_' || c == '-' || c == '.' || c == '/' ||
           c == '~' || c == '+' || c == '@' || c == ':' || c == '=';
}

static bool is_option_start(const char *s, size_t remaining) {
    return remaining >= 2 && s[0] == '-' &&
           (isalnum((unsigned char)s[1]) || s[1] == '-');
}

/* is_variable_start - currently inlined in tokenizer, kept for future use */
#if 0
static bool is_variable_start(char c) {
    return c == '$';
}
#endif

static bool is_glob_char(char c) { return c == '*' || c == '?' || c == '['; }

/**
 * Check if a word is a variable assignment (VAR=value pattern)
 * Valid variable names start with letter or underscore, followed by
 * alphanumeric or underscore, then '='
 */
static bool is_assignment(const char *word, size_t len) {
    if (len < 2) return false;  /* Minimum: "x=" */
    
    /* Find the '=' */
    const char *eq = memchr(word, '=', len);
    if (!eq) return false;
    
    /* Variable name must be before the '=' */
    size_t name_len = eq - word;
    if (name_len == 0) return false;
    
    /* First char must be letter or underscore */
    if (!isalpha((unsigned char)word[0]) && word[0] != '_') {
        return false;
    }
    
    /* Rest must be alphanumeric or underscore */
    for (size_t i = 1; i < name_len; i++) {
        if (!isalnum((unsigned char)word[i]) && word[i] != '_') {
            return false;
        }
    }
    
    return true;
}

static size_t skip_whitespace(const char *input, size_t pos, size_t len) {
    while (pos < len && isspace((unsigned char)input[pos])) {
        pos++;
    }
    return pos;
}

/* ========================================================================== */
/*                         COMMAND CHECKING                                   */
/* ========================================================================== */

/* Use is_builtin() from builtins.h (already included above) */
/* Use is_alias() from alias.h (already included above) */

static bool command_exists_in_path(const char *command) {
    /* Check if command contains a path separator */
    if (strchr(command, '/')) {
        return access(command, X_OK) == 0;
    }

    const char *path_env = getenv("PATH");
    if (!path_env)
        return false;

    char *path_copy = strdup(path_env);
    if (!path_copy)
        return false;

    bool found = false;
    char *dir = strtok(path_copy, ":");

    while (dir && !found) {
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            found = true;
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return found;
}

lle_syntax_token_type_t
lle_syntax_check_command(lle_syntax_highlighter_t *highlighter,
                         const char *command) {
    if (!command || !*command) {
        return LLE_TOKEN_UNKNOWN;
    }

    /* Check cache first */
    if (highlighter && highlighter->command_cache) {
        cmd_cache_t *cache = (cmd_cache_t *)highlighter->command_cache;
        unsigned int idx = hash_string(command);
        cmd_cache_entry_t *entry = &cache->entries[idx];

        if (entry->command && strcmp(entry->command, command) == 0) {
            time_t now = time(NULL);
            if (now - entry->timestamp < CMD_CACHE_TTL) {
                return entry->type;
            }
        }
    }

    /* Check command type */
    lle_syntax_token_type_t type;

    if (is_builtin(command)) {
        type = LLE_TOKEN_COMMAND_BUILTIN;
    } else if (lookup_alias(command) != NULL) {
        type = LLE_TOKEN_COMMAND_ALIAS;
    } else if (command_exists_in_path(command)) {
        type = LLE_TOKEN_COMMAND_VALID;
    } else {
        type = LLE_TOKEN_COMMAND_INVALID;
    }

    /* Update cache */
    if (highlighter && highlighter->command_cache) {
        cmd_cache_t *cache = (cmd_cache_t *)highlighter->command_cache;
        unsigned int idx = hash_string(command);
        cmd_cache_entry_t *entry = &cache->entries[idx];

        free(entry->command);
        entry->command = strdup(command);
        entry->type = type;
        entry->timestamp = time(NULL);
    }

    return type;
}

static bool path_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/* ========================================================================== */
/*                         TOKENIZER                                          */
/* ========================================================================== */

static int ensure_token_capacity(lle_syntax_highlighter_t *h, size_t needed) {
    if (needed <= h->token_capacity)
        return 0;

    size_t new_cap = h->token_capacity ? h->token_capacity * 2 : 32;
    while (new_cap < needed)
        new_cap *= 2;

    lle_syntax_token_t *new_tokens =
        realloc(h->tokens, new_cap * sizeof(lle_syntax_token_t));
    if (!new_tokens)
        return -1;

    h->tokens = new_tokens;
    h->token_capacity = new_cap;
    return 0;
}

static int add_token(lle_syntax_highlighter_t *h, lle_syntax_token_type_t type,
                     size_t start, size_t end) {
    if (ensure_token_capacity(h, h->token_count + 1) < 0)
        return -1;

    lle_syntax_token_t *tok = &h->tokens[h->token_count++];
    tok->type = type;
    tok->start = start;
    tok->end = end;
    tok->color = 0;
    tok->attributes = LLE_ATTR_NONE;

    return 0;
}

int lle_syntax_highlight(lle_syntax_highlighter_t *highlighter,
                         const char *input, size_t input_len) {
    if (!highlighter || !input)
        return -1;

    highlighter->token_count = 0;

    size_t pos = 0;
    bool expect_command = true; /* Next word is a command */

    while (pos < input_len) {
        /* Skip whitespace */
        size_t ws_start = pos;
        pos = skip_whitespace(input, pos, input_len);
        if (pos > ws_start) {
            add_token(highlighter, LLE_TOKEN_WHITESPACE, ws_start, pos);
            /* Check if whitespace contained a newline - new line = new command
             */
            for (size_t i = ws_start; i < pos; i++) {
                if (input[i] == '\n') {
                    expect_command = true;
                    break;
                }
            }
        }
        if (pos >= input_len)
            break;

        char c = input[pos];
        size_t token_start = pos;

        /* Comment */
        if (c == '#') {
            while (pos < input_len && input[pos] != '\n')
                pos++;
            add_token(highlighter, LLE_TOKEN_COMMENT, token_start, pos);
            continue;
        }

        /* Single-quoted string */
        if (c == '\'') {
            pos++;
            while (pos < input_len && input[pos] != '\'')
                pos++;
            if (pos < input_len)
                pos++; /* Skip closing quote */
            add_token(highlighter,
                      pos <= input_len && input[pos - 1] == '\''
                          ? LLE_TOKEN_STRING_SINGLE
                          : LLE_TOKEN_UNCLOSED_STRING,
                      token_start, pos);
            expect_command = false;
            continue;
        }

        /* Double-quoted string */
        if (c == '"') {
            pos++;
            while (pos < input_len && input[pos] != '"') {
                if (input[pos] == '\\' && pos + 1 < input_len)
                    pos++;
                pos++;
            }
            if (pos < input_len)
                pos++; /* Skip closing quote */
            add_token(highlighter,
                      pos <= input_len && input[pos - 1] == '"'
                          ? LLE_TOKEN_STRING_DOUBLE
                          : LLE_TOKEN_UNCLOSED_STRING,
                      token_start, pos);
            expect_command = false;
            continue;
        }

        /* Backtick command substitution */
        if (c == '`') {
            pos++;
            while (pos < input_len && input[pos] != '`') {
                if (input[pos] == '\\' && pos + 1 < input_len)
                    pos++;
                pos++;
            }
            if (pos < input_len)
                pos++;
            add_token(highlighter, LLE_TOKEN_STRING_BACKTICK, token_start, pos);
            expect_command = false;
            continue;
        }

        /* Variable and $-prefixed constructs */
        if (c == '$') {
            pos++;
            if (pos < input_len) {
                char next = input[pos];
                lle_syntax_token_type_t vtype = LLE_TOKEN_VARIABLE;

                /* ANSI-C quoting: $'...' */
                if (next == '\'') {
                    pos++; /* Skip opening quote */
                    while (pos < input_len && input[pos] != '\'') {
                        /* Handle escape sequences in ANSI-C strings */
                        if (input[pos] == '\\' && pos + 1 < input_len) {
                            pos++;
                        }
                        pos++;
                    }
                    if (pos < input_len)
                        pos++; /* Skip closing quote */
                    add_token(highlighter,
                              pos <= input_len && input[pos - 1] == '\''
                                  ? LLE_TOKEN_STRING_ANSIC
                                  : LLE_TOKEN_UNCLOSED_STRING,
                              token_start, pos);
                    expect_command = false;
                    continue;
                }
                /* Arithmetic expansion: $((...)) */
                else if (next == '(' && pos + 1 < input_len &&
                         input[pos + 1] == '(') {
                    pos += 2; /* Skip (( */
                    int depth = 1;
                    while (pos < input_len && depth > 0) {
                        if (pos + 1 < input_len && input[pos] == '(' &&
                            input[pos + 1] == '(') {
                            depth++;
                            pos++;
                        } else if (pos + 1 < input_len && input[pos] == ')' &&
                                   input[pos + 1] == ')') {
                            depth--;
                            pos++;
                        }
                        pos++;
                    }
                    add_token(highlighter, LLE_TOKEN_ARITHMETIC, token_start,
                              pos);
                    expect_command = false;
                    continue;
                }
                /* Special variables */
                else if (next == '?' || next == '#' || next == '@' ||
                         next == '*' || next == '$' || next == '!' ||
                         next == '-' || next == '_' ||
                         (next >= '0' && next <= '9')) {
                    pos++;
                    vtype = LLE_TOKEN_VARIABLE_SPECIAL;
                }
                /* ${...} or $(...) command substitution */
                else if (next == '{' || next == '(') {
                    char close = (next == '{') ? '}' : ')';
                    int depth = 1;
                    pos++;
                    while (pos < input_len && depth > 0) {
                        if (input[pos] == next)
                            depth++;
                        else if (input[pos] == close)
                            depth--;
                        pos++;
                    }
                }
                /* Simple $VAR */
                else if (isalpha((unsigned char)next) || next == '_') {
                    pos++;
                    while (pos < input_len &&
                           (isalnum((unsigned char)input[pos]) ||
                            input[pos] == '_')) {
                        pos++;
                    }
                }

                add_token(highlighter, vtype, token_start, pos);
                expect_command = false;
                continue;
            }
        }

        /* Operators */
        if (c == '|') {
            pos++;
            if (pos < input_len && input[pos] == '|') {
                pos++;
                add_token(highlighter, LLE_TOKEN_OR, token_start, pos);
            } else {
                add_token(highlighter, LLE_TOKEN_PIPE, token_start, pos);
            }
            expect_command = true;
            continue;
        }

        if (c == '&') {
            pos++;
            if (pos < input_len && input[pos] == '&') {
                pos++;
                add_token(highlighter, LLE_TOKEN_AND, token_start, pos);
                expect_command = true;
            } else {
                add_token(highlighter, LLE_TOKEN_BACKGROUND, token_start, pos);
            }
            continue;
        }

        if (c == ';') {
            pos++;
            add_token(highlighter, LLE_TOKEN_SEMICOLON, token_start, pos);
            expect_command = true;
            continue;
        }

        if (c == '>' || c == '<') {
            /* Process substitution: >(...) or <(...) */
            if (pos + 1 < input_len && input[pos + 1] == '(') {
                lle_syntax_token_type_t pstype =
                    (c == '<') ? LLE_TOKEN_PROCSUB_IN : LLE_TOKEN_PROCSUB_OUT;
                pos += 2; /* Skip <( or >( */
                int depth = 1;
                while (pos < input_len && depth > 0) {
                    if (input[pos] == '(')
                        depth++;
                    else if (input[pos] == ')')
                        depth--;
                    pos++;
                }
                add_token(highlighter, pstype, token_start, pos);
                expect_command = false;
                continue;
            }

            /* Here-string: <<< */
            if (c == '<' && pos + 2 < input_len && input[pos + 1] == '<' &&
                input[pos + 2] == '<') {
                pos += 3; /* Skip <<< */
                add_token(highlighter, LLE_TOKEN_HERESTRING, token_start, pos);
                expect_command = false;
                continue;
            }

            /* Here-document: << or <<- (with optional quoting of delimiter) */
            if (c == '<' && pos + 1 < input_len && input[pos + 1] == '<' &&
                (pos + 2 >= input_len || input[pos + 2] != '<')) {
                pos += 2; /* Skip << */
                /* Check for <<- (strip leading tabs) */
                if (pos < input_len && input[pos] == '-') {
                    pos++;
                }
                add_token(highlighter, LLE_TOKEN_HEREDOC_OP, token_start, pos);
                /* Next token will be the delimiter (handled by word parsing) */
                expect_command = false;
                continue;
            }

            /* Regular redirect: >, >>, <, >&, <&, etc. */
            pos++;
            while (pos < input_len &&
                   (input[pos] == '>' || input[pos] == '&' ||
                    isdigit((unsigned char)input[pos]))) {
                pos++;
            }
            add_token(highlighter, LLE_TOKEN_REDIRECT, token_start, pos);
            expect_command = false;
            continue;
        }

        if (c == '(') {
            pos++;
            add_token(highlighter, LLE_TOKEN_SUBSHELL_START, token_start, pos);
            expect_command = true;
            continue;
        }

        if (c == ')') {
            pos++;
            add_token(highlighter, LLE_TOKEN_SUBSHELL_END, token_start, pos);
            expect_command = false;
            continue;
        }

        if (c == '{') {
            pos++;
            add_token(highlighter, LLE_TOKEN_BRACE_START, token_start, pos);
            expect_command = true;
            continue;
        }

        if (c == '}') {
            pos++;
            add_token(highlighter, LLE_TOKEN_BRACE_END, token_start, pos);
            expect_command = false;
            continue;
        }

        /* Word (command, argument, path, option, etc.) */
        if (is_word_char(c) || c == '\\') {
            /* Scan the word */
            bool has_glob = false;
            bool has_slash = false;

            while (pos < input_len) {
                char ch = input[pos];
                if (ch == '\\' && pos + 1 < input_len) {
                    pos += 2;
                    continue;
                }
                if (is_glob_char(ch))
                    has_glob = true;
                if (ch == '/')
                    has_slash = true;
                if (!is_word_char(ch) && !is_glob_char(ch))
                    break;
                pos++;
            }

            size_t word_len = pos - token_start;

            /* Determine token type */
            lle_syntax_token_type_t type;

            if (expect_command) {
                /* Extract word for checking */
                char word[256];
                size_t copy_len =
                    word_len < sizeof(word) - 1 ? word_len : sizeof(word) - 1;
                memcpy(word, input + token_start, copy_len);
                word[copy_len] = '\0';

                /* Check for VAR=value assignment prefix first */
                if (is_assignment(input + token_start, word_len)) {
                    type = LLE_TOKEN_ASSIGNMENT;
                    /* Keep expect_command = true because command follows */
                    /* e.g., "VAR=value command arg1 arg2" */
                } else if (is_shell_keyword(input + token_start, word_len)) {
                    type = LLE_TOKEN_KEYWORD;
                    /* Block-ending keywords (done, fi, esac, etc.) don't expect
                       a command after them. Block-starting keywords do. */
                    if (is_block_ending_keyword(input + token_start, word_len)) {
                        expect_command = false;
                    } else {
                        expect_command = true;
                    }
                } else if (highlighter->validate_commands) {
                    type = lle_syntax_check_command(highlighter, word);
                    expect_command = false;
                } else {
                    type = LLE_TOKEN_COMMAND_VALID; /* Assume valid if not
                                                       checking */
                    expect_command = false;
                }
            } else {
                /* Not expecting command - it's an argument */
                if (is_option_start(input + token_start, word_len)) {
                    type = LLE_TOKEN_OPTION;
                } else if (has_glob) {
                    type = LLE_TOKEN_GLOB;
                } else if (has_slash && highlighter->validate_paths) {
                    /* Looks like a path - check if it exists */
                    char path[4096];
                    size_t copy_len = word_len < sizeof(path) - 1
                                          ? word_len
                                          : sizeof(path) - 1;
                    memcpy(path, input + token_start, copy_len);
                    path[copy_len] = '\0';

                    /* Expand ~ if present */
                    if (path[0] == '~') {
                        const char *home = getenv("HOME");
                        if (home) {
                            char expanded[4096];
                            snprintf(expanded, sizeof(expanded), "%s%s", home,
                                     path + 1);
                            type = path_exists(expanded)
                                       ? LLE_TOKEN_PATH_VALID
                                       : LLE_TOKEN_PATH_INVALID;
                        } else {
                            type = LLE_TOKEN_PATH_INVALID;
                        }
                    } else {
                        type = path_exists(path) ? LLE_TOKEN_PATH_VALID
                                                 : LLE_TOKEN_PATH_INVALID;
                    }
                } else if (has_slash) {
                    type =
                        LLE_TOKEN_ARGUMENT; /* Path-like but not validating */
                } else {
                    /* Check if it's a number */
                    bool is_num = true;
                    for (size_t i = token_start; i < pos && is_num; i++) {
                        if (!isdigit((unsigned char)input[i]) &&
                            input[i] != '.' && input[i] != '-' &&
                            input[i] != '+') {
                            is_num = false;
                        }
                    }
                    type = is_num ? LLE_TOKEN_NUMBER : LLE_TOKEN_ARGUMENT;
                }
            }

            add_token(highlighter, type, token_start, pos);
            continue;
        }

        /* Unknown character - skip it */
        pos++;
        add_token(highlighter, LLE_TOKEN_UNKNOWN, token_start, pos);
    }

    /* Apply colors to tokens */
    for (size_t i = 0; i < highlighter->token_count; i++) {
        lle_syntax_token_t *tok = &highlighter->tokens[i];
        const lle_syntax_colors_t *c = &highlighter->colors;

        switch (tok->type) {
        case LLE_TOKEN_COMMAND_VALID:
            tok->color = c->command_valid;
            if (c->command_bold)
                tok->attributes |= LLE_ATTR_BOLD;
            break;
        case LLE_TOKEN_COMMAND_INVALID:
            tok->color = c->command_invalid;
            break;
        case LLE_TOKEN_COMMAND_BUILTIN:
        case LLE_TOKEN_COMMAND_ALIAS:
            tok->color = c->command_builtin;
            if (c->command_bold)
                tok->attributes |= LLE_ATTR_BOLD;
            break;
        case LLE_TOKEN_COMMAND_FUNCTION:
            tok->color = c->command_function;
            break;
        case LLE_TOKEN_KEYWORD:
            tok->color = c->keyword;
            if (c->keyword_bold)
                tok->attributes |= LLE_ATTR_BOLD;
            break;
        case LLE_TOKEN_ASSIGNMENT:
            tok->color = c->assignment;
            break;
        case LLE_TOKEN_STRING_SINGLE:
        case LLE_TOKEN_STRING_DOUBLE:
        case LLE_TOKEN_STRING_BACKTICK:
            tok->color = c->string;
            break;
        case LLE_TOKEN_VARIABLE:
            tok->color = c->variable;
            break;
        case LLE_TOKEN_VARIABLE_SPECIAL:
            tok->color = c->variable_special;
            break;
        case LLE_TOKEN_PATH_VALID:
            tok->color = c->path_valid;
            if (c->path_underline)
                tok->attributes |= LLE_ATTR_UNDERLINE;
            break;
        case LLE_TOKEN_PATH_INVALID:
            tok->color = c->path_invalid;
            if (c->path_underline)
                tok->attributes |= LLE_ATTR_UNDERLINE;
            break;
        case LLE_TOKEN_PIPE:
            tok->color = c->pipe;
            break;
        case LLE_TOKEN_REDIRECT:
            tok->color = c->redirect;
            break;
        case LLE_TOKEN_AND:
        case LLE_TOKEN_OR:
        case LLE_TOKEN_BACKGROUND:
        case LLE_TOKEN_SEMICOLON:
        case LLE_TOKEN_SUBSHELL_START:
        case LLE_TOKEN_SUBSHELL_END:
        case LLE_TOKEN_BRACE_START:
        case LLE_TOKEN_BRACE_END:
            tok->color = c->operator_other;
            break;
        case LLE_TOKEN_COMMENT:
            tok->color = c->comment;
            if (c->comment_dim)
                tok->attributes |= LLE_ATTR_DIM;
            break;
        case LLE_TOKEN_NUMBER:
            tok->color = c->number;
            break;
        case LLE_TOKEN_OPTION:
            tok->color = c->option;
            break;
        case LLE_TOKEN_GLOB:
            tok->color = c->glob;
            break;
        case LLE_TOKEN_ARGUMENT:
            tok->color = c->argument;
            break;
        /* Here-documents and here-strings */
        case LLE_TOKEN_HEREDOC_OP:
            tok->color = c->heredoc_op;
            break;
        case LLE_TOKEN_HEREDOC_DELIM:
            tok->color = c->heredoc_delim;
            break;
        case LLE_TOKEN_HEREDOC_CONTENT:
            tok->color = c->heredoc_content;
            break;
        case LLE_TOKEN_HERESTRING:
            tok->color = c->herestring;
            break;
        /* Process substitution */
        case LLE_TOKEN_PROCSUB_IN:
        case LLE_TOKEN_PROCSUB_OUT:
            tok->color = c->procsub;
            break;
        /* ANSI-C quoting */
        case LLE_TOKEN_STRING_ANSIC:
            tok->color = c->string_ansic;
            break;
        /* Arithmetic expansion */
        case LLE_TOKEN_ARITHMETIC:
            tok->color = c->arithmetic;
            break;
        case LLE_TOKEN_ERROR:
        case LLE_TOKEN_UNCLOSED_STRING:
        case LLE_TOKEN_UNCLOSED_SUBSHELL:
            tok->color = c->error;
            if (c->error_underline)
                tok->attributes |= LLE_ATTR_UNDERLINE;
            break;
        default:
            tok->color = 0;
            break;
        }
    }

    return (int)highlighter->token_count;
}

/* ========================================================================== */
/*                         ANSI RENDERING                                     */
/* ========================================================================== */

int lle_syntax_color_to_ansi(uint32_t color, uint8_t attributes,
                             int color_depth, char *output,
                             size_t output_size) {
    if (!output || output_size < 2)
        return -1;

    char *p = output;
    char *end = output + output_size - 1;

    /* Start escape sequence */
    p += snprintf(p, end - p, "\x1b[");

    /* Attributes */
    bool need_sep = false;
    if (attributes & LLE_ATTR_BOLD) {
        p += snprintf(p, end - p, "1");
        need_sep = true;
    }
    if (attributes & LLE_ATTR_DIM) {
        p += snprintf(p, end - p, "%s2", need_sep ? ";" : "");
        need_sep = true;
    }
    if (attributes & LLE_ATTR_ITALIC) {
        p += snprintf(p, end - p, "%s3", need_sep ? ";" : "");
        need_sep = true;
    }
    if (attributes & LLE_ATTR_UNDERLINE) {
        p += snprintf(p, end - p, "%s4", need_sep ? ";" : "");
        need_sep = true;
    }

    /* Color */
    if (color != 0 && color_depth > 0) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;

        if (color_depth >= 3) {
            /* Truecolor */
            p += snprintf(p, end - p, "%s38;2;%d;%d;%d", need_sep ? ";" : "", r,
                          g, b);
        } else if (color_depth == 2) {
            /* 256 color - convert RGB to 256 color index */
            int idx;
            if (r == g && g == b) {
                /* Grayscale */
                if (r < 8)
                    idx = 16;
                else if (r > 248)
                    idx = 231;
                else
                    idx = 232 + (r - 8) / 10;
            } else {
                /* Color cube */
                idx = 16 + (r / 51) * 36 + (g / 51) * 6 + (b / 51);
            }
            p += snprintf(p, end - p, "%s38;5;%d", need_sep ? ";" : "", idx);
        } else {
            /* 8 color - pick closest basic color */
            int basic = 0;
            if (r > 127)
                basic |= 1;
            if (g > 127)
                basic |= 2;
            if (b > 127)
                basic |= 4;
            /* Use bright if any component is very bright */
            if (r > 200 || g > 200 || b > 200) {
                p += snprintf(p, end - p, "%s9%d", need_sep ? ";" : "", basic);
            } else {
                p += snprintf(p, end - p, "%s3%d", need_sep ? ";" : "", basic);
            }
        }
    }

    p += snprintf(p, end - p, "m");

    return (int)(p - output);
}

int lle_syntax_render_ansi(lle_syntax_highlighter_t *highlighter,
                           const char *input, char *output,
                           size_t output_size) {
    if (!highlighter || !input || !output || output_size == 0)
        return -1;

    char *p = output;
    char *end = output + output_size - 1;

    for (size_t i = 0; i < highlighter->token_count && p < end; i++) {
        lle_syntax_token_t *tok = &highlighter->tokens[i];

        /* Skip whitespace and unknown tokens - just copy them */
        if (tok->type == LLE_TOKEN_WHITESPACE ||
            tok->type == LLE_TOKEN_UNKNOWN) {
            size_t len = tok->end - tok->start;
            if (p + len >= end)
                len = end - p;
            memcpy(p, input + tok->start, len);
            p += len;
            continue;
        }

        /* Apply color */
        if (tok->color != 0 || tok->attributes != 0) {
            char color_seq[64];
            int seq_len = lle_syntax_color_to_ansi(
                tok->color, tok->attributes, highlighter->color_depth,
                color_seq, sizeof(color_seq));
            if (seq_len > 0 && p + seq_len < end) {
                memcpy(p, color_seq, seq_len);
                p += seq_len;
            }
        }

        /* Copy token text */
        size_t len = tok->end - tok->start;
        if (p + len >= end)
            len = end - p;
        memcpy(p, input + tok->start, len);
        p += len;

        /* Reset after token */
        if (tok->color != 0 || tok->attributes != 0) {
            const char *reset = "\x1b[0m";
            size_t reset_len = 4;
            if (p + reset_len < end) {
                memcpy(p, reset, reset_len);
                p += reset_len;
            }
        }
    }

    *p = '\0';
    return (int)(p - output);
}

/* ========================================================================== */
/*                         PUBLIC API                                         */
/* ========================================================================== */

int lle_syntax_highlighter_create(lle_syntax_highlighter_t **highlighter) {
    if (!highlighter)
        return -1;

    lle_syntax_highlighter_t *h = calloc(1, sizeof(lle_syntax_highlighter_t));
    if (!h)
        return -1;

    /* Set defaults */
    h->colors = default_colors;
    h->enabled = true;
    h->validate_commands = true;
    h->validate_paths = true;
    h->highlight_errors = true;
    h->color_depth = 3; /* Assume truecolor */

    /* Create command cache */
    h->command_cache = calloc(1, sizeof(cmd_cache_t));

    *highlighter = h;
    return 0;
}

void lle_syntax_highlighter_destroy(lle_syntax_highlighter_t *highlighter) {
    if (!highlighter)
        return;

    /* Free command cache */
    if (highlighter->command_cache) {
        cmd_cache_t *cache = (cmd_cache_t *)highlighter->command_cache;
        for (int i = 0; i < CMD_CACHE_SIZE; i++) {
            free(cache->entries[i].command);
        }
        free(cache);
    }

    free(highlighter->tokens);
    free(highlighter);
}

void lle_syntax_highlighter_set_colors(lle_syntax_highlighter_t *highlighter,
                                       const lle_syntax_colors_t *colors) {
    if (!highlighter || !colors)
        return;
    highlighter->colors = *colors;
}

void lle_syntax_colors_get_default(lle_syntax_colors_t *colors) {
    if (!colors)
        return;
    *colors = default_colors;
}

const lle_syntax_token_t *
lle_syntax_get_tokens(lle_syntax_highlighter_t *highlighter, size_t *count) {
    if (!highlighter) {
        if (count)
            *count = 0;
        return NULL;
    }
    if (count)
        *count = highlighter->token_count;
    return highlighter->tokens;
}

void lle_syntax_clear_cache(lle_syntax_highlighter_t *highlighter) {
    if (!highlighter || !highlighter->command_cache)
        return;

    cmd_cache_t *cache = (cmd_cache_t *)highlighter->command_cache;
    for (int i = 0; i < CMD_CACHE_SIZE; i++) {
        free(cache->entries[i].command);
        cache->entries[i].command = NULL;
        cache->entries[i].type = LLE_TOKEN_UNKNOWN;
        cache->entries[i].timestamp = 0;
    }
}
