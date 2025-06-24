/*
 * Legacy Symbol Table Interface with Modern Implementation
 * 
 * This file provides the legacy symtable interface that existing code expects,
 * but implements it using the modern symtable_modern system underneath.
 * This allows legacy code to work unchanged while using modern infrastructure.
 */

#include "../include/symtable.h"
#include "../include/symtable_modern.h"
#include "../include/errors.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <strings.h>
#include <unistd.h>

// Global modern symtable manager (replaces legacy global symtable)
static symtable_manager_t *global_manager = NULL;

// Legacy compatibility structures
static symtable_t dummy_symtable = {0, NULL, NULL};

// Initialize the symbol table system
void init_symtable(void) {
    if (!global_manager) {
        global_manager = symtable_manager_new();
        if (!global_manager) {
            error_abort("Failed to initialize symbol table");
        }
    }
}

// Free the global symbol table
void free_global_symtable(void) {
    if (global_manager) {
        symtable_manager_free(global_manager);
        global_manager = NULL;
    }
}

// Add entry to symbol table (legacy interface)
symtable_entry_t *add_to_symtable(char *name) {
    if (!global_manager || !name) {
        return NULL;
    }
    
    // Set variable with empty value if it doesn't exist
    if (!symtable_var_exists(global_manager, name)) {
        symtable_set_var(global_manager, name, "", SYMVAR_NONE);
    }
    
    // Return a dummy pointer for compatibility
    // Legacy code just checks for NULL/non-NULL
    return (symtable_entry_t*)1;
}

// Get symbol table entry (legacy interface)
symtable_entry_t *get_symtable_entry(const char *name) {
    if (!global_manager || !name) {
        return NULL;
    }
    
    if (symtable_var_exists(global_manager, name)) {
        return (symtable_entry_t*)1;  // Dummy pointer for compatibility
    }
    
    return NULL;
}

// Set value of symbol table entry (legacy interface)
void symtable_entry_setval(symtable_entry_t *entry, char *val) {
    // This function is problematic in the legacy interface because it doesn't
    // include the variable name. For compatibility, we ignore it.
    // Legacy code should use set_shell_varp instead.
    (void)entry;
    (void)val;
}

// Lookup symbol in symbol table
symtable_entry_t *lookup_symbol(symtable_t *symtable, const char *name) {
    (void)symtable;  // Ignore legacy symtable parameter
    
    if (!global_manager || !name) {
        return NULL;
    }
    
    if (symtable_var_exists(global_manager, name)) {
        return (symtable_entry_t*)1;  // Dummy pointer for compatibility
    }
    
    return NULL;
}

// Get global symbol table (legacy interface)
symtable_t *get_global_symtable(void) {
    return &dummy_symtable;  // Return dummy for compatibility
}

// Get local symbol table (legacy interface) 
symtable_t *get_local_symtable(void) {
    return &dummy_symtable;  // Return dummy for compatibility
}

// Get symbol table stack (legacy interface)
symtable_stack_t *get_symtable_stack(void) {
    static symtable_stack_t dummy_stack = {1, {&dummy_symtable}, &dummy_symtable, &dummy_symtable};
    return &dummy_stack;
}

// Remove entry from symbol table
int remove_from_symtable(symtable_t *symtable, symtable_entry_t *entry) {
    (void)symtable;
    (void)entry;
    // Legacy interface doesn't provide variable name, so we can't implement this properly
    return 0;
}

// Push new symbol table scope (legacy interface)
symtable_t *symtable_stack_push(void) {
    if (global_manager) {
        symtable_push_scope(global_manager, SCOPE_FUNCTION, "legacy-scope");
    }
    return &dummy_symtable;
}

// Pop symbol table scope (legacy interface)
symtable_t *symtable_stack_pop(void) {
    if (global_manager) {
        symtable_pop_scope(global_manager);
    }
    return &dummy_symtable;
}



// Free symbol table (legacy interface)
void free_symtable(symtable_t *symtable) {
    (void)symtable;  // Don't free the global manager
}

// Create new symbol table (legacy interface)
symtable_t *new_symtable(size_t level) {
    (void)level;  // Ignore level parameter
    return &dummy_symtable;  // Return dummy for compatibility
}

// Legacy variable access functions that actually work

// Get environment array for exec functions
char **get_environ_array(void) {
    // For now, return system environ until symtable_get_environ is implemented
    extern char **environ;
    return environ;
}

// Free environment array
void free_environ_array(char **env) {
    // Don't free system environ
    (void)env;
}

// Get the global modern symtable manager (for modern code)
symtable_manager_t *get_global_symtable_manager(void) {
    return global_manager;
}

// Set exit status special variable
void set_exit_status(int status) {
    if (global_manager) {
        char status_str[16];
        snprintf(status_str, sizeof(status_str), "%d", status);
        symtable_set_var(global_manager, "?", status_str, SYMVAR_NONE);
    }
}