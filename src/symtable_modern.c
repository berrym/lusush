/**
 * Modern POSIX Symbol Table Implementation
 * 
 * Clean, efficient symbol table with proper POSIX scoping semantics.
 */

#include "symtable_modern.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Default hash table size
#define DEFAULT_HASH_SIZE 64
#define MAX_SCOPE_DEPTH 256

// Simple hash function for variable names
static size_t hash_name(const char *name, size_t table_size) {
    size_t hash = 5381;
    while (*name) {
        hash = ((hash << 5) + hash) + *name++;
    }
    return hash % table_size;
}

// Create new symbol table manager
symtable_manager_t *symtable_manager_new(void) {
    symtable_manager_t *manager = calloc(1, sizeof(symtable_manager_t));
    if (!manager) {
        return NULL;
    }
    
    // Create global scope
    symtable_modern_t *global = calloc(1, sizeof(symtable_modern_t));
    if (!global) {
        free(manager);
        return NULL;
    }
    
    global->scope_type = SCOPE_GLOBAL;
    global->level = 0;
    global->hash_size = DEFAULT_HASH_SIZE;
    global->vars = calloc(DEFAULT_HASH_SIZE, sizeof(symvar_modern_t *));
    global->parent = NULL;
    global->scope_name = strdup("global");
    
    if (!global->vars || !global->scope_name) {
        free(global->vars);
        free(global->scope_name);
        free(global);
        free(manager);
        return NULL;
    }
    
    manager->current_scope = global;
    manager->global_scope = global;
    manager->max_scope_level = 0;
    manager->debug_mode = false;
    
    return manager;
}

// Free symbol table manager
void symtable_manager_free(symtable_manager_t *manager) {
    if (!manager) return;
    
    // Pop all scopes to free memory
    while (manager->current_scope && manager->current_scope != manager->global_scope) {
        symtable_pop_scope(manager);
    }
    
    // Free global scope
    if (manager->global_scope) {
        for (size_t i = 0; i < manager->global_scope->hash_size; i++) {
            symvar_modern_t *var = manager->global_scope->vars[i];
            while (var) {
                symvar_modern_t *next = var->next;
                free(var->name);
                free(var->value);
                free(var);
                var = next;
            }
        }
        free(manager->global_scope->vars);
        free(manager->global_scope->scope_name);
        free(manager->global_scope);
    }
    
    free(manager);
}

// Set debug mode
void symtable_manager_set_debug(symtable_manager_t *manager, bool debug) {
    if (manager) {
        manager->debug_mode = debug;
    }
}

// Push new scope
int symtable_push_scope(symtable_manager_t *manager, scope_type_t type, const char *name) {
    if (!manager || !name) return -1;
    
    if (manager->current_scope->level >= MAX_SCOPE_DEPTH) {
        if (manager->debug_mode) {
            fprintf(stderr, "ERROR: Maximum scope depth exceeded\n");
        }
        return -1;
    }
    
    symtable_modern_t *new_scope = calloc(1, sizeof(symtable_modern_t));
    if (!new_scope) return -1;
    
    new_scope->scope_type = type;
    new_scope->level = manager->current_scope->level + 1;
    new_scope->hash_size = DEFAULT_HASH_SIZE;
    new_scope->vars = calloc(DEFAULT_HASH_SIZE, sizeof(symvar_modern_t *));
    new_scope->parent = manager->current_scope;
    new_scope->scope_name = strdup(name);
    
    if (!new_scope->vars || !new_scope->scope_name) {
        free(new_scope->vars);
        free(new_scope->scope_name);
        free(new_scope);
        return -1;
    }
    
    manager->current_scope = new_scope;
    if (new_scope->level > manager->max_scope_level) {
        manager->max_scope_level = new_scope->level;
    }
    
    if (manager->debug_mode) {
        const char *type_names[] = {"GLOBAL", "FUNCTION", "LOOP", "SUBSHELL", "CONDITIONAL"};
        printf("DEBUG: Pushed %s scope '%s' (level %zu)\n", 
               type_names[type], name, new_scope->level);
    }
    
    return 0;
}

// Pop current scope
int symtable_pop_scope(symtable_manager_t *manager) {
    if (!manager || !manager->current_scope || manager->current_scope == manager->global_scope) {
        return -1; // Can't pop global scope
    }
    
    symtable_modern_t *old_scope = manager->current_scope;
    manager->current_scope = old_scope->parent;
    
    if (manager->debug_mode) {
        printf("DEBUG: Popped scope '%s' (level %zu)\n", 
               old_scope->scope_name, old_scope->level);
    }
    
    // Free all variables in the old scope
    for (size_t i = 0; i < old_scope->hash_size; i++) {
        symvar_modern_t *var = old_scope->vars[i];
        while (var) {
            symvar_modern_t *next = var->next;
            free(var->name);
            free(var->value);
            free(var);
            var = next;
        }
    }
    
    free(old_scope->vars);
    free(old_scope->scope_name);
    free(old_scope);
    
    return 0;
}

// Get current scope level
size_t symtable_current_level(symtable_manager_t *manager) {
    return manager ? manager->current_scope->level : 0;
}

// Get current scope name
const char *symtable_current_scope_name(symtable_manager_t *manager) {
    return manager ? manager->current_scope->scope_name : "unknown";
}

// Find variable in scope chain
static symvar_modern_t *find_var(symtable_modern_t *scope, const char *name) {
    while (scope) {
        size_t hash = hash_name(name, scope->hash_size);
        symvar_modern_t *var = scope->vars[hash];
        
        while (var) {
            if (strcmp(var->name, name) == 0) {
                return var;
            }
            var = var->next;
        }
        
        scope = scope->parent;
    }
    
    return NULL;
}

// Set variable in current scope
int symtable_set_var(symtable_manager_t *manager, const char *name, const char *value, symvar_flags_t flags) {
    if (!manager || !name) return -1;
    
    symtable_modern_t *scope = manager->current_scope;
    size_t hash = hash_name(name, scope->hash_size);
    
    // Check if variable already exists in current scope
    symvar_modern_t *var = scope->vars[hash];
    while (var) {
        if (strcmp(var->name, name) == 0) {
            // Update existing variable
            free(var->value);
            var->value = value ? strdup(value) : strdup("");
            var->flags = flags;
            
            if (manager->debug_mode) {
                printf("DEBUG: Updated variable '%s' = '%s' in scope '%s'\n", 
                       name, var->value, scope->scope_name);
            }
            return 0;
        }
        var = var->next;
    }
    
    // Create new variable
    var = calloc(1, sizeof(symvar_modern_t));
    if (!var) return -1;
    
    var->name = strdup(name);
    var->value = value ? strdup(value) : strdup("");
    var->type = SYMVAR_STRING;
    var->flags = flags;
    var->scope_level = scope->level;
    var->next = scope->vars[hash];
    
    if (!var->name || !var->value) {
        free(var->name);
        free(var->value);
        free(var);
        return -1;
    }
    
    scope->vars[hash] = var;
    
    if (manager->debug_mode) {
        printf("DEBUG: Created variable '%s' = '%s' in scope '%s'\n", 
               name, value ? value : "", scope->scope_name);
    }
    
    return 0;
}

// Set local variable (current scope only)
int symtable_set_local_var(symtable_manager_t *manager, const char *name, const char *value) {
    return symtable_set_var(manager, name, value, SYMVAR_LOCAL);
}

// Set global variable (force global scope)
int symtable_set_global_var(symtable_manager_t *manager, const char *name, const char *value) {
    if (!manager || !name) return -1;
    
    symtable_modern_t *old_scope = manager->current_scope;
    manager->current_scope = manager->global_scope;
    int result = symtable_set_var(manager, name, value, SYMVAR_NONE);
    manager->current_scope = old_scope;
    
    return result;
}

// Get variable value
char *symtable_get_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) return NULL;
    
    symvar_modern_t *var = find_var(manager->current_scope, name);
    if (!var || (var->flags & SYMVAR_UNSET)) {
        return NULL;
    }
    
    if (manager->debug_mode) {
        printf("DEBUG: Retrieved variable '%s' = '%s' from scope level %zu\n", 
               name, var->value, var->scope_level);
    }
    
    return strdup(var->value);
}

// Check if variable exists
bool symtable_var_exists(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) return false;
    
    symvar_modern_t *var = find_var(manager->current_scope, name);
    return var && !(var->flags & SYMVAR_UNSET);
}

// Unset variable
int symtable_unset_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) return -1;
    
    symvar_modern_t *var = find_var(manager->current_scope, name);
    if (var) {
        var->flags |= SYMVAR_UNSET;
        free(var->value);
        var->value = strdup("");
        
        if (manager->debug_mode) {
            printf("DEBUG: Unset variable '%s'\n", name);
        }
    }
    
    return 0;
}

// Export variable to environment
int symtable_export_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) return -1;
    
    symvar_modern_t *var = find_var(manager->current_scope, name);
    if (var) {
        var->flags |= SYMVAR_EXPORTED;
        if (manager->debug_mode) {
            printf("DEBUG: Exported variable '%s'\n", name);
        }
        return 0;
    }
    
    return -1;
}

// Dump current scope for debugging
void symtable_dump_scope(symtable_manager_t *manager, scope_type_t scope_type) {
    if (!manager) return;
    
    const char *type_names[] = {"GLOBAL", "FUNCTION", "LOOP", "SUBSHELL", "CONDITIONAL"};
    symtable_modern_t *scope = manager->current_scope;
    
    // Find the scope of the requested type
    while (scope && scope->scope_type != scope_type) {
        scope = scope->parent;
    }
    
    if (!scope) {
        printf("No scope of type %s found\n", type_names[scope_type]);
        return;
    }
    
    printf("=== %s SCOPE '%s' (level %zu) ===\n", 
           type_names[scope_type], scope->scope_name, scope->level);
    
    for (size_t i = 0; i < scope->hash_size; i++) {
        symvar_modern_t *var = scope->vars[i];
        while (var) {
            if (!(var->flags & SYMVAR_UNSET)) {
                printf("  %s = '%s'", var->name, var->value);
                if (var->flags & SYMVAR_EXPORTED) printf(" [exported]");
                if (var->flags & SYMVAR_LOCAL) printf(" [local]");
                if (var->flags & SYMVAR_READONLY) printf(" [readonly]");
                printf("\n");
            }
            var = var->next;
        }
    }
}

// Dump all scopes for debugging
void symtable_dump_all_scopes(symtable_manager_t *manager) {
    if (!manager) return;
    
    symtable_modern_t *scope = manager->current_scope;
    while (scope) {
        symtable_dump_scope(manager, scope->scope_type);
        scope = scope->parent;
    }
}
