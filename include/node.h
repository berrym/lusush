#ifndef NODE_H
#define NODE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef enum {
    NODE_COMMAND,
    NODE_VAR,
    NODE_PIPE,
    NODE_REDIR_IN,     // '<'
    NODE_REDIR_OUT,    // '>'
    NODE_REDIR_APPEND, // '>>'
    NODE_REDIR_ERR,    // '2>'
    NODE_REDIR_ERR_APPEND, // '2>>'
    NODE_REDIR_HEREDOC,    // '<<'
    NODE_REDIR_HEREDOC_STRIP, // '<<-'
    NODE_REDIR_HERESTRING, // '<<<'
    NODE_REDIR_BOTH,       // '&>'
    NODE_REDIR_FD,         // '&1', '&2', etc.
    NODE_REDIR_CLOBBER,    // '>|'
    // List types for semantic clarity
    NODE_COMMAND_LIST, // Sequence of commands separated by semicolons
    NODE_PIPELINE,     // Sequence of commands connected by pipes
    // Control structures
    NODE_IF,           // if statement
    NODE_FOR,          // for loop
    NODE_WHILE,        // while loop
    NODE_UNTIL,        // until loop
    NODE_CASE,         // case statement
    NODE_FUNCTION,     // function definition
    NODE_BRACE_GROUP,  // brace group { commands; }
    NODE_SUBSHELL,     // subshell ( commands )
    // Logical operators
    NODE_LOGICAL_AND,  // && operator
    NODE_LOGICAL_OR,   // || operator
    // Job control
    NODE_BACKGROUND,   // & operator (background execution)
} node_type_t;

typedef enum {
    VAL_SINT = 1,
    VAL_UINT,
    VAL_SLLONG,
    VAL_ULLONG,
    VAL_FLOAT,
    VAL_LDOUBLE,
    VAL_CHR,
    VAL_STR,
} val_type_t;

typedef union {
    ssize_t sint;
    size_t uint;
    int64_t sllong;
    uint64_t ullong;
    double sfloat;
    long double ldouble;
    char ch;
    char *str;
} symval_t;

typedef struct node {
    node_type_t type;
    val_type_t val_type;
    symval_t val;
    size_t children;
    struct node *first_child;
    struct node *next_sibling, *prev_sibling;
} node_t;

node_t *new_node(node_type_t);
void add_child_node(node_t *, node_t *);
void free_node_tree(node_t *);
void set_node_val_str(node_t *, char *);

#endif
