/**
 * Modern Tokenizer Implementation for POSIX Shell
 * 
 * Clean, simple tokenizer designed specifically for recursive descent parsing.
 * Handles shell-specific tokenization rules while maintaining parser simplicity.
 */

#include "tokenizer_new.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Keyword lookup table
static const struct {
    const char *text;
    modern_token_type_t type;
} keywords[] = {
    {"if", MODERN_TOK_IF},
    {"then", MODERN_TOK_THEN},
    {"else", MODERN_TOK_ELSE},
    {"elif", MODERN_TOK_ELIF},
    {"fi", MODERN_TOK_FI},
    {"while", MODERN_TOK_WHILE},
    {"do", MODERN_TOK_DO},
    {"done", MODERN_TOK_DONE},
    {"for", MODERN_TOK_FOR},
    {"in", MODERN_TOK_IN},
    {"case", MODERN_TOK_CASE},
    {"esac", MODERN_TOK_ESAC},
    {"until", MODERN_TOK_UNTIL},
    {"function", MODERN_TOK_FUNCTION},
    {NULL, MODERN_TOK_WORD}  // Sentinel
};

// Helper functions
static modern_token_t *token_new(modern_token_type_t type, const char *text, size_t length, 
                         size_t line, size_t column, size_t position);
static void modern_token_free(modern_token_t *token);
static modern_token_t *tokenize_next(modern_tokenizer_t *tokenizer);
static modern_token_type_t classify_word(const char *text, size_t length, bool enable_keywords);
static bool is_operator_char(char c);
static bool is_word_char(char c);
static void skip_whitespace(modern_tokenizer_t *tokenizer);

// Create new tokenizer
modern_tokenizer_t *modern_tokenizer_new(const char *input) {
    if (!input) return NULL;
    
    modern_tokenizer_t *tokenizer = malloc(sizeof(modern_tokenizer_t));
    if (!tokenizer) return NULL;
    
    tokenizer->input = input;
    tokenizer->input_length = strlen(input);
    tokenizer->position = 0;
    tokenizer->line = 1;
    tokenizer->column = 1;
    tokenizer->current = NULL;
    tokenizer->lookahead = NULL;
    tokenizer->enable_keywords = true;
    
    // Initialize with first token
    tokenizer->current = tokenize_next(tokenizer);
    tokenizer->lookahead = tokenize_next(tokenizer);
    
    return tokenizer;
}

// Free tokenizer
void modern_tokenizer_free(modern_tokenizer_t *tokenizer) {
    if (!tokenizer) return;
    
    modern_token_free(tokenizer->current);
    modern_token_free(tokenizer->lookahead);
    free(tokenizer);
}

// Get current token
modern_token_t *modern_tokenizer_current(modern_tokenizer_t *tokenizer) {
    return tokenizer ? tokenizer->current : NULL;
}

// Peek at next token
modern_token_t *modern_tokenizer_peek(modern_tokenizer_t *tokenizer) {
    return tokenizer ? tokenizer->lookahead : NULL;
}

// Advance to next token
void modern_tokenizer_advance(modern_tokenizer_t *tokenizer) {
    if (!tokenizer) return;
    
    modern_token_free(tokenizer->current);
    tokenizer->current = tokenizer->lookahead;
    tokenizer->lookahead = tokenize_next(tokenizer);
}

// Check if current token matches type
bool modern_tokenizer_match(modern_tokenizer_t *tokenizer, modern_token_type_t type) {
    return tokenizer && tokenizer->current && tokenizer->current->type == type;
}

// Consume token if it matches type
bool modern_tokenizer_consume(modern_tokenizer_t *tokenizer, modern_token_type_t type) {
    if (modern_tokenizer_match(tokenizer, type)) {
        modern_tokenizer_advance(tokenizer);
        return true;
    }
    return false;
}

// Control keyword recognition
void modern_tokenizer_enable_keywords(modern_tokenizer_t *tokenizer, bool enable) {
    if (tokenizer) {
        tokenizer->enable_keywords = enable;
    }
}

// Token creation
static modern_token_t *token_new(modern_token_type_t type, const char *text, size_t length,
                         size_t line, size_t column, size_t position) {
    modern_token_t *token = malloc(sizeof(modern_token_t));
    if (!token) return NULL;
    
    token->type = type;
    token->length = length;
    token->line = line;
    token->column = column;
    token->position = position;
    token->next = NULL;
    
    // Copy text
    token->text = malloc(length + 1);
    if (!token->text) {
        free(token);
        return NULL;
    }
    memcpy(token->text, text, length);
    token->text[length] = '\0';
    
    return token;
}

// Free token
static void modern_token_free(modern_token_t *token) {
    if (token) {
        free(token->text);
        free(token);
    }
}

// Skip whitespace (but track position)
static void skip_whitespace(modern_tokenizer_t *tokenizer) {
    while (tokenizer->position < tokenizer->input_length) {
        char c = tokenizer->input[tokenizer->position];
        if (c == ' ' || c == '\t' || c == '\r') {
            tokenizer->position++;
            tokenizer->column++;
        } else {
            break;
        }
    }
}

// Check if character can be part of a word
static bool is_word_char(char c) {
    return isalnum(c) || c == '_' || c == '-' || c == '.' || c == '/' || c == '~' || c == ':' || c == '@';
}

// Check if character is an operator
static bool is_operator_char(char c) {
    return strchr("|&;<>(){}[]!=+-*/%?", c) != NULL;
}

// Classify word as keyword or regular word
static modern_token_type_t classify_word(const char *text, size_t length, bool enable_keywords) {
    if (!enable_keywords) {
        return MODERN_TOK_WORD;
    }
    
    for (int i = 0; keywords[i].text; i++) {
        if (strlen(keywords[i].text) == length && 
            strncmp(text, keywords[i].text, length) == 0) {
            return keywords[i].type;
        }
    }
    
    return MODERN_TOK_WORD;
}

// Main tokenization function
static modern_token_t *tokenize_next(modern_tokenizer_t *tokenizer) {
    if (!tokenizer || tokenizer->position >= tokenizer->input_length) {
        return token_new(MODERN_TOK_EOF, "", 0, tokenizer ? tokenizer->line : 1, 
                        tokenizer ? tokenizer->column : 1, 
                        tokenizer ? tokenizer->position : 0);
    }
    
    skip_whitespace(tokenizer);
    
    if (tokenizer->position >= tokenizer->input_length) {
        return token_new(MODERN_TOK_EOF, "", 0, tokenizer->line, tokenizer->column, tokenizer->position);
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
        return token_new(MODERN_TOK_NEWLINE, "\n", 1, start_line, start_column, start_pos);
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
        return token_new(MODERN_TOK_COMMENT, &tokenizer->input[start], length, 
                        start_line, start_column, start_pos);
    }
    
    // Handle quoted strings
    if (c == '"' || c == '\'') {
        char quote = c;
        tokenizer->position++; // Skip opening quote
        tokenizer->column++;
        size_t start = tokenizer->position;
        
        while (tokenizer->position < tokenizer->input_length) {
            char current = tokenizer->input[tokenizer->position];
            if (current == quote) {
                break;
            }
            // Handle escape sequences in double quotes
            if (current == '\\' && quote == '"' && 
                tokenizer->position + 1 < tokenizer->input_length) {
                tokenizer->position++; // Skip escape
                tokenizer->column++;
                if (tokenizer->position < tokenizer->input_length) {
                    current = tokenizer->input[tokenizer->position];
                    if (current == '\n') {
                        tokenizer->line++;
                        tokenizer->column = 1;
                    } else {
                        tokenizer->column++;
                    }
                    tokenizer->position++;
                }
            } else {
                if (current == '\n') {
                    tokenizer->line++;
                    tokenizer->column = 1;
                } else {
                    tokenizer->column++;
                }
                tokenizer->position++;
            }
        }
        
        size_t length = tokenizer->position - start;
        if (tokenizer->position < tokenizer->input_length) {
            tokenizer->position++; // Skip closing quote
            tokenizer->column++;
        }
        
        // Double quotes create expandable strings, single quotes create literal strings
        modern_token_type_t token_type = (quote == '"') ? MODERN_TOK_EXPANDABLE_STRING : MODERN_TOK_STRING;
        return token_new(token_type, &tokenizer->input[start], length,
                        start_line, start_column, start_pos);
    }
    
    // Handle variable references and command substitution
    if (c == '$') {
        size_t start = tokenizer->position;
        tokenizer->position++;
        tokenizer->column++;
        
        // Handle $((...)) arithmetic expansion
        if (tokenizer->position + 1 < tokenizer->input_length && 
            tokenizer->input[tokenizer->position] == '(' &&
            tokenizer->input[tokenizer->position + 1] == '(') {
            tokenizer->position += 2;
            tokenizer->column += 2;
            int paren_count = 2;
            while (tokenizer->position < tokenizer->input_length && paren_count > 0) {
                if (tokenizer->input[tokenizer->position] == '(') {
                    paren_count++;
                } else if (tokenizer->input[tokenizer->position] == ')') {
                    paren_count--;
                }
                tokenizer->position++;
                tokenizer->column++;
            }
            size_t length = tokenizer->position - start;
            return token_new(MODERN_TOK_ARITH_EXP, &tokenizer->input[start], length,
                            start_line, start_column, start_pos);
        }
        
        // Handle $(...) command substitution
        if (tokenizer->position < tokenizer->input_length && 
            tokenizer->input[tokenizer->position] == '(') {
            tokenizer->position++;
            tokenizer->column++;
            int paren_count = 1;
            while (tokenizer->position < tokenizer->input_length && paren_count > 0) {
                if (tokenizer->input[tokenizer->position] == '(') {
                    paren_count++;
                } else if (tokenizer->input[tokenizer->position] == ')') {
                    paren_count--;
                }
                tokenizer->position++;
                tokenizer->column++;
            }
            size_t length = tokenizer->position - start;
            return token_new(MODERN_TOK_COMMAND_SUB, &tokenizer->input[start], length,
                            start_line, start_column, start_pos);
        }
        
        // Handle ${var} format
        if (tokenizer->position < tokenizer->input_length && 
            tokenizer->input[tokenizer->position] == '{') {
            tokenizer->position++;
            tokenizer->column++;
            while (tokenizer->position < tokenizer->input_length &&
                   tokenizer->input[tokenizer->position] != '}') {
                tokenizer->position++;
                tokenizer->column++;
            }
            if (tokenizer->position < tokenizer->input_length) {
                tokenizer->position++; // Skip closing }
                tokenizer->column++;
            }
        } else {
            // Handle $var format
            while (tokenizer->position < tokenizer->input_length &&
                   (isalnum(tokenizer->input[tokenizer->position]) || 
                    tokenizer->input[tokenizer->position] == '_')) {
                tokenizer->position++;
                tokenizer->column++;
            }
        }
        
        size_t length = tokenizer->position - start;
        return token_new(MODERN_TOK_VARIABLE, &tokenizer->input[start], length,
                        start_line, start_column, start_pos);
    }
    
    // Handle multi-character operators
    if (c == '&' && tokenizer->position + 1 < tokenizer->input_length && 
        tokenizer->input[tokenizer->position + 1] == '&') {
        tokenizer->position += 2;
        tokenizer->column += 2;
        return token_new(MODERN_TOK_LOGICAL_AND, "&&", 2, start_line, start_column, start_pos);
    }
    
    if (c == '|' && tokenizer->position + 1 < tokenizer->input_length && 
        tokenizer->input[tokenizer->position + 1] == '|') {
        tokenizer->position += 2;
        tokenizer->column += 2;
        return token_new(MODERN_TOK_LOGICAL_OR, "||", 2, start_line, start_column, start_pos);
    }
    
    if (c == '>' && tokenizer->position + 1 < tokenizer->input_length && 
        tokenizer->input[tokenizer->position + 1] == '>') {
        tokenizer->position += 2;
        tokenizer->column += 2;
        return token_new(MODERN_TOK_APPEND, ">>", 2, start_line, start_column, start_pos);
    }
    
    if (c == '<' && tokenizer->position + 1 < tokenizer->input_length && 
        tokenizer->input[tokenizer->position + 1] == '<') {
        // Check for <<< (here string)
        if (tokenizer->position + 2 < tokenizer->input_length &&
            tokenizer->input[tokenizer->position + 2] == '<') {
            tokenizer->position += 3;
            tokenizer->column += 3;
            return token_new(MODERN_TOK_HERESTRING, "<<<", 3, start_line, start_column, start_pos);
        }
        // Check for <<- (here document with tab stripping)
        else if (tokenizer->position + 2 < tokenizer->input_length &&
                 tokenizer->input[tokenizer->position + 2] == '-') {
            tokenizer->position += 3;
            tokenizer->column += 3;
            return token_new(MODERN_TOK_HEREDOC_STRIP, "<<-", 3, start_line, start_column, start_pos);
        }
        // Regular << (here document)
        else {
            tokenizer->position += 2;
            tokenizer->column += 2;
            return token_new(MODERN_TOK_HEREDOC, "<<", 2, start_line, start_column, start_pos);
        }
    }
    
    // Handle &> (redirect both stdout and stderr)
    if (c == '&' && tokenizer->position + 1 < tokenizer->input_length && 
        tokenizer->input[tokenizer->position + 1] == '>') {
        tokenizer->position += 2;
        tokenizer->column += 2;
        return token_new(MODERN_TOK_REDIRECT_BOTH, "&>", 2, start_line, start_column, start_pos);
    }
    
    // Handle numeric file descriptor redirections (2>, 2>>, etc.)
    if (isdigit(c)) {
        size_t num_start = tokenizer->position;
        // Read the number
        while (tokenizer->position < tokenizer->input_length && 
               isdigit(tokenizer->input[tokenizer->position])) {
            tokenizer->position++;
            tokenizer->column++;
        }
        
        // Check if followed by > or >>
        if (tokenizer->position < tokenizer->input_length) {
            if (tokenizer->input[tokenizer->position] == '>') {
                if (tokenizer->position + 1 < tokenizer->input_length &&
                    tokenizer->input[tokenizer->position + 1] == '>') {
                    // Handle N>> (append to file descriptor N)
                    tokenizer->position += 2;
                    tokenizer->column += 2;
                    size_t length = tokenizer->position - num_start;
                    return token_new(MODERN_TOK_APPEND_ERR, &tokenizer->input[num_start], length,
                                   start_line, start_column, start_pos);
                } else {
                    // Handle N> (redirect file descriptor N)
                    tokenizer->position++;
                    tokenizer->column++;
                    size_t length = tokenizer->position - num_start;
                    return token_new(MODERN_TOK_REDIRECT_ERR, &tokenizer->input[num_start], length,
                                   start_line, start_column, start_pos);
                }
            }
        }
        
        // Not a redirection, reset position and treat as regular number
        tokenizer->position = num_start;
        tokenizer->column = start_column;
        goto handle_word;
    }
    
    // Handle context-sensitive operators first
    if (c == '-') {
        // Check if this looks like a flag (-word) or standalone minus
        if (tokenizer->position + 1 < tokenizer->input_length &&
            isalpha(tokenizer->input[tokenizer->position + 1])) {
            // This looks like a flag, treat as part of word
            goto handle_word;
        } else {
            // Standalone minus operator
            tokenizer->position++;
            tokenizer->column++;
            return token_new(MODERN_TOK_MINUS, "-", 1, start_line, start_column, start_pos);
        }
    }
    
    if (c == '/') {
        // Always treat / as part of a word (path)
        goto handle_word;
    }
    
    // Handle backtick command substitution
    if (c == '`') {
        size_t start = tokenizer->position;
        tokenizer->position++; // Skip opening backtick
        tokenizer->column++;
        
        // Find matching closing backtick
        while (tokenizer->position < tokenizer->input_length && 
               tokenizer->input[tokenizer->position] != '`') {
            if (tokenizer->input[tokenizer->position] == '\n') {
                tokenizer->line++;
                tokenizer->column = 1;
            } else {
                tokenizer->column++;
            }
            tokenizer->position++;
        }
        
        if (tokenizer->position < tokenizer->input_length) {
            tokenizer->position++; // Skip closing backtick
            tokenizer->column++;
        }
        
        size_t length = tokenizer->position - start;
        return token_new(MODERN_TOK_COMMAND_SUB, &tokenizer->input[start], length,
                        start_line, start_column, start_pos);
    }
    
    // Handle single-character operators
    modern_token_type_t single_char_type;
    switch (c) {
        case ';': single_char_type = MODERN_TOK_SEMICOLON; break;
        case '|': single_char_type = MODERN_TOK_PIPE; break;
        case '&': single_char_type = MODERN_TOK_AND; break;
        case '<': single_char_type = MODERN_TOK_REDIRECT_IN; break;
        case '>': single_char_type = MODERN_TOK_REDIRECT_OUT; break;
        case '(': single_char_type = MODERN_TOK_LPAREN; break;
        case ')': single_char_type = MODERN_TOK_RPAREN; break;
        case '{': single_char_type = MODERN_TOK_LBRACE; break;
        case '}': single_char_type = MODERN_TOK_RBRACE; break;
        case '[': single_char_type = MODERN_TOK_LBRACKET; break;
        case ']': single_char_type = MODERN_TOK_RBRACKET; break;
        case '=': single_char_type = MODERN_TOK_ASSIGN; break;
        case '+': single_char_type = MODERN_TOK_PLUS; break;
        case '*': single_char_type = MODERN_TOK_MULTIPLY; break;
        // case '/': single_char_type = MODERN_TOK_DIVIDE; break;  // Handled above
        case '%': single_char_type = MODERN_TOK_MODULO; break;
        case '?': single_char_type = MODERN_TOK_QUESTION; break;
        default: single_char_type = MODERN_TOK_ERROR; break;
    }
    
    if (single_char_type != MODERN_TOK_ERROR) {
        tokenizer->position++;
        tokenizer->column++;
        char token_text[2] = {c, '\0'};
        return token_new(single_char_type, token_text, 1, start_line, start_column, start_pos);
    }
    
    // Handle words and numbers  
    handle_word:
    if (isalnum(c) || c == '_' || c == '-' || c == '.' || c == '/' || c == '~') {
        size_t start = tokenizer->position;
        bool is_number = isdigit(c);
        
        while (tokenizer->position < tokenizer->input_length && 
               is_word_char(tokenizer->input[tokenizer->position])) {
            if (!isdigit(tokenizer->input[tokenizer->position])) {
                is_number = false;
            }
            tokenizer->position++;
            tokenizer->column++;
        }
        
        size_t length = tokenizer->position - start;
        modern_token_type_t type = is_number ? MODERN_TOK_NUMBER : 
                           classify_word(&tokenizer->input[start], length, tokenizer->enable_keywords);
        
        return token_new(type, &tokenizer->input[start], length,
                        start_line, start_column, start_pos);
    }
    
    // Unknown character
    tokenizer->position++;
    tokenizer->column++;
    char token_text[2] = {c, '\0'};
    return token_new(MODERN_TOK_ERROR, token_text, 1, start_line, start_column, start_pos);
}

// Utility functions
const char *modern_token_type_name(modern_token_type_t type) {
    switch (type) {
        case MODERN_TOK_EOF: return "EOF";
        case MODERN_TOK_WORD: return "WORD";
        case MODERN_TOK_STRING: return "STRING";
        case MODERN_TOK_NUMBER: return "NUMBER";
        case MODERN_TOK_VARIABLE: return "VARIABLE";
        case MODERN_TOK_SEMICOLON: return "SEMICOLON";
        case MODERN_TOK_PIPE: return "PIPE";
        case MODERN_TOK_AND: return "AND";
        case MODERN_TOK_LOGICAL_AND: return "LOGICAL_AND";
        case MODERN_TOK_LOGICAL_OR: return "LOGICAL_OR";
        case MODERN_TOK_REDIRECT_IN: return "REDIRECT_IN";
        case MODERN_TOK_REDIRECT_OUT: return "REDIRECT_OUT";
        case MODERN_TOK_APPEND: return "APPEND";
        case MODERN_TOK_HEREDOC: return "HEREDOC";
        case MODERN_TOK_HEREDOC_STRIP: return "HEREDOC_STRIP";
        case MODERN_TOK_HERESTRING: return "HERESTRING";
        case MODERN_TOK_REDIRECT_ERR: return "REDIRECT_ERR";
        case MODERN_TOK_REDIRECT_BOTH: return "REDIRECT_BOTH";
        case MODERN_TOK_APPEND_ERR: return "APPEND_ERR";
        case MODERN_TOK_REDIRECT_FD: return "REDIRECT_FD";
        case MODERN_TOK_ASSIGN: return "ASSIGN";
        case MODERN_TOK_PLUS: return "PLUS";
        case MODERN_TOK_MINUS: return "MINUS";
        case MODERN_TOK_MULTIPLY: return "MULTIPLY";
        case MODERN_TOK_DIVIDE: return "DIVIDE";
        case MODERN_TOK_MODULO: return "MODULO";
        case MODERN_TOK_GLOB: return "GLOB";
        case MODERN_TOK_QUESTION: return "QUESTION";
        case MODERN_TOK_COMMAND_SUB: return "COMMAND_SUB";
        case MODERN_TOK_ARITH_EXP: return "ARITH_EXP";
        case MODERN_TOK_BACKQUOTE: return "BACKQUOTE";
        case MODERN_TOK_LPAREN: return "LPAREN";
        case MODERN_TOK_RPAREN: return "RPAREN";
        case MODERN_TOK_LBRACE: return "LBRACE";
        case MODERN_TOK_RBRACE: return "RBRACE";
        case MODERN_TOK_LBRACKET: return "LBRACKET";
        case MODERN_TOK_RBRACKET: return "RBRACKET";
        case MODERN_TOK_IF: return "IF";
        case MODERN_TOK_THEN: return "THEN";
        case MODERN_TOK_ELSE: return "ELSE";
        case MODERN_TOK_ELIF: return "ELIF";
        case MODERN_TOK_FI: return "FI";
        case MODERN_TOK_WHILE: return "WHILE";
        case MODERN_TOK_DO: return "DO";
        case MODERN_TOK_DONE: return "DONE";
        case MODERN_TOK_FOR: return "FOR";
        case MODERN_TOK_IN: return "IN";
        case MODERN_TOK_CASE: return "CASE";
        case MODERN_TOK_ESAC: return "ESAC";
        case MODERN_TOK_UNTIL: return "UNTIL";
        case MODERN_TOK_FUNCTION: return "FUNCTION";
        case MODERN_TOK_NEWLINE: return "NEWLINE";
        case MODERN_TOK_WHITESPACE: return "WHITESPACE";
        case MODERN_TOK_COMMENT: return "COMMENT";
        case MODERN_TOK_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool modern_token_is_keyword(modern_token_type_t type) {
    return type >= MODERN_TOK_IF && type <= MODERN_TOK_FUNCTION;
}

bool modern_token_is_operator(modern_token_type_t type) {
    return type >= MODERN_TOK_SEMICOLON && type <= MODERN_TOK_RBRACKET;
}

bool modern_token_is_word_like(modern_token_type_t type) {
    return type == MODERN_TOK_WORD || type == MODERN_TOK_STRING || 
           type == MODERN_TOK_EXPANDABLE_STRING || type == MODERN_TOK_NUMBER;
}
