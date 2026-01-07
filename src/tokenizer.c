/**
 * @file tokenizer.c
 * @brief Shell Command Tokenizer Implementation
 *
 * Lexical analyzer for POSIX shell syntax. Handles tokenization of
 * shell commands including operators, keywords, quoting, and escapes.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2025 Michael Berry. All rights reserved.
 */

#include "tokenizer.h"
#include "lle/utf8_support.h"
#include "shell_mode.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Keyword lookup table
static const struct {
    const char *text;
    token_type_t type;
} keywords[] = {
    {"if", TOK_IF},       {"then", TOK_THEN},
    {"else", TOK_ELSE},   {"elif", TOK_ELIF},
    {"fi", TOK_FI},       {"while", TOK_WHILE},
    {"do", TOK_DO},       {"done", TOK_DONE},
    {"for", TOK_FOR},     {"in", TOK_IN},
    {"case", TOK_CASE},   {"esac", TOK_ESAC},
    {"until", TOK_UNTIL}, {"function", TOK_FUNCTION},
    {"select", TOK_SELECT}, {"time", TOK_TIME},
    {NULL, TOK_WORD} // Sentinel
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

/**
 * @brief Create a new tokenizer instance
 *
 * Initializes a tokenizer for the given input string and
 * pre-tokenizes the first two tokens (current and lookahead).
 *
 * @param input Shell command string to tokenize
 * @return New tokenizer instance, or NULL on failure
 */
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

/**
 * @brief Free a tokenizer instance
 *
 * Frees the tokenizer and any associated tokens.
 *
 * @param tokenizer Tokenizer to free
 */
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

/**
 * @brief Get the current token
 *
 * @param tokenizer Tokenizer instance
 * @return Current token, or NULL if tokenizer is NULL
 */
token_t *tokenizer_current(tokenizer_t *tokenizer) {
    return tokenizer ? tokenizer->current : NULL;
}

/**
 * @brief Peek at the next token without consuming
 *
 * @param tokenizer Tokenizer instance
 * @return Lookahead token, or NULL if tokenizer is NULL
 */
token_t *tokenizer_peek(tokenizer_t *tokenizer) {
    return tokenizer ? tokenizer->lookahead : NULL;
}

/**
 * @brief Advance to the next token
 *
 * Frees the current token, moves lookahead to current,
 * and tokenizes the next lookahead.
 *
 * @param tokenizer Tokenizer instance
 */
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

/**
 * @brief Check if current token matches expected type
 *
 * @param tokenizer Tokenizer instance
 * @param type Token type to match
 * @return true if current token matches type
 */
bool tokenizer_match(tokenizer_t *tokenizer, token_type_t type) {
    if (!tokenizer || !tokenizer->current) {
        return false;
    }
    return tokenizer->current->type == type;
}

/**
 * @brief Consume a token if it matches expected type
 *
 * Advances past the token if it matches, otherwise leaves
 * the tokenizer state unchanged.
 *
 * @param tokenizer Tokenizer instance
 * @param type Token type to match and consume
 * @return true if token was matched and consumed
 */
bool tokenizer_consume(tokenizer_t *tokenizer, token_type_t type) {
    if (tokenizer_match(tokenizer, type)) {
        tokenizer_advance(tokenizer);
        return true;
    }
    return false;
}

/**
 * @brief Get human-readable name for token type
 *
 * Returns a string representation of the token type
 * for debugging and error messages.
 *
 * @param type Token type
 * @return String name of token type
 */
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
    case TOK_DOUBLE_LPAREN:
        return "DOUBLE_LPAREN";
    case TOK_DOUBLE_RPAREN:
        return "DOUBLE_RPAREN";
    case TOK_LBRACE:
        return "LBRACE";
    case TOK_RBRACE:
        return "RBRACE";
    case TOK_LBRACKET:
        return "LBRACKET";
    case TOK_RBRACKET:
        return "RBRACKET";
    case TOK_DOUBLE_LBRACKET:
        return "DOUBLE_LBRACKET";
    case TOK_DOUBLE_RBRACKET:
        return "DOUBLE_RBRACKET";
    case TOK_PLUS_ASSIGN:
        return "PLUS_ASSIGN";
    case TOK_REGEX_MATCH:
        return "REGEX_MATCH";
    case TOK_PROC_SUB_IN:
        return "PROC_SUB_IN";
    case TOK_PROC_SUB_OUT:
        return "PROC_SUB_OUT";
    case TOK_PIPE_STDERR:
        return "PIPE_STDERR";
    case TOK_APPEND_BOTH:
        return "APPEND_BOTH";
    case TOK_COPROC:
        return "COPROC";
    case TOK_CASE_FALLTHROUGH:
        return "CASE_FALLTHROUGH";
    case TOK_CASE_CONTINUE:
        return "CASE_CONTINUE";
    case TOK_SELECT:
        return "SELECT";
    case TOK_TIME:
        return "TIME";
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

/**
 * @brief Check if token type is a shell keyword
 *
 * Keywords include: if, then, else, elif, fi, while, do, done,
 * for, in, case, esac, until, function.
 *
 * @param type Token type to check
 * @return true if token is a keyword
 */
bool token_is_keyword(token_type_t type) {
    return type >= TOK_IF && type <= TOK_TIME;
}

/**
 * @brief Check if token type is an operator
 *
 * Operators include: ;, |, &, &&, ||, <, >, >>, <<, =, etc.
 *
 * @param type Token type to check
 * @return true if token is an operator
 */
bool token_is_operator(token_type_t type) {
    return (type >= TOK_SEMICOLON && type <= TOK_REDIRECT_CLOBBER) ||
           (type >= TOK_ASSIGN && type <= TOK_BACKQUOTE);
}

/**
 * @brief Check if token can be part of a command/argument
 *
 * Word-like tokens include: WORD, STRING, EXPANDABLE_STRING,
 * NUMBER, and VARIABLE.
 *
 * @param type Token type to check
 * @return true if token is word-like
 */
bool token_is_word_like(token_type_t type) {
    return type == TOK_WORD || type == TOK_STRING ||
           type == TOK_EXPANDABLE_STRING || type == TOK_NUMBER ||
           type == TOK_VARIABLE;
}

/**
 * @brief Enable or disable keyword recognition
 *
 * When disabled, keywords like 'if', 'while' are treated as
 * regular words. Useful in certain parsing contexts.
 *
 * @param tokenizer Tokenizer instance
 * @param enable true to enable keyword recognition
 */
void tokenizer_enable_keywords(tokenizer_t *tokenizer, bool enable) {
    if (tokenizer) {
        tokenizer->enable_keywords = enable;
    }
}

/**
 * @brief Refresh tokenizer from current position
 *
 * Discards current and lookahead tokens and re-tokenizes
 * from the current input position. Used for error recovery
 * and after manually advancing the position (e.g., here-documents).
 *
 * @param tokenizer Tokenizer instance
 */
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

/* ========== Helper Functions ========== */

/**
 * @brief Create a new token
 *
 * Allocates and initializes a token with the given properties.
 *
 * @param type Token type
 * @param text Token text (will be copied)
 * @param length Length of text
 * @param line Source line number
 * @param column Source column number
 * @param position Byte offset in input
 * @return New token, or NULL on failure
 */
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
        memcpy(token->text, text, length);
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

/**
 * @brief Free a token
 *
 * @param token Token to free
 */
static void token_free(token_t *token) {
    if (!token) {
        return;
    }

    if (token->text) {
        free(token->text);
    }
    free(token);
}

/**
 * @brief Classify a word as keyword or regular word
 *
 * Checks the word against the keyword lookup table.
 *
 * @param text Word text
 * @param length Word length
 * @param enable_keywords If false, always returns TOK_WORD
 * @return Token type (keyword type or TOK_WORD)
 */
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

/**
 * @brief Check if character can start/be part of an operator
 *
 * @param c Character to check
 * @return true if character is an operator character
 */
static bool is_operator_char(char c) {
    return strchr(";|&<>=+*%?(){}[]#!", c) != NULL;
}

/**
 * @brief Check if ASCII character can be part of a word
 *
 * Word characters include alphanumerics and: _.-/~:@*?[]+%
 *
 * @param c Character to check
 * @return true if character can be part of a word
 */
static bool is_word_char(char c) {
    return isalnum(c) || strchr("_.-/~:@*?[]+%", c) != NULL;
}

/**
 * @brief Check if Unicode codepoint can be part of a word
 *
 * For ASCII (< 0x80), uses traditional shell word character logic.
 * For non-ASCII, all Unicode codepoints are valid word characters,
 * allowing filenames with international characters.
 *
 * @param codepoint Unicode codepoint
 * @return true if codepoint can be part of a word
 */
static bool is_word_codepoint(uint32_t codepoint) {
    // ASCII range: Use traditional shell word character logic
    if (codepoint < 0x80) {
        char c = (char)codepoint;
        return isalnum(c) || strchr("_.-/~:@*?[]+%", c) != NULL;
    }

    // Non-ASCII UTF-8: All non-ASCII codepoints are valid word characters
    // This includes:
    // - Latin Extended (accented characters like é, ñ, ü)
    // - CJK (Chinese, Japanese, Korean)
    // - Emoji
    // - All other Unicode scripts
    //
    // We explicitly exclude:
    // - ASCII control characters (0x00-0x1F, already < 0x80)
    // - Invalid codepoints (handled by UTF-8 decoder)
    //
    // Shell metacharacters (quotes, pipes, etc.) are all ASCII (< 0x80),
    // so they're handled by the ASCII logic above.
    return true;
}

/**
 * @brief Skip whitespace characters
 *
 * Advances past spaces and tabs, but not newlines
 * (which are significant in shell syntax).
 *
 * @param tokenizer Tokenizer instance
 */
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

/**
 * @brief Tokenize the next token from input
 *
 * Main tokenization function that handles:
 * - Newlines and comments
 * - Single and double quoted strings
 * - Variable references ($var, ${var}, $(cmd), $((expr)))
 * - Backtick command substitution
 * - All operators and redirections
 * - Keywords and words (UTF-8 aware)
 * - Numbered file descriptor redirections
 *
 * @param tokenizer Tokenizer instance
 * @return Next token, or TOK_EOF at end of input
 */
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
            // Check for ;;& (case continue - test next pattern)
            if (tokenizer->position + 2 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == ';' &&
                tokenizer->input[tokenizer->position + 2] == '&') {
                tokenizer->position += 3;
                tokenizer->column += 3;
                return token_new(TOK_CASE_CONTINUE, ";;&", 3, start_line,
                                 start_column, start_pos);
            }
            // Check for ;& (case fall-through - execute next without test)
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == '&') {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_CASE_FALLTHROUGH, ";&", 2, start_line,
                                 start_column, start_pos);
            }
            // Regular semicolon
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_SEMICOLON, ";", 1, start_line, start_column,
                             start_pos);

        case '|':
            if (tokenizer->position + 1 < tokenizer->input_length) {
                char next = tokenizer->input[tokenizer->position + 1];
                if (next == '|') {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_LOGICAL_OR, "||", 2, start_line,
                                     start_column, start_pos);
                }
                // Pipe stderr |& (shorthand for 2>&1 |)
                if (next == '&' &&
                    shell_mode_allows(FEATURE_PROCESS_SUBSTITUTION)) {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_PIPE_STDERR, "|&", 2, start_line,
                                     start_column, start_pos);
                }
            }
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_PIPE, "|", 1, start_line, start_column,
                             start_pos);

        case '&':
            if (tokenizer->position + 1 < tokenizer->input_length) {
                char next = tokenizer->input[tokenizer->position + 1];
                if (next == '&') {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_LOGICAL_AND, "&&", 2, start_line,
                                     start_column, start_pos);
                } else if (next == '>') {
                    // Check for &>> (append both stdout and stderr)
                    if (tokenizer->position + 2 < tokenizer->input_length &&
                        tokenizer->input[tokenizer->position + 2] == '>' &&
                        shell_mode_allows(FEATURE_PROCESS_SUBSTITUTION)) {
                        tokenizer->position += 3;
                        tokenizer->column += 3;
                        return token_new(TOK_APPEND_BOTH, "&>>", 3, start_line,
                                         start_column, start_pos);
                    }
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
                // Process substitution <( - check before heredoc
                if (next == '(' && 
                    shell_mode_allows(FEATURE_PROCESS_SUBSTITUTION)) {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_PROC_SUB_IN, "<(", 2, start_line,
                                     start_column, start_pos);
                }
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
            if (tokenizer->position + 1 < tokenizer->input_length) {
                char next = tokenizer->input[tokenizer->position + 1];
                // Process substitution >(
                if (next == '(' &&
                    shell_mode_allows(FEATURE_PROCESS_SUBSTITUTION)) {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_PROC_SUB_OUT, ">(", 2, start_line,
                                     start_column, start_pos);
                }
                if (next == '>') {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_APPEND, ">>", 2, start_line, start_column,
                                     start_pos);
                }
                if (next == '|') {
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    return token_new(TOK_REDIRECT_CLOBBER, ">|", 2, start_line,
                                     start_column, start_pos);
                }
                if (next == '&' &&
                    tokenizer->position + 2 < tokenizer->input_length &&
                    isdigit(tokenizer->input[tokenizer->position + 2])) {
                    // Handle >&N pattern (redirect stdout to file descriptor N)
                    tokenizer->position += 3;
                    tokenizer->column += 3;
                    return token_new(TOK_REDIRECT_FD, &tokenizer->input[start_pos],
                                     3, start_line, start_column, start_pos);
                }
            }
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_REDIRECT_OUT, ">", 1, start_line,
                             start_column, start_pos);

        case '=':
            // Check for =~ regex match operator (inside [[ ]])
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == '~' &&
                shell_mode_allows(FEATURE_REGEX_MATCH)) {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_REGEX_MATCH, "=~", 2, start_line,
                                 start_column, start_pos);
            }
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
            return token_new(TOK_WORD, "!", 1, start_line, start_column,
                             start_pos);

        case '+':
            // Check for += (append/add assignment)
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == '=' &&
                shell_mode_allows(FEATURE_INDEXED_ARRAYS)) {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_PLUS_ASSIGN, "+=", 2, start_line,
                                 start_column, start_pos);
            }
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
            // Check for (( arithmetic command
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == '(' &&
                shell_mode_allows(FEATURE_ARITH_COMMAND)) {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_DOUBLE_LPAREN, "((", 2, start_line,
                                 start_column, start_pos);
            }
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_LPAREN, "(", 1, start_line, start_column,
                             start_pos);

        case ')':
            // Check for )) arithmetic command end
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == ')' &&
                shell_mode_allows(FEATURE_ARITH_COMMAND)) {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_DOUBLE_RPAREN, "))", 2, start_line,
                                 start_column, start_pos);
            }
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
            // Check for [[ extended test
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == '[' &&
                shell_mode_allows(FEATURE_EXTENDED_TEST)) {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_DOUBLE_LBRACKET, "[[", 2, start_line,
                                 start_column, start_pos);
            }
            tokenizer->position++;
            tokenizer->column++;
            return token_new(TOK_LBRACKET, "[", 1, start_line, start_column,
                             start_pos);

        case ']':
            // Check for ]] extended test end
            if (tokenizer->position + 1 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position + 1] == ']' &&
                shell_mode_allows(FEATURE_EXTENDED_TEST)) {
                tokenizer->position += 2;
                tokenizer->column += 2;
                return token_new(TOK_DOUBLE_RBRACKET, "]]", 2, start_line,
                                 start_column, start_pos);
            }
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

    // Handle words and numbers (UTF-8 aware)
    // First, try to decode the current character as UTF-8
    uint32_t codepoint;
    int char_len = lle_utf8_decode_codepoint(
        &tokenizer->input[tokenizer->position],
        tokenizer->input_length - tokenizer->position, &codepoint);

    // Check if this could be the start of a word
    // (either ASCII word char or non-ASCII UTF-8)
    bool could_be_word = false;
    if (char_len > 0) {
        could_be_word = is_word_codepoint(codepoint);
    } else if (isalnum(c) || is_word_char(c)) {
        // Fallback for byte-level check (ASCII)
        could_be_word = true;
    }

    if (could_be_word) {
        size_t start = tokenizer->position;
        bool is_numeric = (char_len == 1 && isdigit(c));

        // Scan word character by character (UTF-8 aware)
        while (tokenizer->position < tokenizer->input_length) {
            // Try to decode UTF-8 codepoint at current position
            uint32_t curr_codepoint;
            int curr_char_len = lle_utf8_decode_codepoint(
                &tokenizer->input[tokenizer->position],
                tokenizer->input_length - tokenizer->position, &curr_codepoint);

            if (curr_char_len > 0) {
                // Valid UTF-8 character - check if it's a word character
                if (is_word_codepoint(curr_codepoint)) {
                    // Check if still numeric (only single-byte ASCII digits
                    // count)
                    if (curr_char_len > 1 ||
                        !isdigit(tokenizer->input[tokenizer->position])) {
                        is_numeric = false;
                    }

                    // Advance by the UTF-8 character length
                    tokenizer->position += curr_char_len;
                    tokenizer->column++; // One visual column per character
                } else {
                    // Not a word character - end of word
                    break;
                }
            } else {
                // Invalid UTF-8 sequence - treat as end of word
                break;
            }
        }

        // Check for glob qualifier suffix: (.) (/) (@) (*) (r) (w)
        // Only if FEATURE_GLOB_QUALIFIERS is enabled and word contains glob chars
        if (shell_mode_allows(FEATURE_GLOB_QUALIFIERS)) {
            size_t word_len = tokenizer->position - start;
            bool has_glob_char = false;
            for (size_t i = 0; i < word_len; i++) {
                if (tokenizer->input[start + i] == '*' ||
                    tokenizer->input[start + i] == '?' ||
                    tokenizer->input[start + i] == '[') {
                    has_glob_char = true;
                    break;
                }
            }
            
            if (has_glob_char &&
                tokenizer->position + 2 < tokenizer->input_length &&
                tokenizer->input[tokenizer->position] == '(') {
                char qual = tokenizer->input[tokenizer->position + 1];
                if ((qual == '.' || qual == '/' || qual == '@' ||
                     qual == '*' || qual == 'r' || qual == 'w') &&
                    tokenizer->input[tokenizer->position + 2] == ')') {
                    // Include the glob qualifier in the word token
                    tokenizer->position += 3;
                    tokenizer->column += 3;
                }
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
