/*
 * Global Symbol Table Management for Lusush Shell
 * 
 * This module provides a global modern symtable that replaces the legacy
 * symtable system while maintaining the same interface expectations.
 * It bridges the gap between legacy code that expects global symtable
 * access and the modern symtable_manager system.
 */

#include "../include/symtable_modern.h"
#include "../include/errors.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global modern symtable manager instance
static symtable_manager_t *global_symtable_manager = NULL;

// Initialize global modern symtable system
int init_global_symtable(void) {
    if (global_symtable_manager) {
        // Already initialized
        return 0;
    }
    
    global_symtable_manager = symtable_manager_new();
    if (!global_symtable_manager) {
        return -1;
    }
    
    return 0;
}

// Get the global symtable manager
symtable_manager_t *get_global_symtable_manager(void) {
    return global_symtable_manager;
}

// Set a variable in the global symtable
int set_global_var(const char *name, const char *value) {
    if (!global_symtable_manager || !name) {
        return -1;
    }
    
    return symtable_set_var(global_symtable_manager, name, value, SYMVAR_NONE);
}

// Get a variable from the global symtable
char *get_global_var(const char *name) {
    if (!global_symtable_manager || !name) {
        return NULL;
    }
    
    return symtable_get_var(global_symtable_manager, name);
}

// Set an exported variable in the global symtable
int export_global_var(const char *name, const char *value) {
    if (!global_symtable_manager || !name) {
        return -1;
    }
    
    return symtable_set_var(global_symtable_manager, name, value, SYMVAR_EXPORTED);
}

// Check if a variable exists in the global symtable
bool global_var_exists(const char *name) {
    if (!global_symtable_manager || !name) {
        return false;
    }
    
    return symtable_var_exists(global_symtable_manager, name);
}

// Unset a variable from the global symtable
int unset_global_var(const char *name) {
    if (!global_symtable_manager || !name) {
        return -1;
    }
    
    return symtable_unset_var(global_symtable_manager, name);
}

// Get environment array for execve
char **get_global_environ(void) {
    if (!global_symtable_manager) {
        return NULL;
    }
    
    return symtable_get_environ(global_symtable_manager);
}

// Free environment array
void free_global_environ(char **environ) {
    if (environ) {
        symtable_free_environ(environ);
    }
}

// Cleanup global symtable system
void free_global_symtable(void) {
    if (global_symtable_manager) {
        symtable_manager_free(global_symtable_manager);
        global_symtable_manager = NULL;
    }
}

// Legacy compatibility functions for existing code

// Legacy init_symtable replacement
void init_symtable(void) {
    init_global_symtable();
}

// Legacy add_to_symtable replacement - simplified interface
void *add_to_symtable(const char *name) {
    if (!name || !global_symtable_manager) {
        return NULL;
    }
    
    // If variable doesn't exist, create it with empty value
    if (!symtable_var_exists(global_symtable_manager, name)) {
        symtable_set_var(global_symtable_manager, name, "", SYMVAR_NONE);
    }
    
    // Return a dummy pointer to indicate success (legacy code expects non-NULL)
    return (void*)1;
}

// Legacy symtable_entry_setval replacement
void symtable_entry_setval(void *entry, const char *value) {
    // This is a simplified compatibility function
    // In practice, legacy code should be updated to use direct variable setting
    (void)entry;  // Suppress unused parameter warning
    (void)value;  // This function is deprecated
}

// Legacy get_symtable_entry replacement
void *get_symtable_entry(const char *name) {
    if (!name || !global_symtable_manager) {
        return NULL;
    }
    
    if (symtable_var_exists(global_symtable_manager, name)) {
        return (void*)1;  // Return dummy pointer for compatibility
    }
    
    return NULL;
}

// Get shell variable with default value (common pattern in legacy code)
char *get_shell_varp(char *name, char *default_value) {
    char *value = get_global_var(name);
    if (value) {
        return value;
    }
    return default_value;
}

// Get shell variable as integer with default
int get_shell_vari(char *name, int default_value) {
    char *value = get_global_var(name);
    if (value) {
        return atoi(value);
    }
    return default_value;
}

// Get shell variable as boolean with default
bool get_shell_varb(const char *name, bool default_value) {
    char *value = get_global_var(name);
    if (value) {
        return (strcmp(value, "1") == 0 || 
                strcmp(value, "true") == 0 || 
                strcmp(value, "yes") == 0);
    }
    return default_value;
}

// Set special shell variables (like $?, $!, $$)
int set_special_var(const char *name, const char *value) {
    if (!global_symtable_manager || !name) {
        return -1;
    }
    
    // Special variables are typically read-only or system-managed
    return symtable_set_var(global_symtable_manager, name, value, SYMVAR_NONE);
}

// Update last exit status ($?)
void set_exit_status(int status) {
    char status_str[16];
    snprintf(status_str, sizeof(status_str), "%d", status);
    set_special_var("?", status_str);
}

// Debug function to dump global symtable
void dump_global_symtable(void) {
    if (global_symtable_manager) {
        symtable_dump_all_scopes(global_symtable_manager);
    }
}