/**
 * Tokenizer for POSIX Shell - Designed for Recursive Descent Parsing
 *
 * This tokenizer provides a clean, simple design that properly
 * supports recursive descent parsing with lookahead and proper token
 * boundaries.
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>
#include <stddef.h>

// Simple, clean token classification for parser
typedef enum {
    // Basic token types
    TOK_EOF,               // End of input
    TOK_WORD,              // Regular word (command, argument, variable name)
    TOK_STRING,            // Quoted string ('...' - literal)
    TOK_EXPANDABLE_STRING, // Double-quoted string ("...") - needs variable
                           // expansion
    TOK_NUMBER,            // Numeric literal
    TOK_VARIABLE,          // Variable reference ($var, ${var}, etc.)

    // Operators and separators
    TOK_SEMICOLON,     // ;
    TOK_PIPE,          // |
    TOK_AND,           // &
    TOK_LOGICAL_AND,   // &&
    TOK_LOGICAL_OR,    // ||
    TOK_REDIRECT_IN,   // <
    TOK_REDIRECT_OUT,  // >
    TOK_APPEND,        // >>
    TOK_HEREDOC,       // <<
    TOK_HEREDOC_STRIP, // <<-
    TOK_HERESTRING,    // <<<
    TOK_REDIRECT_ERR,  // 2>
    TOK_REDIRECT_BOTH, // &>
    TOK_APPEND_ERR,    // 2>>
    TOK_REDIRECT_FD,   // &1, &2, etc.
    TOK_ASSIGN,        // =
    TOK_PLUS,          // +
    TOK_MINUS,         // -
    TOK_MULTIPLY,      // *
    TOK_DIVIDE,        // /
    TOK_MODULO,        // %
    TOK_GLOB,          // * (when used for globbing)
    TOK_QUESTION,      // ?
    TOK_COMMAND_SUB,   // $(...)
    TOK_ARITH_EXP,     // $((...))
    TOK_BACKQUOTE,     // `

    // Delimiters
    TOK_LPAREN,   // (
    TOK_RPAREN,   // )
    TOK_LBRACE,   // {
    TOK_RBRACE,   // }
    TOK_LBRACKET, // [
    TOK_RBRACKET, // ]

    // Keywords (recognized contextually)
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_ELIF,
    TOK_FI,
    TOK_WHILE,
    TOK_DO,
    TOK_DONE,
    TOK_FOR,
    TOK_IN,
    TOK_CASE,
    TOK_ESAC,
    TOK_UNTIL,
    TOK_FUNCTION,

    // Special
    TOK_NEWLINE,    // \n (significant in shell)
    TOK_WHITESPACE, // Spaces, tabs (usually ignored)
    TOK_COMMENT,    // # comment
    TOK_ERROR       // Invalid token
} token_type_t;

// Token structure for parser
typedef struct token {
    token_type_t type;
    char *text;         // Token text (null-terminated)
    size_t length;      // Token length
    size_t line;        // Line number (1-based)
    size_t column;      // Column number (1-based)
    size_t position;    // Absolute position in input
    struct token *next; // For token stream
} token_t;

// Tokenizer state for parser
typedef struct tokenizer {
    const char *input;    // Input string
    size_t input_length;  // Input length
    size_t position;      // Current position
    size_t line;          // Current line (1-based)
    size_t column;        // Current column (1-based)
    token_t *current;     // Current token
    token_t *lookahead;   // Next token (for lookahead)
    bool enable_keywords; // Whether to recognize keywords (context-sensitive)
} tokenizer_t;

// Tokenizer interface
tokenizer_t *tokenizer_new(const char *input);
void tokenizer_free(tokenizer_t *tokenizer);

// Token operations
token_t *tokenizer_current(tokenizer_t *tokenizer);
token_t *tokenizer_peek(tokenizer_t *tokenizer);
void tokenizer_advance(tokenizer_t *tokenizer);
bool tokenizer_match(tokenizer_t *tokenizer, token_type_t type);
bool tokenizer_consume(tokenizer_t *tokenizer, token_type_t type);

// Token utility functions
const char *token_type_name(token_type_t type);
bool token_is_keyword(token_type_t type);
bool token_is_operator(token_type_t type);
bool token_is_word_like(token_type_t type);

// Token control
void tokenizer_enable_keywords(tokenizer_t *tokenizer, bool enable);
void tokenizer_refresh_from_position(tokenizer_t *tokenizer);

#endif // TOKENIZER_H
