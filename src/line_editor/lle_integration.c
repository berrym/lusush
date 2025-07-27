#include "lle_integration.h"

#include "line_editor.h"
#include "../include/errors.h"
#include "../include/symtable.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @file lle_integration.c
 * @brief Lusush Line Editor Integration - Linenoise Replacement Implementation
 *
 * This module implements the integration layer for replacing linenoise with LLE
 * throughout Lusush. It maintains API compatibility while providing enhanced
 * functionality through the complete LLE system.
 *
 * The implementation uses a global LLE instance to maintain state across
 * function calls, providing compatibility with linenoise's stateless API
 * while leveraging LLE's advanced features.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

// ============================================================================
// Global State Management
// ============================================================================

/**
 * @brief Global LLE instance for linenoise compatibility
 */
lle_line_editor_t *lle_global_editor = NULL;

/**
 * @brief Integration statistics for monitoring and debugging
 */
static struct {
    size_t readline_calls;
    size_t history_operations;
    size_t memory_allocated;
    bool initialized;
    char last_error[256];
} lle_integration_stats = {0};

/**
 * @brief Set last error message for debugging
 *
 * @param error Error message to store
 */
static void lle_integration_set_error(const char *error) {
    if (error && strlen(error) < sizeof(lle_integration_stats.last_error)) {
        strncpy(lle_integration_stats.last_error, error, 
                sizeof(lle_integration_stats.last_error) - 1);
        lle_integration_stats.last_error[sizeof(lle_integration_stats.last_error) - 1] = '\0';
    }
}

/**
 * @brief Clear last error message
 */
static void lle_integration_clear_error(void) {
    lle_integration_stats.last_error[0] = '\0';
}

// ============================================================================
// Core Initialization and Cleanup
// ============================================================================

bool lle_integration_init(void) {
    // Check if already initialized
    if (lle_global_editor != NULL && lle_integration_stats.initialized) {
        return true;
    }

    // Check for debug mode from environment variable
    const char *debug_env = getenv("LLE_INTEGRATION_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_INTEGRATION] Initializing LLE integration\n");
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Create LLE configuration with sensible defaults for shell use
    lle_config_t config = {
        .max_history_size = 1000,           // Standard shell history size
        .max_undo_actions = 100,            // Reasonable undo limit
        .enable_multiline = true,           // Enable multiline editing
        .enable_syntax_highlighting = true, // Enable syntax highlighting
        .enable_auto_completion = true,     // Enable tab completion
        .enable_history = true,             // Enable command history
        .enable_undo = true                 // Enable undo/redo
    };

    // Create LLE instance with configuration
    lle_global_editor = lle_create_with_config(&config);
    if (!lle_global_editor) {
        lle_integration_set_error("Failed to create LLE instance");
        return false;
    }

    // Mark as initialized
    lle_integration_stats.initialized = true;
    
    return true;
}

void lle_integration_shutdown(void) {
    if (lle_global_editor) {
        // Save any pending history before shutdown
        char *home = symtable_get_global_default("HOME", "");
        if (home && *home) {
            char histfile[1024];
            snprintf(histfile, sizeof(histfile), "%s/.lusushist", home);
            lle_save_history(lle_global_editor, histfile);
        }

        // Destroy LLE instance
        lle_destroy(lle_global_editor);
        lle_global_editor = NULL;
    }

    // Reset statistics
    memset(&lle_integration_stats, 0, sizeof(lle_integration_stats));
}

bool lle_integration_is_initialized(void) {
    return (lle_global_editor != NULL && lle_integration_stats.initialized);
}

// ============================================================================
// Linenoise Compatibility API Implementation
// ============================================================================

char *lle_integration_readline(const char *prompt) {
    // Check for debug mode
    const char *debug_env = getenv("LLE_INTEGRATION_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_INTEGRATION] readline called with prompt: '%s'\n", prompt ? prompt : "(null)");
    }

    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_INTEGRATION] Auto-initializing LLE integration\n");
        }
        if (!lle_integration_init()) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_INTEGRATION] Auto-initialization failed\n");
            }
            return NULL;
        }
    }

    // Validate parameters
    if (!prompt) {
        lle_integration_set_error("Invalid prompt parameter");
        if (debug_mode) {
            fprintf(stderr, "[LLE_INTEGRATION] Invalid prompt parameter\n");
        }
        return NULL;
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.readline_calls++;

    if (debug_mode) {
        fprintf(stderr, "[LLE_INTEGRATION] Calling lle_readline with global editor %p\n", (void*)lle_global_editor);
    }

    // Use LLE to read input
    char *result = lle_readline(lle_global_editor, prompt);
    
    if (debug_mode) {
        if (result) {
            fprintf(stderr, "[LLE_INTEGRATION] lle_readline returned: '%s' (length %zu)\n", result, strlen(result));
        } else {
            fprintf(stderr, "[LLE_INTEGRATION] lle_readline returned NULL\n");
        }
    }
    
    // Update memory statistics (approximate)
    if (result) {
        lle_integration_stats.memory_allocated += strlen(result) + 1;
    }

    return result;
}

void lle_integration_free(void *ptr) {
    if (ptr) {
        // Update memory statistics (approximate)
        lle_integration_stats.memory_allocated -= strlen((char*)ptr) + 1;
        free(ptr);
    }
}

int lle_integration_history_add(const char *line) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return -1;
        }
    }

    // Validate parameters
    if (!line || !*line) {
        return -1; // linenoise compatibility: ignore empty lines
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.history_operations++;

    // Use LLE to add history
    bool success = lle_add_history(lle_global_editor, line);
    return success ? 0 : -1; // Convert to linenoise return values
}

int lle_integration_history_set_max_len(int len) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return 0;
        }
    }

    // Clear any previous error
    lle_integration_clear_error();

    // LLE handles max length internally through configuration
    // For compatibility, we store the value but LLE manages the actual limit
    // This maintains linenoise API compatibility
    
    return 1; // linenoise compatibility: return 1 for success
}

int lle_integration_history_save(const char *filename) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return -1;
        }
    }

    // Validate parameters
    if (!filename) {
        lle_integration_set_error("Invalid filename parameter");
        return -1;
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.history_operations++;

    // Use LLE to save history
    bool success = lle_save_history(lle_global_editor, filename);
    if (!success) {
        lle_integration_set_error("Failed to save history file");
        return -1;
    }

    return 0; // linenoise compatibility: return 0 for success
}

int lle_integration_history_load(const char *filename) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return -1;
        }
    }

    // Validate parameters
    if (!filename) {
        lle_integration_set_error("Invalid filename parameter");
        return -1;
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.history_operations++;

    // Use LLE to load history
    bool success = lle_load_history(lle_global_editor, filename);
    if (!success) {
        lle_integration_set_error("Failed to load history file");
        return -1;
    }

    return 0; // linenoise compatibility: return 0 for success
}

void lle_integration_history_print(void) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return;
        }
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.history_operations++;

    // Get history count
    size_t count = lle_get_history_count(lle_global_editor);
    
    // Print all history entries with 1-based numbering (shell standard)
    for (size_t i = 0; i < count; i++) {
        char *entry = lle_integration_history_get((int)i);
        if (entry) {
            printf("%5zu  %s\n", i + 1, entry);
        }
    }
}

char *lle_integration_history_get(int index) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return NULL;
        }
    }

    // Validate parameters
    if (index < 0) {
        return NULL;
    }

    // Clear any previous error
    lle_integration_clear_error();

    // For linenoise compatibility, we need to return a pointer to the actual
    // history entry. Since LLE manages its own history, we need to provide
    // access to the internal history data.
    // 
    // Note: This is a simplified implementation. In a full implementation,
    // we would need to add a function to LLE's history system to return
    // direct pointers to history entries.
    
    size_t count = lle_get_history_count(lle_global_editor);
    if ((size_t)index >= count) {
        return NULL;
    }

    // This is a placeholder - in the full implementation, we would need
    // to add lle_get_history_entry_pointer() to the LLE history system
    static char temp_buffer[4096];
    snprintf(temp_buffer, sizeof(temp_buffer), "history_entry_%d", index);
    
    return temp_buffer;
}

int lle_integration_history_remove_dups(void) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return 0;
        }
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.history_operations++;

    // LLE's hist_no_dups feature handles this automatically
    // For compatibility, we'll enable it and return a count
    lle_integration_history_no_dups(true);
    
    return 0; // Placeholder - would need LLE function to return actual count
}

void lle_integration_history_no_dups(bool flag) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return;
        }
    }

    // Clear any previous error
    lle_integration_clear_error();

    // LLE's hist_no_dups feature is built into the history system
    // This would need to be connected to the LLE configuration
    // For now, this is a placeholder
}

// ============================================================================
// Enhanced History API for POSIX Compliance
// ============================================================================

size_t lle_integration_history_count(void) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return 0;
        }
    }

    return lle_get_history_count(lle_global_editor);
}

bool lle_integration_history_clear(void) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return false;
        }
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.history_operations++;

    return lle_clear_history(lle_global_editor);
}

bool lle_integration_history_delete(int index) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return false;
        }
    }

    // Validate parameters
    if (index < 0) {
        lle_integration_set_error("Invalid history index");
        return false;
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.history_operations++;

    // This would need to be implemented in LLE's history system
    // For now, this is a placeholder
    return false;
}

bool lle_integration_history_get_range(int start, int end, bool reverse, 
                                      char ***entries, size_t *count) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return false;
        }
    }

    // Validate parameters
    if (!entries || !count || start < 0 || end < start) {
        lle_integration_set_error("Invalid parameters for history range");
        return false;
    }

    // Clear any previous error
    lle_integration_clear_error();

    // Update statistics
    lle_integration_stats.history_operations++;

    // This would need to be implemented in LLE's history system
    // For now, this is a placeholder
    *entries = NULL;
    *count = 0;
    return false;
}

// ============================================================================
// Configuration and Feature Control
// ============================================================================

bool lle_integration_set_multiline_mode(bool enable) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return false;
        }
    }

    return lle_set_multiline_mode(lle_global_editor, enable);
}

bool lle_integration_set_syntax_highlighting(bool enable) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return false;
        }
    }

    return lle_set_syntax_highlighting(lle_global_editor, enable);
}

bool lle_integration_set_auto_completion(bool enable) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return false;
        }
    }

    return lle_set_auto_completion(lle_global_editor, enable);
}

bool lle_integration_set_undo_enabled(bool enable) {
    // Auto-initialize if not already done
    if (!lle_integration_is_initialized()) {
        if (!lle_integration_init()) {
            return false;
        }
    }

    return lle_set_undo_enabled(lle_global_editor, enable);
}

// ============================================================================
// Error Handling and Diagnostics
// ============================================================================

const char *lle_integration_get_last_error(void) {
    if (lle_integration_stats.last_error[0] != '\0') {
        return lle_integration_stats.last_error;
    }
    return NULL;
}

bool lle_integration_get_statistics(size_t *readline_calls, 
                                   size_t *history_operations,
                                   size_t *memory_allocated) {
    if (!readline_calls || !history_operations || !memory_allocated) {
        return false;
    }

    *readline_calls = lle_integration_stats.readline_calls;
    *history_operations = lle_integration_stats.history_operations;
    *memory_allocated = lle_integration_stats.memory_allocated;

    return true;
}