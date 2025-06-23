/**
 * Modern Tokenizer for POSIX Shell - Designed for Recursive Descent Parsing
 * 
 * This tokenizer replaces the old scanner with a clean, simple design that properly
 * supports recursive descent parsing with lookahead and proper token boundaries.
 */

#ifndef TOKENIZER_NEW_H
#define TOKENIZER_NEW_H

#include <stdbool.h>
#include <stddef.h>

// Simple, clean token classification for modern parser
typedef enum {
    // Basic token types
    MODERN_TOK_EOF,          // End of input
    MODERN_TOK_WORD,         // Regular word (command, argument, variable name)
    MODERN_TOK_STRING,       // Quoted string ('...' - literal)
    MODERN_TOK_EXPANDABLE_STRING, // Double-quoted string ("...") - needs variable expansion
    MODERN_TOK_NUMBER,       // Numeric literal
    MODERN_TOK_VARIABLE,     // Variable reference ($var, ${var}, etc.)
    
    // Operators and separators
    MODERN_TOK_SEMICOLON,    // ;
    MODERN_TOK_PIPE,         // |
    MODERN_TOK_AND,          // &
    MODERN_TOK_LOGICAL_AND,  // &&
    MODERN_TOK_LOGICAL_OR,   // ||
    MODERN_TOK_REDIRECT_IN,  // <
    MODERN_TOK_REDIRECT_OUT, // >
    MODERN_TOK_APPEND,       // >>
    MODERN_TOK_HEREDOC,      // <<
    MODERN_TOK_HEREDOC_STRIP, // <<-
    MODERN_TOK_HERESTRING,   // <<<
    MODERN_TOK_REDIRECT_ERR, // 2>
    MODERN_TOK_REDIRECT_BOTH, // &>
    MODERN_TOK_APPEND_ERR,   // 2>>
    MODERN_TOK_REDIRECT_FD,  // &1, &2, etc.
    MODERN_TOK_ASSIGN,       // =
    MODERN_TOK_PLUS,         // +
    MODERN_TOK_MINUS,        // -
    MODERN_TOK_MULTIPLY,     // *
    MODERN_TOK_DIVIDE,       // /
    MODERN_TOK_MODULO,       // %
    MODERN_TOK_GLOB,         // * (when used for globbing)
    MODERN_TOK_QUESTION,     // ?
    MODERN_TOK_COMMAND_SUB,  // $(...) 
    MODERN_TOK_ARITH_EXP,    // $((...))
    MODERN_TOK_BACKQUOTE,    // `
    
    // Delimiters
    MODERN_TOK_LPAREN,       // (
    MODERN_TOK_RPAREN,       // )
    MODERN_TOK_LBRACE,       // {
    MODERN_TOK_RBRACE,       // }
    MODERN_TOK_LBRACKET,     // [
    MODERN_TOK_RBRACKET,     // ]
    
    // Keywords (recognized contextually)
    MODERN_TOK_IF,
    MODERN_TOK_THEN,
    MODERN_TOK_ELSE,
    MODERN_TOK_ELIF,
    MODERN_TOK_FI,
    MODERN_TOK_WHILE,
    MODERN_TOK_DO,
    MODERN_TOK_DONE,
    MODERN_TOK_FOR,
    MODERN_TOK_IN,
    MODERN_TOK_CASE,
    MODERN_TOK_ESAC,
    MODERN_TOK_UNTIL,
    MODERN_TOK_FUNCTION,
    
    // Special
    MODERN_TOK_NEWLINE,      // \n (significant in shell)
    MODERN_TOK_WHITESPACE,   // Spaces, tabs (usually ignored)
    MODERN_TOK_COMMENT,      // # comment
    MODERN_TOK_ERROR         // Invalid token
} modern_token_type_t;

// Token structure for modern parser
typedef struct modern_token {
    modern_token_type_t type;
    char *text;           // Token text (null-terminated)
    size_t length;        // Token length
    size_t line;          // Line number (1-based)
    size_t column;        // Column number (1-based)
    size_t position;      // Absolute position in input
    struct modern_token *next;   // For token stream
} modern_token_t;

// Tokenizer state for modern parser
typedef struct modern_tokenizer {
    const char *input;    // Input string
    size_t input_length;  // Input length
    size_t position;      // Current position
    size_t line;          // Current line (1-based)
    size_t column;        // Current column (1-based)
    modern_token_t *current;     // Current token
    modern_token_t *lookahead;   // Next token (for lookahead)
    bool enable_keywords; // Whether to recognize keywords (context-sensitive)
} modern_tokenizer_t;

// Modern tokenizer interface
modern_tokenizer_t *modern_tokenizer_new(const char *input);
void modern_tokenizer_free(modern_tokenizer_t *tokenizer);

// Token operations
modern_token_t *modern_tokenizer_current(modern_tokenizer_t *tokenizer);
modern_token_t *modern_tokenizer_peek(modern_tokenizer_t *tokenizer);
void modern_tokenizer_advance(modern_tokenizer_t *tokenizer);
bool modern_tokenizer_match(modern_tokenizer_t *tokenizer, modern_token_type_t type);
bool modern_tokenizer_consume(modern_tokenizer_t *tokenizer, modern_token_type_t type);

// Token utility functions
const char *modern_token_type_name(modern_token_type_t type);
bool modern_token_is_keyword(modern_token_type_t type);
bool modern_token_is_operator(modern_token_type_t type);
bool modern_token_is_word_like(modern_token_type_t type);

// Token control
void modern_tokenizer_enable_keywords(modern_tokenizer_t *tokenizer, bool enable);

#endif // TOKENIZER_NEW_H
