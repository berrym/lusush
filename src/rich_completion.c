/*
 * Lusush - A modern shell with rich completion system
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Rich Completion System - Fish-inspired completions with descriptions
 * 
 * This module enhances the existing completion system with:
 * - Rich descriptions for completions (like Fish shell)
 * - Categorized completions (commands, files, variables, etc.)
 * - Multi-column display with descriptions
 * - Context-aware completion metadata
 * - Performance-optimized with intelligent caching
 * - Seamless integration with existing completion system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>

#include "../include/rich_completion.h"
#include "../include/completion.h"
#include "../include/builtins.h"
#include "../include/config.h"
#include "../include/symtable.h"
#include "../include/alias.h"

// ============================================================================
// GLOBAL STATE AND CONFIGURATION
// ============================================================================

static bool initialized = false;

static rich_completion_config_t rich_config = {
    .enabled = true,
    .show_descriptions = true,
    .show_file_details = true,
    .show_command_types = true,
    .max_description_length = 60,
    .max_completions_displayed = 50,
    .description_color = "\033[90m",      // Gray
    .category_color = "\033[94m",         // Blue
    .file_color = "\033[92m",             // Green
    .command_color = "\033[93m",          // Yellow
    .variable_color = "\033[95m"          // Magenta
};

// Performance cache for expensive operations
typedef struct {
    char *command;
    char *description;
    time_t cache_time;
    bool is_valid;
} command_description_cache_entry_t;

#define DESCRIPTION_CACHE_SIZE 256
static command_description_cache_entry_t desc_cache[DESCRIPTION_CACHE_SIZE];
static int desc_cache_initialized = 0;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Get file size in human-readable format
 */
static char* get_file_size_string(off_t size) {
    static char buffer[32];
    
    if (size < 1024) {
        snprintf(buffer, sizeof(buffer), "%lld B", (long long)size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f KB", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f MB", size / (1024.0 * 1024.0));
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
    
    return buffer;
}

/**
 * Get file type description
 */
static const char* get_file_type_description(mode_t mode) {
    if (S_ISREG(mode)) {
        if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
            return "executable";
        }
        return "file";
    } else if (S_ISDIR(mode)) {
        return "directory";
    } else if (S_ISLNK(mode)) {
        return "symlink";
    } else if (S_ISFIFO(mode)) {
        return "pipe";
    } else if (S_ISSOCK(mode)) {
        return "socket";
    } else if (S_ISBLK(mode)) {
        return "block device";
    } else if (S_ISCHR(mode)) {
        return "char device";
    }
    return "unknown";
}

/**
 * Hash function for cache
 */
static unsigned int hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash % DESCRIPTION_CACHE_SIZE;
}

/**
 * Get cached command description
 */
static const char* get_cached_command_description(const char *command) {
    if (!desc_cache_initialized) {
        memset(desc_cache, 0, sizeof(desc_cache));
        desc_cache_initialized = 1;
    }
    
    unsigned int index = hash_string(command);
    command_description_cache_entry_t *entry = &desc_cache[index];
    
    // Check if cache entry is valid and matches
    if (entry->is_valid && entry->command && 
        strcmp(entry->command, command) == 0) {
        // Check if cache entry is still fresh (5 minutes)
        if (time(NULL) - entry->cache_time < 300) {
            return entry->description;
        }
    }
    
    return NULL;
}

/**
 * Cache command description
 */
static void cache_command_description(const char *command, const char *description) {
    if (!command || !description) return;
    
    unsigned int index = hash_string(command);
    command_description_cache_entry_t *entry = &desc_cache[index];
    
    // Free existing entry
    free(entry->command);
    free(entry->description);
    
    // Set new entry
    entry->command = strdup(command);
    entry->description = strdup(description);
    entry->cache_time = time(NULL);
    entry->is_valid = true;
}

// ============================================================================
// DESCRIPTION GENERATORS
// ============================================================================

/**
 * Get description for a builtin command
 */
static char* get_builtin_description(const char *command) {
    // Built-in command descriptions
    struct {
        const char *cmd;
        const char *desc;
    } builtin_descriptions[] = {
        {"cd", "Change directory"},
        {"pwd", "Print working directory"},
        {"ls", "List directory contents"},
        {"echo", "Display text"},
        {"export", "Set environment variable"},
        {"set", "Set shell variable"},
        {"unset", "Unset variable"},
        {"alias", "Create command alias"},
        {"unalias", "Remove alias"},
        {"history", "Command history"},
        {"jobs", "List active jobs"},
        {"fg", "Bring job to foreground"},
        {"bg", "Send job to background"},
        {"kill", "Terminate process"},
        {"exit", "Exit shell"},
        {"source", "Execute commands from file"},
        {".", "Execute commands from file"},
        {"type", "Show command type"},
        {"which", "Locate command"},
        {"help", "Show help information"},
        {"theme", "Change shell theme"},
        {"complete", "Manage completions"},
        {"bind", "Manage key bindings"},
        {"test", "Evaluate expressions"},
        {"[", "Evaluate expressions"},
        {"true", "Always succeeds"},
        {"false", "Always fails"},
        {"return", "Return from function"},
        {"break", "Break from loop"},
        {"continue", "Continue loop"},
        {"if", "Conditional execution"},
        {"while", "Loop construct"},
        {"for", "Loop construct"},
        {"case", "Pattern matching"},
        {"function", "Define function"},
        {NULL, NULL}
    };
    
    for (int i = 0; builtin_descriptions[i].cmd; i++) {
        if (strcmp(command, builtin_descriptions[i].cmd) == 0) {
            return strdup(builtin_descriptions[i].desc);
        }
    }
    
    return NULL;
}

/**
 * Get description for an external command using man page
 */
static char* get_external_command_description(const char *command) {
    // Check cache first
    const char *cached = get_cached_command_description(command);
    if (cached) {
        return strdup(cached);
    }
    
    char cmd_buffer[256];
    char line_buffer[512];
    char *description = NULL;
    
    // Try to get description from whatis/man
    snprintf(cmd_buffer, sizeof(cmd_buffer), "whatis '%s' 2>/dev/null | head -1", command);
    
    FILE *fp = popen(cmd_buffer, "r");
    if (fp) {
        if (fgets(line_buffer, sizeof(line_buffer), fp)) {
            // Parse whatis output: "command (section) - description"
            char *dash = strstr(line_buffer, " - ");
            if (dash) {
                dash += 3; // Skip " - "
                
                // Remove trailing newline
                char *newline = strchr(dash, '\n');
                if (newline) *newline = '\0';
                
                // Truncate if too long
                if (strlen(dash) > rich_config.max_description_length) {
                    dash[rich_config.max_description_length - 3] = '.';
                    dash[rich_config.max_description_length - 2] = '.';
                    dash[rich_config.max_description_length - 1] = '.';
                    dash[rich_config.max_description_length] = '\0';
                }
                
                description = strdup(dash);
            }
        }
        pclose(fp);
    }
    
    // Fallback: try to determine command type
    if (!description) {
        struct stat st;
        if (stat(command, &st) == 0) {
            if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
                description = strdup("executable program");
            }
        } else {
            // Try to find in PATH
            snprintf(cmd_buffer, sizeof(cmd_buffer), "which '%s' >/dev/null 2>&1", command);
            if (system(cmd_buffer) == 0) {
                description = strdup("command");
            }
        }
    }
    
    // Cache the result (even if NULL)
    if (description) {
        cache_command_description(command, description);
    } else {
        cache_command_description(command, "external command");
        description = strdup("external command");
    }
    
    return description;
}

/**
 * Get description for a file or directory
 */
static char* get_file_description(const char *filename) {
    if (!rich_config.show_file_details) {
        return NULL;
    }
    
    struct stat st;
    if (stat(filename, &st) != 0) {
        return NULL;
    }
    
    char *description = malloc(128);
    if (!description) return NULL;
    
    const char *type_desc = get_file_type_description(st.st_mode);
    
    if (S_ISREG(st.st_mode)) {
        char *size_str = get_file_size_string(st.st_size);
        snprintf(description, 128, "%s, %s", type_desc, size_str);
    } else {
        strncpy(description, type_desc, 127);
        description[127] = '\0';
    }
    
    return description;
}

/**
 * Get description for a variable
 */
static char* get_variable_description(const char *varname) {
    // Remove $ prefix if present
    if (varname[0] == '$') {
        varname++;
    }
    
    // Special shell variables
    struct {
        const char *var;
        const char *desc;
    } special_vars[] = {
        {"HOME", "User home directory"},
        {"PATH", "Command search path"},
        {"PWD", "Current directory"},
        {"OLDPWD", "Previous directory"},
        {"USER", "Current username"},
        {"SHELL", "Current shell"},
        {"TERM", "Terminal type"},
        {"EDITOR", "Default editor"},
        {"PAGER", "Default pager"},
        {"LANG", "System language"},
        {"LC_ALL", "Locale settings"},
        {"PS1", "Primary prompt"},
        {"PS2", "Secondary prompt"},
        {"IFS", "Input field separator"},
        {"RANDOM", "Random number"},
        {"SECONDS", "Seconds since shell start"},
        {"LINENO", "Current line number"},
        {"BASHPID", "Shell process ID"},
        {"PPID", "Parent process ID"},
        {"UID", "User ID"},
        {"EUID", "Effective user ID"},
        {"?", "Last command exit status"},
        {"$", "Shell process ID"},
        {"!", "Last background job PID"},
        {"0", "Script/shell name"},
        {"#", "Number of arguments"},
        {"*", "All arguments"},
        {"@", "All arguments (array)"},
        {NULL, NULL}
    };
    
    for (int i = 0; special_vars[i].var; i++) {
        if (strcmp(varname, special_vars[i].var) == 0) {
            return strdup(special_vars[i].desc);
        }
    }
    
    // Check if it's a numbered parameter ($1, $2, etc.)
    if (isdigit(varname[0]) && varname[1] == '\0') {
        return strdup("positional parameter");
    }
    
    // Try to get the variable value to show as description
    const char *value = getenv(varname);
    if (value && strlen(value) > 0) {
        char *desc = malloc(rich_config.max_description_length + 16);
        if (desc) {
            if (strlen(value) <= rich_config.max_description_length - 10) {
                snprintf(desc, rich_config.max_description_length + 16, "value: %s", value);
            } else {
                snprintf(desc, rich_config.max_description_length + 16, "value: %.50s...", value);
            }
            return desc;
        }
    }
    
    return strdup("environment variable");
}

// ============================================================================
// RICH COMPLETION GENERATORS
// ============================================================================

/**
 * Convert regular completion to rich completion
 */
static rich_completion_item_t* create_rich_completion(const char *text, 
                                                     completion_category_t category,
                                                     const char *description) {
    rich_completion_item_t *item = calloc(1, sizeof(rich_completion_item_t));
    if (!item) return NULL;
    
    item->completion = strdup(text ? text : "");
    item->description = description ? strdup(description) : NULL;
    item->category = category;
    
    // Set display properties based on category
    switch (category) {
        case COMPLETION_COMMAND:
            item->display_color = rich_config.command_color;
            break;
        case COMPLETION_BUILTIN:
            item->display_color = rich_config.command_color;
            break;
        case COMPLETION_FILE:
        case COMPLETION_DIRECTORY:
            item->display_color = rich_config.file_color;
            break;
        case COMPLETION_VARIABLE:
            item->display_color = rich_config.variable_color;
            break;
        default:
            item->display_color = NULL;
            break;
    }
    
    return item;
}

/**
 * Generate rich completions for commands
 */
static void generate_rich_command_completions(const char *text, rich_completion_list_t *list) {
    lusush_completions_t basic_completions = {0};
    complete_commands(text, &basic_completions);
    
    for (size_t i = 0; i < basic_completions.len; i++) {
        const char *cmd = basic_completions.cvec[i];
        char *description = NULL;
        completion_category_t category = COMPLETION_COMMAND;
        
        // Check if it's a builtin
        if (is_builtin(cmd)) {
            description = get_builtin_description(cmd);
            category = COMPLETION_BUILTIN;
        } else {
            description = get_external_command_description(cmd);
            category = COMPLETION_COMMAND;
        }
        
        rich_completion_item_t *item = create_rich_completion(cmd, category, description);
        if (item) {
            if (list->count < MAX_RICH_COMPLETIONS) {
                list->items[list->count++] = item;
            } else {
                // Free excess items
                free(item->completion);
                free(item->description);
                free(item);
            }
        }
        
        free(description);
    }
    
    lusush_free_completions(&basic_completions);
}

/**
 * Generate rich completions for files
 */
static void generate_rich_file_completions(const char *text, rich_completion_list_t *list) {
    lusush_completions_t basic_completions = {0};
    complete_files(text, &basic_completions);
    
    for (size_t i = 0; i < basic_completions.len; i++) {
        const char *filename = basic_completions.cvec[i];
        char *description = get_file_description(filename);
        
        // Determine if it's a file or directory
        completion_category_t category = COMPLETION_FILE;
        struct stat st;
        if (stat(filename, &st) == 0 && S_ISDIR(st.st_mode)) {
            category = COMPLETION_DIRECTORY;
        }
        
        rich_completion_item_t *item = create_rich_completion(filename, category, description);
        if (item) {
            if (list->count < MAX_RICH_COMPLETIONS) {
                list->items[list->count++] = item;
            } else {
                free(item->completion);
                free(item->description);
                free(item);
            }
        }
        
        free(description);
    }
    
    lusush_free_completions(&basic_completions);
}

/**
 * Generate rich completions for variables
 */
static void generate_rich_variable_completions(const char *text, rich_completion_list_t *list) {
    lusush_completions_t basic_completions = {0};
    complete_variables(text, &basic_completions);
    
    for (size_t i = 0; i < basic_completions.len; i++) {
        const char *var = basic_completions.cvec[i];
        char *description = get_variable_description(var);
        
        rich_completion_item_t *item = create_rich_completion(var, COMPLETION_VARIABLE, description);
        if (item) {
            if (list->count < MAX_RICH_COMPLETIONS) {
                list->items[list->count++] = item;
            } else {
                free(item->completion);
                free(item->description);
                free(item);
            }
        }
        
        free(description);
    }
    
    lusush_free_completions(&basic_completions);
}

// ============================================================================
// MAIN API FUNCTIONS
// ============================================================================

/**
 * Initialize rich completion system
 */
bool lusush_rich_completion_init(void) {
    if (initialized) {
        return true;
    }
    
    // Initialize description cache
    if (!desc_cache_initialized) {
        memset(desc_cache, 0, sizeof(desc_cache));
        desc_cache_initialized = 1;
    }
    
    initialized = true;
    return true;
}

/**
 * Cleanup rich completion system
 */
void lusush_rich_completion_cleanup(void) {
    if (!initialized) {
        return;
    }
    
    // Clean up description cache
    for (int i = 0; i < DESCRIPTION_CACHE_SIZE; i++) {
        free(desc_cache[i].command);
        free(desc_cache[i].description);
        memset(&desc_cache[i], 0, sizeof(desc_cache[i]));
    }
    
    desc_cache_initialized = 0;
    initialized = false;
}

/**
 * Generate rich completions for given input
 */
rich_completion_list_t* lusush_get_rich_completions(const char *text, completion_context_t context) {
    if (!initialized || !rich_config.enabled || !text) {
        return NULL;
    }
    
    rich_completion_list_t *list = calloc(1, sizeof(rich_completion_list_t));
    if (!list) return NULL;
    
    // Generate completions based on context
    switch (context) {
        case CONTEXT_COMMAND:
            generate_rich_command_completions(text, list);
            break;
            
        case CONTEXT_FILE:
        case CONTEXT_ARGUMENT:
            generate_rich_file_completions(text, list);
            break;
            
        case CONTEXT_VARIABLE:
            generate_rich_variable_completions(text, list);
            break;
            
        case CONTEXT_MIXED:
        default:
            // Try all types for mixed context
            generate_rich_command_completions(text, list);
            generate_rich_file_completions(text, list);
            generate_rich_variable_completions(text, list);
            break;
    }
    
    return list;
}

/**
 * Free rich completion list
 */
void lusush_free_rich_completions(rich_completion_list_t *list) {
    if (!list) return;
    
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i]) {
            free(list->items[i]->completion);
            free(list->items[i]->description);
            free(list->items[i]);
        }
    }
    
    free(list);
}

/**
 * Configure rich completion system
 */
void lusush_configure_rich_completion(const rich_completion_config_t *new_config) {
    if (!new_config) return;
    
    rich_config.enabled = new_config->enabled;
    rich_config.show_descriptions = new_config->show_descriptions;
    rich_config.show_file_details = new_config->show_file_details;
    rich_config.show_command_types = new_config->show_command_types;
    rich_config.max_description_length = new_config->max_description_length;
    rich_config.max_completions_displayed = new_config->max_completions_displayed;
    
    // Update colors if provided
    if (new_config->description_color) {
        free((char*)rich_config.description_color);
        rich_config.description_color = strdup(new_config->description_color);
    }
    if (new_config->category_color) {
        free((char*)rich_config.category_color);
        rich_config.category_color = strdup(new_config->category_color);
    }
    if (new_config->file_color) {
        free((char*)rich_config.file_color);
        rich_config.file_color = strdup(new_config->file_color);
    }
    if (new_config->command_color) {
        free((char*)rich_config.command_color);
        rich_config.command_color = strdup(new_config->command_color);
    }
    if (new_config->variable_color) {
        free((char*)rich_config.variable_color);
        rich_config.variable_color = strdup(new_config->variable_color);
    }
}

/**
 * Display rich completions in a formatted way
 */
void lusush_display_rich_completions(const rich_completion_list_t *list) {
    if (!list || list->count == 0) {
        return;
    }
    
    printf("\n");
    
    // Calculate column widths
    size_t max_completion_len = 0;
    for (size_t i = 0; i < list->count; i++) {
        size_t len = strlen(list->items[i]->completion);
        if (len > max_completion_len) {
            max_completion_len = len;
        }
    }
    
    // Display completions with descriptions
    size_t displayed = 0;
    for (size_t i = 0; i < list->count && displayed < rich_config.max_completions_displayed; i++) {
        rich_completion_item_t *item = list->items[i];
        
        // Print completion with color if enabled
        if (item->display_color && rich_config.show_command_types) {
            printf("%s%-*s\033[0m", item->display_color, 
                   (int)max_completion_len + 2, item->completion);
        } else {
            printf("%-*s", (int)max_completion_len + 2, item->completion);
        }
        
        // Print description if available and enabled
        if (item->description && rich_config.show_descriptions) {
            printf("%s%s\033[0m", rich_config.description_color, item->description);
        }
        
        printf("\n");
        displayed++;
    }
    
    if (list->count > rich_config.max_completions_displayed) {
        printf("%s... and %zu more\033[0m\n", 
               rich_config.description_color,
               list->count - rich_config.max_completions_displayed);
    }
    
    printf("\n");
}

/**
 * Check if rich completions are enabled
 */
bool lusush_are_rich_completions_enabled(void) {
    return initialized && rich_config.enabled;
}

/**
 * Set rich completion enabled state
 */
void lusush_set_rich_completion_enabled(bool enabled) {
    rich_config.enabled = enabled;
}

/**
 * Get default rich completion configuration
 */
rich_completion_config_t lusush_get_default_rich_completion_config(void) {
    rich_completion_config_t default_config = {
        .enabled = true,
        .show_descriptions = true,
        .show_file_details = true,
        .show_command_types = true,
        .max_description_length = 60,
        .max_completions_displayed = 50,
        .description_color = "\033[90m",      // Gray
        .category_color = "\033[94m",         // Blue
        .file_color = "\033[92m",             // Green
        .command_color = "\033[93m",          // Yellow
        .variable_color = "\033[95m"          // Magenta
    };
    return default_config;
}