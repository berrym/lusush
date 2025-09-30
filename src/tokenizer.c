#include "../include/tokenizer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Keyword lookup table
static const struct {
    const char *text;
    token_type_t type;
} keywords[] = {
    {      "if",       TOK_IF},
    {    "then",     TOK_THEN},
    {    "else",     TOK_ELSE},
    {    "elif",     TOK_ELIF},
    {      "fi",       TOK_FI},
    {   "while",    TOK_WHILE},
    {      "do",       TOK_DO},
    {    "done",     TOK_DONE},
    {     "for",      TOK_FOR},
    {      "in",       TOK_IN},
    {    "case",     TOK_CASE},
    {    "esac",     TOK_ESAC},
    {   "until",    TOK_UNTIL},
    {"function", TOK_FUNCTION},
    {      NULL,     TOK_WORD}  // Sentinel
};

// Helper functions
static token_t *token_new(token_type_t type, const char *text, size_t length,
                          size_t line, size_t column, size_t position);
static void token_free(token_t *token);
static token_t *tokenize_next(tokenizer_t *tokenizer);
static token_type_t classify_word(const char *text, size_t length,
                                  bool enable_keywords);
static bool is_operator_char(char c);
static bool is_word_char(char c);
static void skip_whitespace(tokenizer_t *tokenizer);

// Create new tokenizer
tokenizer_t *tokenizer_new(const char *input) {
    if (!input) {
        return NULL;
    }

    tokenizer_t *tokenizer = malloc(sizeof(tokenizer_t));
    if (!tokenizer) {
        return NULL;
    }

    tokenizer->input = input;
    tokenizer->input_length = strlen(input);
    tokenizer->position = 0;
    tokenizer->line = 1;
    tokenizer->column = 1;
    tokenizer->current = NULL;
    tokenizer->lookahead = NULL;
    tokenizer->enable_keywords = true;

    // Initialize by getting the first two tokens
    tokenizer->current = tokenize_next(tokenizer);
    tokenizer->lookahead = tokenize_next(tokenizer);

    return tokenizer;
}

// Free tokenizer
void tokenizer_free(tokenizer_t *tokenizer) {
    if (!tokenizer) {
        return;
    }

    if (tokenizer->current) {
        token_free(tokenizer->current);
    }
    if (tokenizer->lookahead) {
        token_free(tokenizer->lookahead);
    }

    free(tokenizer);
}

// Get current token
token_t *tokenizer_current(tokenizer_t *tokenizer) {
    return tokenizer ? tokenizer->current : NULL;
}

// Peek at next token
token_t *tokenizer_peek(tokenizer_t *tokenizer) {
    return tokenizer ? tokenizer->lookahead : NULL;
}

// Advance to next token
void tokenizer_advance(tokenizer_t *tokenizer) {
    if (!tokenizer) {
        return;
    }

    // Free the current token
    if (tokenizer->current) {
        token_free(tokenizer->current);
    }

    // Move lookahead to current
    tokenizer->current = tokenizer->lookahead;

    // Get new lookahead
    tokenizer->lookahead = tokenize_next(tokenizer);
}

// Check if current token matches expected type
bool tokenizer_match(tokenizer_t *tokenizer, token_type_t type) {
    if (!tokenizer || !tokenizer->current) {
        return false;
    }
    return tokenizer->current->type == type;
}

// Consume expected token type
bool tokenizer_consume(tokenizer_t *tokenizer, token_type_t type) {
    if (tokenizer_match(tokenizer, type)) {
        tokenizer_advance(tokenizer);
        return true;
    }
    return false;
}

// Get token type name for debugging
const char *token_type_name(token_type_t type) {
    switch (type) {
    case TOK_EOF:
        return "EOF";
    case TOK_WORD:
        return "WORD";
    case TOK_STRING:
        return "STRING";
    case TOK_EXPANDABLE_STRING:
        return "EXPANDABLE_STRING";
    case TOK_NUMBER:
        return "NUMBER";
    case TOK_VARIABLE:
        return "VARIABLE";
    case TOK_SEMICOLON:
        return "SEMICOLON";
    case TOK_PIPE:
        return "PIPE";
    case TOK_AND:
        return "AND";
    case TOK_LOGICAL_AND:
        return "LOGICAL_AND";
    case TOK_LOGICAL_OR:
        return "LOGICAL_OR";
    case TOK_REDIRECT_IN:
        return "REDIRECT_IN";
    case TOK_REDIRECT_OUT:
        return "REDIRECT_OUT";
    case TOK_APPEND:
        return "APPEND";
    case TOK_HEREDOC:
        return "HEREDOC";
    case TOK_HEREDOC_STRIP:
        return "HEREDOC_STRIP";
    case TOK_HERESTRING:
        return "HERESTRING";
    case TOK_REDIRECT_ERR:
        return "REDIRECT_ERR";
    case TOK_REDIRECT_BOTH:
        return "REDIRECT_BOTH";
    case TOK_APPEND_ERR:
        return "APPEND_ERR";
    case TOK_REDIRECT_FD:
        return "REDIRECT_FD";
    case TOK_REDIRECT_CLOBBER:
        return "REDIRECT_CLOBBER";
    case TOK_ASSIGN:
        return "ASSIGN";
    case TOK_NOT_EQUAL:
        return "NOT_EQUAL";
    case TOK_PLUS:
        return "PLUS";
    case TOK_MINUS:
        return "MINUS";
    case TOK_MULTIPLY:
        return "MULTIPLY";
    case TOK_DIVIDE:
        return "DIVIDE";
    case TOK_MODULO:
        return "MODULO";
    case TOK_GLOB:
        return "GLOB";
    case TOK_QUESTION:
        return "QUESTION";
    case TOK_COMMAND_SUB:
        return "COMMAND_SUB";
    case TOK_ARITH_EXP:
        return "ARITH_EXP";
    case TOK_BACKQUOTE:
        return "BACKQUOTE";
    case TOK_LPAREN:
        return "LPAREN";
    case TOK_RPAREN:
        return "RPAREN";
    case TOK_LBRACE:
        return "LBRACE";
    case TOK_RBRACE:
        return "RBRACE";
    case TOK_LBRACKET:
        return "LBRACKET";
    case TOK_RBRACKET:
        return "RBRACKET";
    case TOK_IF:
        return "IF";
    case TOK_THEN:
        return "THEN";
    case TOK_ELSE:
        return "ELSE";
    case TOK_ELIF:
        return "ELIF";
    case TOK_FI:
        return "FI";
    case TOK_WHILE:
        return "WHILE";
    case TOK_DO:
        return "DO";
    case TOK_DONE:
        return "DONE";
    case TOK_FOR:
        return "FOR";
    case TOK_IN:
        return "IN";
    case TOK_CASE:
        return "CASE";
    case TOK_ESAC:
        return "ESAC";
    case TOK_UNTIL:
        return "UNTIL";
    case TOK_FUNCTION:
        return "FUNCTION";
    case TOK_NEWLINE:
        return "NEWLINE";
    case TOK_WHITESPACE:
        return "WHITESPACE";
    case TOK_COMMENT:
        return "COMMENT";
    case TOK_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

// Check if token is a keyword
bool token_is_keyword(token_type_t type) {
    return type >= TOK_IF && type <= TOK_FUNCTION;
}

// Check if token is an operator
bool token_is_operator(token_type_t type) {
    return (type >= TOK_SEMICOLON && type <= TOK_REDIRECT_CLOBBER) ||
           (type >= TOK_ASSIGN && type <= TOK_BACKQUOTE);
}

// Check if token is word-like (can be part of command/argument)
bool token_is_word_like(token_type_t type) {
    return type == TOK_WORD || type == TOK_STRING ||
           type == TOK_EXPANDABLE_STRING || type == TOK_NUMBER ||
           type == TOK_VARIABLE;
}

// Enable/disable keyword recognition
void tokenizer_enable_keywords(tokenizer_t *tokenizer, bool enable) {
    if (tokenizer) {
        tokenizer->enable_keywords = enable;
    }
}

// Refresh tokenizer from current position (for error recovery)
void tokenizer_refresh_from_position(tokenizer_t *tokenizer) {
    if (!tokenizer) {
        return;
    }

    // Free existing tokens
    if (tokenizer->current) {
        token_free(tokenizer->current);
        tokenizer->current = NULL;
    }
    if (tokenizer->lookahead) {
        token_free(tokenizer->lookahead);
        tokenizer->lookahead = NULL;
    }

    // Re-tokenize from current position
    tokenizer->current = tokenize_next(tokenizer);
    tokenizer->lookahead = tokenize_next(tokenizer);
}

// Helper functions implementation

// Create new token
static token_t *token_new(token_type_t type, const char *text, size_t length,
                          size_t line, size_t column, size_t position) {
    token_t *token = malloc(sizeof(token_t));
    if (!token) {
        return NULL;
    }

    token->type = type;
    token->length = length;
    token->line = line;
    token->column = column;
    token->position = position;
    token->next = NULL;

    if (text && length > 0) {
        token->text = malloc(length + 1);
        if (!token->text) {
            free(token);
            return NULL;
        }
        strncpy(token->text, text, length);
        token->text[length] = '\0';
    } else {
        // Always allocate text, even for empty strings
        token->text = malloc(1);
        if (!token->text) {
            free(token);
            return NULL;
        }
        token->text[0] = '\0';
    }

    return token;
}

// Free token
static void token_free(token_t *token) {
    if (!token) {
        return;
    }

    if (token->text) {
        free(token->text);
    }
    free(token);
}

// Classify word as keyword or regular word
static token_type_t classify_word(const char *text, size_t length,
                                  bool enable_keywords) {
    if (!enable_keywords || !text || length == 0) {
        return TOK_WORD;
    }

    // Check against keyword table
    for (size_t i = 0; keywords[i].text; i++) {
        if (strlen(keywords[i].text) == length &&
            strncmp(keywords[i].text, text, length) == 0) {
            return keywords[i].type;
        }
    }

    return TOK_WORD;
}

// Check if character can be part of an operator
static bool is_operator_char(char c) {
    return strchr(";|&<>=+*%?(){}[]#!", c) != NULL;
}

// Check if character can be part of a word
static bool is_word_char(char c) {
    return isalnum(c) || strchr("_.-/~:@*?[]+%", c) != NULL;
}

// Skip whitespace (except newlines)
static void skip_whitespace(tokenizer_t *tokenizer) {
    while (tokenizer->position < tokenizer->input_length) {
        char c = tokenizer->input[tokenizer->position];
        if (c == ' ' || c == '\t') {
            tokenizer->position++;
            tokenizer->column++;
        } else {
            break;
        }
    }
}

// Main tokenization function
static token_t *tokenize_next(tokenizer_t *tokenizer) {
    if (!tokenizer || tokenizer->position >= tokenizer->input_length) {
        return token_new(TOK_EOF, NULL, 0, tokenizer ? tokenizer->line : 1,
                         tokenizer ? tokenizer->column : 1,
                         tokenizer ? tokenizer->position : 0);
    }

    skip_whitespace(tokenizer);

    if (tokenizer->position >= tokenizer->input_length) {
        return token_new(TOK_EOF, NULL, 0, tokenizer->line, tokenizer->column,
                         tokenizer->position);
    }

    size_t start_pos = tokenizer->position;
    size_t start_line = tokenizer->line;
    size_t start_column = tokenizer->column;
    char c = tokenizer->input[tokenizer->position];

    // Handle newlines
    if (c == '\n') {
        tokenizer->position++;
        tokenizer->line++;
        tokenizer->column = 1;
        return token_new(TOK_NEWLINE, "\n", 1, start_line, start_column,
                         start_pos);
    }

    // Handle comments
    if (c == '#') {
        size_t start = tokenizer->position;
        while (tokenizer->position < tokenizer->input_length &&
               tokenizer->input[tokenizer->position] != '\n') {
            tokenizer->position++;
            tokenizer->column++;
        }
        size_t length = tokenizer->position - start;
        return token_new(TOK_COMMENT, &tokenizer->input[start], length,
                         start_line, start_column, start_pos);
    }

    // Handle quoted strings
    if (c == '\'' || c == '"') {
        char quote_char = c;
        tokenizer->position++; // Skip opening quote
        tokenizer->column++;
        size_t start = tokenizer->position;

        while (tokenizer->position < tokenizer->input_length) {
            char curr = tokenizer->input[tokenizer->position];
            if (curr == quote_char) {
                // Found closing quote
                size_t length = tokenizer->position - start;
                tokenizer->position++; // Skip closing quote
                tokenizer->column++;

                token_type_t type =
                    (quote_char == '"') ? TOK_EXPANDABLE_STRING : TOK_STRING;
                return token_new(type, &tokenizer->input[start], length,
                                 start_line, start_column, start_pos);
            } else if (curr == '$' && quote_char == '"' &&
                       tokenizer->position + 1 < tokenizer->input_length &&
                       tokenizer->input[tokenizer->position + 1] == '(') {
                // Handle command substitution inside double quotes
                tokenizer->position += 2; // Skip $(
                tokenizer->column += 2;
                int paren_depth = 1;

                // Skip over the entire command substitution, handling nested
                // quotes
                while (tokenizer->position < tokenizer->input_length &&
                       paren_depth > 0) {
                    char sub_curr = tokenizer->input[tokenizer->position];
                    if (sub_curr == '(') {
                        paren_depth++;
                    } else if (sub_curr == ')') {
                        paren_depth--;
                    } else if (sub_curr == '"' || sub_curr == '\'') {
                        // Handle nested quotes within command substitution
                        char sub_quote = sub_curr;
                        tokenizer->position++;
                        tokenizer->column++;
                        while (tokenizer->position < tokenizer->input_length) {
                            char nested_curr =
                                tokenizer->input[tokenizer->position];
                            if (nested_curr == sub_quote) {
                                break; // Found matching quote
                            } else if (nested_curr == '\\' &&
                                       tokenizer->position + 1 <
                                           tokenizer->input_length) {
                                // Skip escaped character
                                tokenizer->position++;
                                tokenizer->column++;
                            }
                            if (nested_curr == '\n') {
                                tokenizer->line++;
                                tokenizer->column = 1;
                            } else {
                                tokenizer->column++;
                            }
                            tokenizer->position++;
                        }
                    } else if (sub_curr == '\\' &&
                               tokenizer->position + 1 <
                                   tokenizer->input_length) {
                        // Skip escaped character
                        tokenizer->position++;
                        tokenizer->column++;
                    }

                    if (sub_curr == '\n') {
                        tokenizer->line++;
                        tokenizer->column = 1;
                    } else {
                        tokenizer->column++;
                    }
                    tokenizer->position++;
                }
            } else if (curr == '`' && quote_char == '"') {
                // Handle backtick command substitution inside double quotes
                tokenizer->position++; // Skip opening backtick
                tokenizer->column++;

                // Skip over the entire backtick command substitution
                while (tokenizer->position < tokenizer->input_length) {
                    char sub_curr = tokenizer->input[tokenizer->position];
                    if (sub_curr == '`') {
                        // Found closing backtick, advance past it
                        tokenizer->position++;
                        tokenizer->column++;
                        break;
                    } else if (sub_curr == '\\' &&
                               tokenizer->position + 1 <
                                   tokenizer->input_length) {
                        // Skip escaped character
                        tokenizer->position++;
                        tokenizer->column++;
                    }

                    if (sub_curr == '\n') {
                        tokenizer->line++;
                        tokenizer->column = 1;
                    } else {
                        tokenizer->column++;
                    }
                    tokenizer->position++;
                }
            } else if (curr == '\\' && quote_char == '"' &&
                       tokenizer->position + 1 < tokenizer->input_length) {
                // Handle escape sequences in double quotes
                tokenizer->position++; // Skip backslash
                tokenizer->column++;
                if (tokenizer->position < tokenizer->input_length) {
                    char escaped = tokenizer->input[tokenizer->position];
                    if (escaped == '\n') {
                        tokenizer->line++;
                        tokenizer->column = 1;
                    } else {
                        tokenizer->column++;
                    }
                    tokenizer->position++;
                }
            } else if (curr == '\n') {
                tokenizer->line++;
                tokenizer->column = 1;
                tokenizer->position++;
            } else {
                tokenizer->column++;
                tokenizer->position++;
            }
        }

        // Unterminated string
        return token_new(TOK_ERROR, &tokenizer->input[start_pos],
                         tokenizer->position - start_pos, start_line,
                         start_column, start_pos);
    }

    // Handle variable references ($var, ${var}, $(cmd), $((expr)))
    if (c == '$') {
        size_t start = tokenizer->position;
        tokenizer->position++;
        tokenizer->column++;

        if (tokenizer->position < tokenizer->input_length) {
            char next = tokenizer->input[tokenizer->position];

            if (next == '(') {
                // Check for $(( (arithmetic) or $( (command substitution)
                tokenizer->position++;
                tokenizer->column++;

                if (tokenizer->position < tokenizer->input_length &&
                    tokenizer->input[tokenizer->position] == '(') {
                    // Arithmetic expansion $((expr))
                    tokenizer->position++;
                    tokenizer->column++;

                    int paren_count = 2;
                    while (tokenizer->position < tokenizer->input_length &&
                           paren_count > 0) {
                        char curr = tokenizer->input[tokenizer->position];
                        if (curr == '(') {
                            paren_count++;
                        } else if (curr == ')') {
                            paren_count--;
                        } else if (curr == '\n') {
                            tokenizer->line++;
                            tokenizer->column = 0;
                        }
                        tokenizer->position++;
                        tokenizer->column++;
                    }

                    size_t length = tokenizer->position - start;
                    return token_new(TOK_ARITH_EXP, &tokenizer->input[start],
                                     length, start_line, start_column,
                                     start_pos);
                } else {
                    // Command substitution $(cmd)
                    int paren_count = 1;
                    while (tokenizer->position < tokenizer->input_length &&
                           paren_count > 0) {
                        char curr = tokenizer->input[tokenizer->position];
                        if (curr == '(') {
                            paren_count++;
                        } else if (curr == ')') {
                            paren_count--;
                        } else if (curr == '\n') {
                            tokenizer->line++;
                            tokenizer->column = 0;
                        }
                        tokenizer->position++;
                        tokenizer->column++;
                    }

                    size_t length = tokenizer->position - start;
                    return token_new(TOK_COMMAND_SUB, &tokenizer->input[start],
                                     length, start_line, start_column,
                                     start_pos);
                }
            } else if (next == '{') {
                // Parameter expansion ${var} with proper nested brace handling
                tokenizer->position++;
                tokenizer->column++;

                int brace_count = 1; // We've seen the opening brace
                while (tokenizer->position < tokenizer->input_length &&
                       brace_count > 0) {
                    char curr = tokenizer->input[tokenizer->position];

                    if (curr == '{') {
                        brace_count++;
                    } else if (curr == '}') {
                        brace_count--;
                    } else if (curr == '\n') {
                        tokenizer->line++;
                        tokenizer->column = 0;
                        tokenizer->position++;
                        continue;
                    }

                    tokenizer->position++;
                    tokenizer->column++;
                }

                size_t length = tokenizer->position - start;
                return token_new(TOK_VARIABLE, &tokenizer->input[start], length,
                                 start_line, start_column, start_pos);
            } else if (isalnum(next) || next == '_' || next == '?' ||
                       next == '$' || next == '!' || next == '@' ||
                       next == '*' || next == '#') {
                // Simple variable $var, $?, $$, $!, etc.

                // For special single-character variables, only advance by one
                if (next == '?' || next == '$' || next == '!' || next == '@' ||
                    next == '*' || next == '#') {
                    tokenizer
                        ->position++; // Just one character for special vars
                    tokenizer->column++;
                } else {
                    // For regular variables, continue until non-identifier
                    // character
                    while (tokenizer->position < tokenizer->input_length) {
                        char curr = tokenizer->input[tokenizer->position];
                        if (isalnum(curr) || curr == '_') {
                            tokenizer->position++;
                            tokenizer->column++;
                        } else {
                            break;
                        }
                    }
                }

                size_t length = tokenizer->position - start;
                return token_new(TOK_VARIABLE, &tokenizer->input[start], length,
                                 start_line, start_column, start_pos);
            }
        }

        // Just a plain $ - treat as word
        size_t length = tokenizer->position - start;
        return token_new(TOK_WORD, &tokenizer->input[start], length, start_line,
                         start_column, start_pos);
    }

    // Handle backtick command substitution
    if (c == '`') {
        size_t start = tokenizer->position;
        tokenizer->position++;
        tokenizer->column++;

        while (tokenizer->position < tokenizer->input_length &&
               tokenizer->input[tokenizer->position] != '`') {
            if (tokenizer->input[tokenizer->position] == '\n') {
                tokenizer->line++;
                tokenizer->column = 0;
            }
            tokenizer->position++;
            tokenizer->column++;
        }

        if (tokenizer->position < tokenizer->input_length) {
            tokenizer->position++; // Skip closing backtick
            tokenizer->column++;
        }

        size_t length = tokenizer->position - start;
        return token_new(TOK_BACKQUOTE, &tokenizer->input[start], length,
                         start_line, start_column, start_pos);
    }

    // Handle operators
    if (is_operator_char(c)) {
        switch (c) {
        case ';':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_SEMICOLON, ";", 1, start_line, start_column,
                             start_pos);

        case '|':
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == '|') {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_LOGICAL_OR, "||", 2, start_line,
                                 start_column, start_pos);
            } else {
                tokenizer->position++;
                tokenizer->column++;
                return token_new(TOK_PIPE, "|", 1, start_line, start_column,
                                 start_pos);
            }

        case '&':
            if (tokenizer->position + 1 < tokenizer->input_length) {
                char next = tokenizer->input[tokenizer->position + 1];
                if (next == '&') {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_LOGICAL_AND, "&&", 2, start_line,
                                     start_column, start_pos);
                } else if (next == '>') {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_REDIRECT_BOTH, "&>", 2, start_line,
                                     start_column, start_pos);
                }
            }
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_AND, "&", 1, start_line, start_column,
                             start_pos);

        case '<':
            if (tokenizer->position + 1 < tokenizer->input_length) {
                char next = tokenizer->input[tokenizer->position + 1];
                if (next == '<') {
                    if (tokenizer->position + 2 < tokenizer->input_length &&
                        tokenizer->input[tokenizer->position + 2] == '<') {
                        tokenizer->position += 3;
                        tokenizer->column += 3;
                        return token_new(TOK_HERESTRING, "<<<", 3, start_line,
                                         start_column, start_pos);
                    } else if (tokenizer->position + 2 <
                                   tokenizer->input_length &&
                               tokenizer->input[tokenizer->position + 2] ==
                                   '-') {
                        tokenizer->position += 3;
                        tokenizer->column += 3;
                        return token_new(TOK_HEREDOC_STRIP, "<<-", 3,
                                         start_line, start_column, start_pos);
                    } else {
                        tokenizer->position += 2;
                        tokenizer->column += 2;
                        return token_new(TOK_HEREDOC, "<<", 2, start_line,
                                         start_column, start_pos);
                    }
                }
            }
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_REDIRECT_IN, "<", 1, start_line, start_column,
                             start_pos);

        case '>':
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == '>') {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_APPEND, ">>", 2, start_line, start_column,
                                 start_pos);
            } else if (tokenizer->position + 1 < tokenizer->input_length &&
                       tokenizer->input[tokenizer->position + 1] == '|') {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_REDIRECT_CLOBBER, ">|", 2, start_line, start_column,
                                 start_pos);
            } else if (tokenizer->position + 1 < tokenizer->input_length &&
                       tokenizer->input[tokenizer->position + 1] == '&' &&
                       tokenizer->position + 2 < tokenizer->input_length &&
                       isdigit(tokenizer->input[tokenizer->position + 2])) {
                // Handle >&N pattern (redirect stdout to file descriptor N)
                tokenizer->position += 3;
                tokenizer->column += 3;
                return token_new(TOK_REDIRECT_FD, &tokenizer->input[start_pos],
                                 3, start_line, start_column, start_pos);
            } else {
                tokenizer->position++;
                tokenizer->column++;
                return token_new(TOK_REDIRECT_OUT, ">", 1, start_line,
                                 start_column, start_pos);
            }

        case '=':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_ASSIGN, "=", 1, start_line, start_column,
                             start_pos);

        case '!':
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == '=') {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_NOT_EQUAL, "!=", 2, start_line,
                                 start_column, start_pos);
            }
            // Standalone ! character (for test negation)
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_WORD, "!", 1, start_line, start_column, start_pos);

        case '+':
            // Let + be handled as part of words (e.g., date +%Y)
            // Fall through to word tokenization
            break;

        case '-':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_MINUS, "-", 1, start_line, start_column,
                             start_pos);

            // case '*':
            //     tokenizer->position++;
            //     tokenizer->column++;
            //     return token_new(TOK_MULTIPLY, "*", 1, start_line,
            //     start_column,
            //                      start_pos);

        case '%':
            // Let % be handled as part of words (e.g., +%Y format specifiers)
            // Fall through to word tokenization
            break;

            // case '?':
            //     tokenizer->position++;
            //     tokenizer->column++;
            //     return token_new(TOK_QUESTION, "?", 1, start_line,
            //     start_column,
            //                      start_pos);

        case '(':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_LPAREN, "(", 1, start_line, start_column,
                             start_pos);

        case ')':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_RPAREN, ")", 1, start_line, start_column,
                             start_pos);

        case '{':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_LBRACE, "{", 1, start_line, start_column,
                             start_pos);

        case '}':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_RBRACE, "}", 1, start_line, start_column,
                             start_pos);

        case '[':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_LBRACKET, "[", 1, start_line, start_column,
                             start_pos);

        case ']':
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_RBRACKET, "]", 1, start_line, start_column,
                             start_pos);
        }
    }

    // Handle numbered file descriptor redirections first
    if (isdigit(c)) {
        size_t num_start = tokenizer->position;
        // Read the number
        while (tokenizer->position < tokenizer->input_length &&
               isdigit(tokenizer->input[tokenizer->position])) {
            tokenizer->position++;
            tokenizer->column++;
        }

        // Check if followed by >, >>, or >&
        if (tokenizer->position < tokenizer->input_length) {
            if (tokenizer->input[tokenizer->position] == '>') {
                if (tokenizer->position + 1 < tokenizer->input_length &&
                    tokenizer->input[tokenizer->position + 1] == '>') {
                    // Handle N>> (append to file descriptor N)
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    size_t length = tokenizer->position - num_start;
                    return token_new(TOK_APPEND_ERR,
                                     &tokenizer->input[num_start], length,
                                     start_line, start_column, start_pos);
                } else if (tokenizer->position + 1 < tokenizer->input_length &&
                           tokenizer->input[tokenizer->position + 1] == '&') {
                    // Check for N>&M pattern (redirect file descriptor N to M)
                    if (tokenizer->position + 2 < tokenizer->input_length &&
                        isdigit(tokenizer->input[tokenizer->position + 2])) {
                        tokenizer->position += 3; // Skip >&M
                        tokenizer->column += 3;
                        size_t length = tokenizer->position - num_start;
                        return token_new(TOK_REDIRECT_FD,
                                         &tokenizer->input[num_start], length,
                                         start_line, start_column, start_pos);
                    }
                } else {
                    // Handle N> (redirect file descriptor N)
                    tokenizer->position++;
                    tokenizer->column++;
                    size_t length = tokenizer->position - num_start;
                    return token_new(TOK_REDIRECT_ERR,
                                     &tokenizer->input[num_start], length,
                                     start_line, start_column, start_pos);
                }
            }
        }

        // Not a redirection, reset position and treat as regular number
        tokenizer->position = num_start;
        tokenizer->column = start_column;
    }

    // Handle words and numbers
    if (isalnum(c) || is_word_char(c)) {
        size_t start = tokenizer->position;
        bool is_numeric = isdigit(c);

        while (tokenizer->position < tokenizer->input_length) {
            char curr = tokenizer->input[tokenizer->position];
            if (is_word_char(curr) || isalnum(curr)) {
                if (!isdigit(curr)) {
                    is_numeric = false;
                }
                tokenizer->position++;
                tokenizer->column++;
            } else {
                break;
            }
        }

        size_t length = tokenizer->position - start;
        token_type_t type =
            is_numeric ? TOK_NUMBER
                       : classify_word(&tokenizer->input[start], length,
                                       tokenizer->enable_keywords);

        return token_new(type, &tokenizer->input[start], length, start_line,
                         start_column, start_pos);
    }

    // Unknown character - treat as error
    tokenizer->position++;
    tokenizer->column++;
    return token_new(TOK_ERROR, &tokenizer->input[start_pos], 1, start_line,
                     start_column, start_pos);
}
