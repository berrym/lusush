#ifndef NODE_NEW_H
#define NODE_NEW_H

#include "node.h"
#include "scanner.h"

/**
 * ENHANCED AST NODE TYPES FOR POSIX-COMPLIANT PARSER
 * 
 * This extends the existing node system with proper POSIX grammar support
 * while maintaining compatibility with the current execution engine.
 */

// ============================================================================
// NEW NODE TYPES FOR POSIX GRAMMAR
// ============================================================================

typedef enum {
    // Existing types (for compatibility)
    NODE_COMMAND_OLD = NODE_COMMAND,
    NODE_VAR_OLD = NODE_VAR,
    
    // New POSIX grammar node types
    NODE_COMPLETE_COMMAND = 100,    // Top-level command
    NODE_LIST,                      // Command list with operators
    NODE_AND_OR,                    // && || operators
    NODE_PIPELINE_NEW,              // Pipeline (distinct from old NODE_PIPE)
    NODE_PIPE_SEQUENCE,             // Sequence of piped commands
    
    // Enhanced command types
    NODE_SIMPLE_COMMAND_NEW,        // Simple command with full POSIX support
    NODE_COMPOUND_COMMAND,          // Container for control structures
    
    // Control structures (enhanced)
    NODE_IF_CLAUSE,                 // if statement
    NODE_WHILE_CLAUSE,              // while loop
    NODE_FOR_CLAUSE,                // for loop  
    NODE_CASE_CLAUSE,               // case statement
    NODE_UNTIL_CLAUSE,              // until loop
    NODE_FUNCTION_DEF,              // function definition
    
    // Command components
    NODE_WORD_NEW,                  // Word with expansion info
    NODE_ASSIGNMENT_NEW,            // Variable assignment
    NODE_REDIRECTION_NEW,           // I/O redirection
    
    // Utility nodes
    NODE_SEPARATOR,                 // ; & separators
    NODE_NEWLINE_LIST,              // Newline sequences
    NODE_COMPOUND_LIST,             // Compound command list
    NODE_TERM,                      // Term in compound list
    
    // Case statement components
    NODE_CASE_ITEM,                 // pattern) commands ;;
    NODE_CASE_PATTERN,              // Case pattern
    
    // For loop components  
    NODE_WORDLIST,                  // Word list for for loops
    
    // Function components
    NODE_FUNCTION_BODY,             // Function body
    
    // Error recovery
    NODE_ERROR,                     // Error placeholder
} node_type_new_t;

// ============================================================================
// ENHANCED NODE DATA STRUCTURES
// ============================================================================

/**
 * Assignment data for variable assignments
 */
typedef struct assignment {
    char *name;                     // Variable name
    char *value;                    // Variable value
    bool is_array;                  // Array assignment
    bool is_export;                 // Export flag
    struct assignment *next;        // Next assignment
} assignment_t;

/**
 * Redirection data for I/O redirection
 */
typedef struct redirection {
    int fd_from;                    // Source file descriptor
    int fd_to;                      // Target file descriptor
    char *filename;                 // Target filename
    token_type_t type;              // Redirection type
    bool is_heredoc;                // Here document flag
    char *heredoc_content;          // Here document content
    struct redirection *next;       // Next redirection
} redirection_t;

/**
 * Word data with expansion information
 */
typedef struct word_data {
    char *text;                     // Original text
    char *expanded;                 // Expanded text (if different)
    bool quoted;                    // Was quoted
    bool needs_expansion;           // Needs parameter/command expansion
    bool is_pattern;                // Contains glob patterns
} word_data_t;

/**
 * Enhanced node structure for new AST
 */
typedef struct node_new {
    node_type_new_t type;           // Node type
    
    union {
        // Simple command data
        struct {
            char **argv;            // Command arguments
            int argc;               // Argument count
            assignment_t *assignments; // Variable assignments
            redirection_t *redirections; // I/O redirections
            bool is_builtin;        // Is builtin command
        } simple_command;
        
        // Compound command data
        struct {
            node_type_new_t compound_type; // Specific type (if/while/for/case)
            struct node_new *condition;    // Condition expression
            struct node_new *body;         // Command body
            struct node_new *else_part;    // else clause (if applicable)
            struct node_new *wordlist;     // for loop wordlist
            char *variable;         // for loop variable
        } compound_command;
        
        // Pipeline data
        struct {
            struct node_new **commands; // Array of commands
            int command_count;      // Number of commands
            bool background;        // Background execution (&)
        } pipeline;
        
        // List data (command lists with operators)
        struct {
            struct node_new *left;  // Left operand
            struct node_new *right; // Right operand
            token_type_t operator;  // && || ; &
        } list;
        
        // Word data
        word_data_t word;
        
        // Assignment data
        assignment_t assignment;
        
        // Redirection data
        redirection_t redirection;
        
        // Case statement data
        struct {
            char *word;             // Case word
            struct node_new *patterns; // Pattern list
        } case_clause;
        
        // Case item data
        struct {
            char *pattern;          // Pattern string
            struct node_new *commands; // Commands to execute
        } case_item;
        
        // Function definition data
        struct {
            char *name;             // Function name
            struct node_new *body;  // Function body
        } function_def;
        
        // Error data
        struct {
            char *message;          // Error message
            int position;           // Error position
        } error;
    } data;
    
    struct node_new *next;          // Sibling nodes
    struct node_new *children;      // Child nodes
    
    // Source location for error reporting
    struct {
        int line;                   // Line number
        int column;                 // Column number
        int position;               // Absolute position
        const char *filename;       // Source filename
    } location;
    
    // Memory management
    bool owns_data;                 // Whether node owns its data
} node_new_t;

// ============================================================================
// NODE CREATION AND MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * Create new node with specified type
 */
node_new_t *node_new_create(node_type_new_t type);

/**
 * Create simple command node
 */
node_new_t *node_new_create_simple_command(char **argv, int argc);

/**
 * Create compound command node
 */
node_new_t *node_new_create_compound(node_type_new_t type, node_new_t *condition, node_new_t *body);

/**
 * Create pipeline node
 */
node_new_t *node_new_create_pipeline(node_new_t **commands, int count);

/**
 * Create list node (with operators)
 */
node_new_t *node_new_create_list(node_new_t *left, node_new_t *right, token_type_t operator);

/**
 * Create word node
 */
node_new_t *node_new_create_word(const char *text, bool quoted);

/**
 * Create assignment node
 */
node_new_t *node_new_create_assignment(const char *name, const char *value);

/**
 * Create redirection node
 */
node_new_t *node_new_create_redirection(token_type_t type, const char *filename);

/**
 * Add child node
 */
void node_new_add_child(node_new_t *parent, node_new_t *child);

/**
 * Add sibling node
 */
void node_new_add_sibling(node_new_t *node, node_new_t *sibling);

/**
 * Free node and all children
 */
void node_new_free(node_new_t *node);

/**
 * Free node tree recursively
 */
void node_new_free_tree(node_new_t *root);

// ============================================================================
// COMPATIBILITY FUNCTIONS
// ============================================================================

/**
 * Convert new AST to old AST format (for execution engine compatibility)
 */
struct node *node_new_to_old(node_new_t *new_node);

/**
 * Convert old AST to new AST format
 */
node_new_t *node_old_to_new(struct node *old_node);

// ============================================================================
// DEBUGGING AND UTILITY FUNCTIONS
// ============================================================================

/**
 * Print AST in readable format (for debugging)
 */
void node_new_print_tree(node_new_t *root, int indent);

/**
 * Validate AST structure
 */
bool node_new_validate(node_new_t *root);

/**
 * Get node type name as string
 */
const char *node_new_type_name(node_type_new_t type);

#endif /* NODE_NEW_H */
