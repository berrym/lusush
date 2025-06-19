/**
 * LUSUSH NEW AST NODE IMPLEMENTATION
 * 
 * This file implements the enhanced AST node system for the POSIX-compliant parser.
 */

#include "../include/node_new.h"
#include "../include/node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ============================================================================
// NODE CREATION AND MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * Create new node with specified type
 */
node_new_t *node_new_create(node_type_new_t type) {
    node_new_t *node = calloc(1, sizeof(node_new_t));
    if (!node) {
        return NULL;
    }
    
    node->type = type;
    node->owns_data = true;
    
    return node;
}

/**
 * Create simple command node
 */
node_new_t *node_new_create_simple_command(char **argv, int argc) {
    node_new_t *node = node_new_create(NODE_SIMPLE_COMMAND_NEW);
    if (!node) {
        return NULL;
    }
    
    if (argc > 0 && argv) {
        node->data.simple_command.argv = malloc(argc * sizeof(char*));
        if (node->data.simple_command.argv) {
            for (int i = 0; i < argc; i++) {
                node->data.simple_command.argv[i] = argv[i] ? strdup(argv[i]) : NULL;
            }
            node->data.simple_command.argc = argc;
        }
    }
    
    return node;
}

/**
 * Create compound command node
 */
node_new_t *node_new_create_compound(node_type_new_t type, node_new_t *condition, node_new_t *body) {
    node_new_t *node = node_new_create(NODE_COMPOUND_COMMAND);
    if (!node) {
        return NULL;
    }
    
    node->data.compound_command.compound_type = type;
    node->data.compound_command.condition = condition;
    node->data.compound_command.body = body;
    
    return node;
}

/**
 * Create pipeline node
 */
node_new_t *node_new_create_pipeline(node_new_t **commands, int count) {
    node_new_t *node = node_new_create(NODE_PIPELINE_NEW);
    if (!node) {
        return NULL;
    }
    
    if (count > 0 && commands) {
        node->data.pipeline.commands = malloc(count * sizeof(node_new_t*));
        if (node->data.pipeline.commands) {
            memcpy(node->data.pipeline.commands, commands, count * sizeof(node_new_t*));
            node->data.pipeline.command_count = count;
        }
    }
    
    return node;
}

/**
 * Create list node (with operators)
 */
node_new_t *node_new_create_list(node_new_t *left, node_new_t *right, token_type_t operator) {
    node_new_t *node = node_new_create(NODE_LIST);
    if (!node) {
        return NULL;
    }
    
    node->data.list.left = left;
    node->data.list.right = right;
    node->data.list.operator = operator;
    
    return node;
}

/**
 * Create word node
 */
node_new_t *node_new_create_word(const char *text, bool quoted) {
    node_new_t *node = node_new_create(NODE_WORD_NEW);
    if (!node) {
        return NULL;
    }
    
    if (text) {
        node->data.word.text = strdup(text);
        node->data.word.quoted = quoted;
        node->data.word.needs_expansion = !quoted; // Quoted words don't need expansion
    }
    
    return node;
}

/**
 * Create assignment node
 */
node_new_t *node_new_create_assignment(const char *name, const char *value) {
    node_new_t *node = node_new_create(NODE_ASSIGNMENT_NEW);
    if (!node) {
        return NULL;
    }
    
    if (name) {
        node->data.assignment.name = strdup(name);
    }
    if (value) {
        node->data.assignment.value = strdup(value);
    }
    
    return node;
}

/**
 * Create redirection node
 */
node_new_t *node_new_create_redirection(token_type_t type, const char *filename) {
    node_new_t *node = node_new_create(NODE_REDIRECTION_NEW);
    if (!node) {
        return NULL;
    }
    
    node->data.redirection.type = type;
    if (filename) {
        node->data.redirection.filename = strdup(filename);
    }
    
    return node;
}

/**
 * Add child node
 */
void node_new_add_child(node_new_t *parent, node_new_t *child) {
    if (!parent || !child) {
        return;
    }
    
    if (!parent->children) {
        parent->children = child;
    } else {
        // Add to end of children list
        node_new_t *current = parent->children;
        while (current->next) {
            current = current->next;
        }
        current->next = child;
    }
}

/**
 * Add sibling node
 */
void node_new_add_sibling(node_new_t *node, node_new_t *sibling) {
    if (!node || !sibling) {
        return;
    }
    
    // Find end of sibling chain
    while (node->next) {
        node = node->next;
    }
    
    node->next = sibling;
}

/**
 * Free node data based on type
 */
static void node_new_free_data(node_new_t *node) {
    if (!node || !node->owns_data) {
        return;
    }
    
    switch (node->type) {
        case NODE_SIMPLE_COMMAND_NEW:
            if (node->data.simple_command.argv) {
                for (int i = 0; i < node->data.simple_command.argc; i++) {
                    free(node->data.simple_command.argv[i]);
                }
                free(node->data.simple_command.argv);
            }
            // TODO: Free assignments and redirections
            break;
            
        case NODE_PIPELINE_NEW:
            free(node->data.pipeline.commands);
            break;
            
        case NODE_WORD_NEW:
            free(node->data.word.text);
            free(node->data.word.expanded);
            break;
            
        case NODE_ASSIGNMENT_NEW:
            free(node->data.assignment.name);
            free(node->data.assignment.value);
            break;
            
        case NODE_REDIRECTION_NEW:
            free(node->data.redirection.filename);
            free(node->data.redirection.heredoc_content);
            break;
            
        case NODE_CASE_CLAUSE:
            free(node->data.case_clause.word);
            break;
            
        case NODE_CASE_ITEM:
            free(node->data.case_item.pattern);
            break;
            
        case NODE_FUNCTION_DEF:
            free(node->data.function_def.name);
            break;
            
        case NODE_ERROR:
            free(node->data.error.message);
            break;
            
        default:
            // No special cleanup needed for other types
            break;
    }
}

/**
 * Free node and all children
 */
void node_new_free(node_new_t *node) {
    if (!node) {
        return;
    }
    
    node_new_free_data(node);
    free(node);
}

/**
 * Free node tree recursively
 */
void node_new_free_tree(node_new_t *root) {
    if (!root) {
        return;
    }
    
    // Free children first
    node_new_free_tree(root->children);
    
    // Free siblings
    node_new_free_tree(root->next);
    
    // Free this node
    node_new_free(root);
}

// ============================================================================
// COMPATIBILITY FUNCTIONS
// ============================================================================

/**
 * Convert new AST to old AST format (for execution engine compatibility)
 */
struct node *node_new_to_old(node_new_t *new_node) {
    if (!new_node) {
        return NULL;
    }
    
    // This is a complex conversion that would need to map
    // the new AST structure to the old one
    // For now, return NULL as placeholder
    
    // TODO: Implement full conversion logic
    return NULL;
}

/**
 * Convert old AST to new AST format
 */
node_new_t *node_old_to_new(struct node *old_node) {
    if (!old_node) {
        return NULL;
    }
    
    // TODO: Implement conversion from old format
    return NULL;
}

// ============================================================================
// DEBUGGING AND UTILITY FUNCTIONS
// ============================================================================

/**
 * Get node type name as string
 */
const char *node_new_type_name(node_type_new_t type) {
    switch (type) {
        case NODE_COMPLETE_COMMAND: return "COMPLETE_COMMAND";
        case NODE_LIST: return "LIST";
        case NODE_AND_OR: return "AND_OR";
        case NODE_PIPELINE_NEW: return "PIPELINE";
        case NODE_PIPE_SEQUENCE: return "PIPE_SEQUENCE";
        case NODE_SIMPLE_COMMAND_NEW: return "SIMPLE_COMMAND";
        case NODE_COMPOUND_COMMAND: return "COMPOUND_COMMAND";
        case NODE_IF_CLAUSE: return "IF_CLAUSE";
        case NODE_WHILE_CLAUSE: return "WHILE_CLAUSE";
        case NODE_FOR_CLAUSE: return "FOR_CLAUSE";
        case NODE_CASE_CLAUSE: return "CASE_CLAUSE";
        case NODE_UNTIL_CLAUSE: return "UNTIL_CLAUSE";
        case NODE_FUNCTION_DEF: return "FUNCTION_DEF";
        case NODE_WORD_NEW: return "WORD";
        case NODE_ASSIGNMENT_NEW: return "ASSIGNMENT";
        case NODE_REDIRECTION_NEW: return "REDIRECTION";
        case NODE_SEPARATOR: return "SEPARATOR";
        case NODE_NEWLINE_LIST: return "NEWLINE_LIST";
        case NODE_COMPOUND_LIST: return "COMPOUND_LIST";
        case NODE_TERM: return "TERM";
        case NODE_CASE_ITEM: return "CASE_ITEM";
        case NODE_CASE_PATTERN: return "CASE_PATTERN";
        case NODE_WORDLIST: return "WORDLIST";
        case NODE_FUNCTION_BODY: return "FUNCTION_BODY";
        case NODE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

/**
 * Print AST in readable format (for debugging)
 */
void node_new_print_tree(node_new_t *root, int indent) {
    if (!root) {
        return;
    }
    
    // Print indentation
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    printf("%s", node_new_type_name(root->type));
    
    // Print type-specific data
    switch (root->type) {
        case NODE_SIMPLE_COMMAND_NEW:
            printf(" (argc=%d)", root->data.simple_command.argc);
            if (root->data.simple_command.argc > 0 && root->data.simple_command.argv[0]) {
                printf(" cmd='%s'", root->data.simple_command.argv[0]);
            }
            break;
            
        case NODE_WORD_NEW:
            if (root->data.word.text) {
                printf(" text='%s'", root->data.word.text);
                if (root->data.word.quoted) {
                    printf(" (quoted)");
                }
            }
            break;
            
        case NODE_ASSIGNMENT_NEW:
            if (root->data.assignment.name) {
                printf(" %s=", root->data.assignment.name);
                if (root->data.assignment.value) {
                    printf("'%s'", root->data.assignment.value);
                }
            }
            break;
            
        case NODE_LIST:
            printf(" op=%d", root->data.list.operator);
            break;
            
        default:
            break;
    }
    
    printf("\n");
    
    // Print children
    if (root->children) {
        node_new_print_tree(root->children, indent + 1);
    }
    
    // Print siblings
    if (root->next) {
        node_new_print_tree(root->next, indent);
    }
}

/**
 * Validate AST structure
 */
bool node_new_validate(node_new_t *root) {
    if (!root) {
        return true; // NULL is valid
    }
    
    // Basic validation - check for cycles, null pointers, etc.
    // TODO: Implement comprehensive validation
    
    return true;
}
