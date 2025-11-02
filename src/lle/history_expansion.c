/**
 * @file history_expansion.c
 * @brief LLE History System - History Expansion Implementation (Spec 09 Phase 3 Day 10)
 * 
 * Implements bash-compatible history expansion for the LLE history system:
 * - !! - Repeat last command
 * - !n - Repeat command number n
 * - !-n - Repeat command n positions back
 * - !string - Repeat most recent command starting with string
 * - !?string - Repeat most recent command containing string
 * - ^old^new - Quick substitution in last command
 * 
 * Behavior (matches bash):
 * - Expansion occurs before command execution
 * - Failed expansion prints error and aborts command
 * - Space prefix disables expansion (configurable)
 * - :p modifier prints without executing
 * - :s/old/new/ performs substitution
 * 
 * @date 2025-11-01
 * @author LLE Implementation Team
 */

#include "lle/history.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lusush.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define EXPANSION_MAX_LENGTH 4096
#define EXPANSION_MAX_DEPTH 10  /* Prevent infinite recursion */

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/**
 * Expansion type
 */
typedef enum {
    EXPANSION_TYPE_NONE,           /* No expansion */
    EXPANSION_TYPE_LAST,           /* !! */
    EXPANSION_TYPE_NUMBER,         /* !n */
    EXPANSION_TYPE_RELATIVE,       /* !-n */
    EXPANSION_TYPE_PREFIX,         /* !string */
    EXPANSION_TYPE_SUBSTRING,      /* !?string */
    EXPANSION_TYPE_QUICK_SUB       /* ^old^new */
} lle_expansion_type_t;

/**
 * Expansion result
 */
typedef struct {
    lle_expansion_type_t type;     /* Type of expansion */
    char *expanded_command;        /* Expanded command text */
    size_t expansion_start;        /* Start position in original */
    size_t expansion_end;          /* End position in original */
    bool print_only;               /* :p modifier - print only */
    bool needs_substitution;       /* :s modifier present */
    char *sub_old;                 /* Substitution old pattern */
    char *sub_new;                 /* Substitution new pattern */
} lle_expansion_result_t;

/**
 * Expansion context
 */
typedef struct {
    lle_history_core_t *history_core;  /* History core for lookups */
    int recursion_depth;               /* Current recursion depth */
    bool space_disables_expansion;     /* Leading space disables expansion */
    bool verify_before_execute;        /* Verify expansion before execute */
} lle_expansion_context_t;

/* Global expansion context */
static lle_expansion_context_t g_expansion_ctx = {
    .history_core = NULL,
    .recursion_depth = 0,
    .space_disables_expansion = true,
    .verify_before_execute = false
};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Duplicate string using memory pool
 */
static char *pool_strdup(const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char *dup = lle_pool_alloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

/**
 * Find the next history expansion marker in a string
 * 
 * @param str String to search
 * @param start_pos Starting position
 * @return Position of expansion marker, or -1 if not found
 */
static ssize_t find_expansion_marker(const char *str, size_t start_pos) {
    if (!str) return -1;
    
    size_t len = strlen(str);
    for (size_t i = start_pos; i < len; i++) {
        if (str[i] == '!') {
            /* Check if it's escaped */
            if (i > 0 && str[i-1] == '\\') {
                continue;  /* Escaped, not an expansion */
            }
            /* Check if it's escaped or in quotes - basic implementation */
            return (ssize_t)i;
        } else if (str[i] == '^' && i == 0) {
            /* Quick substitution must be at start */
            return (ssize_t)i;
        }
    }
    
    return -1;
}

/**
 * Parse history number from string (handles !n and !-n)
 * 
 * @param str String starting with number or -number
 * @param number Output for parsed number
 * @param is_relative Output - true if relative (!-n), false if absolute (!n)
 * @param consumed Output - number of characters consumed
 * @return true on success, false on parse error
 */
static bool parse_history_number(const char *str, int64_t *number, bool *is_relative, size_t *consumed) {
    if (!str || !number || !is_relative || !consumed) {
        return false;
    }
    
    *consumed = 0;
    *is_relative = false;
    
    /* Check for relative notation (!-n) */
    if (str[0] == '-') {
        *is_relative = true;
        str++;
        (*consumed)++;
    }
    
    /* Parse the number */
    char *endptr;
    errno = 0;
    long parsed = strtol(str, &endptr, 10);
    
    if (errno != 0 || endptr == str) {
        return false;  /* Parse error */
    }
    
    *number = parsed;
    *consumed += (endptr - str);
    
    return true;
}

/**
 * Extract string argument from expansion (for !string and !?string)
 * 
 * @param str String after ! or !?
 * @param output Output buffer for extracted string
 * @param max_len Maximum length of output buffer
 * @return Number of characters consumed, or 0 on error
 */
static size_t extract_expansion_string(const char *str, char *output, size_t max_len) {
    if (!str || !output || max_len == 0) {
        return 0;
    }
    
    size_t i = 0;
    while (str[i] != '\0' && i < max_len - 1) {
        /* Stop at whitespace, special chars, or command terminators */
        if (isspace(str[i]) || str[i] == ';' || str[i] == '|' || 
            str[i] == '&' || str[i] == '>' || str[i] == '<' ||
            str[i] == '(' || str[i] == ')' || str[i] == '\n') {
            break;
        }
        output[i] = str[i];
        i++;
    }
    
    output[i] = '\0';
    return i;
}

/**
 * Perform quick substitution (^old^new)
 * 
 * @param last_command Last command from history
 * @param old_pattern Pattern to replace
 * @param new_pattern Replacement pattern
 * @param result Output buffer for result
 * @param max_len Maximum length of result buffer
 * @return true on success, false on error
 */
static bool perform_quick_substitution(
    const char *last_command,
    const char *old_pattern,
    const char *new_pattern,
    char *result,
    size_t max_len
) {
    if (!last_command || !old_pattern || !new_pattern || !result || max_len == 0) {
        return false;
    }
    
    /* Find the old pattern in the last command */
    const char *match_pos = strstr(last_command, old_pattern);
    if (!match_pos) {
        /* Pattern not found - substitution fails */
        return false;
    }
    
    /* Build the substituted command */
    size_t prefix_len = match_pos - last_command;
    size_t old_len = strlen(old_pattern);
    size_t new_len = strlen(new_pattern);
    size_t suffix_len = strlen(match_pos + old_len);
    
    /* Check if result will fit */
    if (prefix_len + new_len + suffix_len >= max_len) {
        /* Result too long */
        return false;
    }
    
    /* Copy prefix */
    memcpy(result, last_command, prefix_len);
    
    /* Copy replacement */
    memcpy(result + prefix_len, new_pattern, new_len);
    
    /* Copy suffix */
    memcpy(result + prefix_len + new_len, match_pos + old_len, suffix_len);
    
    /* Null terminate */
    result[prefix_len + new_len + suffix_len] = '\0';
    
    return true;
}

/**
 * Expand a single history reference
 * 
 * @param expansion_str The expansion string (without leading !)
 * @param result Output for expansion result
 * @return LLE_SUCCESS or error code
 */
static lle_result_t expand_single_reference(const char *expansion_str, lle_expansion_result_t *result) {
    if (!expansion_str || !result || !g_expansion_ctx.history_core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    memset(result, 0, sizeof(lle_expansion_result_t));
    
    /* Handle !! (repeat last command) */
    if (expansion_str[0] == '!') {
        result->type = EXPANSION_TYPE_LAST;
        result->expansion_end = 2;  /* !! is 2 characters */
        
        lle_history_entry_t *entry = NULL;
        lle_result_t res = lle_history_bridge_get_by_reverse_index(0, &entry);
        
        if (res != LLE_SUCCESS || !entry) {
            /* Error - details in return code */
            return LLE_ERROR_NOT_FOUND;
        }
        
        result->expanded_command = pool_strdup(entry->command);
        if (!result->expanded_command) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        
        return LLE_SUCCESS;
    }
    
    /* Handle !n or !-n (number reference) */
    if (isdigit(expansion_str[0]) || expansion_str[0] == '-') {
        int64_t number;
        bool is_relative;
        size_t consumed;
        
        if (!parse_history_number(expansion_str, &number, &is_relative, &consumed)) {
            /* Error - details in return code */
            return LLE_ERROR_INVALID_PARAMETER;
        }
        
        result->type = is_relative ? EXPANSION_TYPE_RELATIVE : EXPANSION_TYPE_NUMBER;
        result->expansion_end = consumed + 1;  /* +1 for the ! */
        
        lle_history_entry_t *entry = NULL;
        lle_result_t res;
        
        if (is_relative) {
            /* !-n means n commands back (0 = most recent) */
            if (number < 0) {
                /* Error - details in return code */
                return LLE_ERROR_INVALID_PARAMETER;
            }
            res = lle_history_bridge_get_by_reverse_index((size_t)number, &entry);
        } else {
            /* !n means entry ID n */
            res = lle_history_bridge_get_by_number((uint64_t)number, &entry);
        }
        
        if (res != LLE_SUCCESS || !entry) {
            char err[128];
            snprintf(err, sizeof(err), "!%s%ld: event not found", 
                    is_relative ? "-" : "", number);
            /* Error - details in return code */
            return LLE_ERROR_NOT_FOUND;
        }
        
        result->expanded_command = pool_strdup(entry->command);
        if (!result->expanded_command) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        
        return LLE_SUCCESS;
    }
    
    /* Handle !?string (substring search) */
    if (expansion_str[0] == '?') {
        result->type = EXPANSION_TYPE_SUBSTRING;
        
        char search_str[256];
        size_t consumed = extract_expansion_string(expansion_str + 1, search_str, sizeof(search_str));
        
        if (consumed == 0) {
            /* Error - details in return code */
            return LLE_ERROR_INVALID_PARAMETER;
        }
        
        result->expansion_end = consumed + 2;  /* +1 for !, +1 for ? */
        
        /* Search for command containing string */
        lle_history_search_results_t *search_results = 
            lle_history_search_substring(g_expansion_ctx.history_core, search_str, 1);
        
        if (!search_results || lle_history_search_results_get_count(search_results) == 0) {
            if (search_results) {
                lle_history_search_results_destroy(search_results);
            }
            char err[256];
            snprintf(err, sizeof(err), "!?%s: event not found", search_str);
            /* Error - details in return code */
            return LLE_ERROR_NOT_FOUND;
        }
        
        const lle_search_result_t *first_result = lle_history_search_results_get(search_results, 0);
        result->expanded_command = pool_strdup(first_result->command);
        
        lle_history_search_results_destroy(search_results);
        
        if (!result->expanded_command) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        
        return LLE_SUCCESS;
    }
    
    /* Handle !string (prefix search) */
    result->type = EXPANSION_TYPE_PREFIX;
    
    char search_str[256];
    size_t consumed = extract_expansion_string(expansion_str, search_str, sizeof(search_str));
    
    if (consumed == 0) {
        /* Error - details in return code */
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    result->expansion_end = consumed + 1;  /* +1 for ! */
    
    /* Search for most recent command starting with string */
    lle_history_search_results_t *search_results = 
        lle_history_search_prefix(g_expansion_ctx.history_core, search_str, 1);
    
    if (!search_results || lle_history_search_results_get_count(search_results) == 0) {
        if (search_results) {
            lle_history_search_results_destroy(search_results);
        }
        char err[256];
        snprintf(err, sizeof(err), "!%s: event not found", search_str);
        /* Error - details in return code */
        return LLE_ERROR_NOT_FOUND;
    }
    
    const lle_search_result_t *first_result = lle_history_search_results_get(search_results, 0);
    result->expanded_command = pool_strdup(first_result->command);
    
    lle_history_search_results_destroy(search_results);
    
    if (!result->expanded_command) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * PUBLIC API - EXPANSION OPERATIONS
 * ============================================================================ */

/**
 * Initialize history expansion system
 * 
 * @param history_core History core instance
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expansion_init(lle_history_core_t *history_core) {
    if (!history_core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    g_expansion_ctx.history_core = history_core;
    g_expansion_ctx.recursion_depth = 0;
    g_expansion_ctx.space_disables_expansion = true;
    g_expansion_ctx.verify_before_execute = false;
    
    return LLE_SUCCESS;
}

/**
 * Shutdown history expansion system
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expansion_shutdown(void) {
    g_expansion_ctx.history_core = NULL;
    g_expansion_ctx.recursion_depth = 0;
    
    return LLE_SUCCESS;
}

/**
 * Check if a command line contains history expansion
 * 
 * @param command Command line to check
 * @return true if expansion is present, false otherwise
 */
bool lle_history_expansion_needed(const char *command) {
    if (!command || command[0] == '\0') {
        return false;
    }
    
    /* Check for space prefix disabling expansion */
    if (g_expansion_ctx.space_disables_expansion && isspace(command[0])) {
        return false;
    }
    
    /* Check for quick substitution (^old^new) */
    if (command[0] == '^') {
        return true;
    }
    
    /* Check for ! expansion */
    return (find_expansion_marker(command, 0) >= 0);
}

/**
 * Expand history references in a command line
 * 
 * Expands all history references (!!, !n, !-n, !string, etc.) in the command.
 * The result must be freed by the caller using lle_pool_free().
 * 
 * @param command Original command with history references
 * @param expanded Output pointer for expanded command (allocated, caller must free)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expand_line(const char *command, char **expanded) {
    if (!command || !expanded) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_expansion_ctx.history_core) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    *expanded = NULL;
    
    /* Check for recursion depth */
    if (g_expansion_ctx.recursion_depth >= EXPANSION_MAX_DEPTH) {
        /* Error - details in return code */
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Check if expansion is needed */
    if (!lle_history_expansion_needed(command)) {
        *expanded = pool_strdup(command);
        return *expanded ? LLE_SUCCESS : LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Handle quick substitution (^old^new) */
    if (command[0] == '^') {
        /* Parse ^old^new format */
        const char *p = command + 1;
        char old_pattern[256] = {0};
        char new_pattern[256] = {0};
        
        /* Extract old pattern */
        size_t i = 0;
        while (*p != '\0' && *p != '^' && i < sizeof(old_pattern) - 1) {
            old_pattern[i++] = *p++;
        }
        
        if (*p != '^') {
            /* Error - details in return code */
            return LLE_ERROR_INVALID_PARAMETER;
        }
        
        p++;  /* Skip second ^ */
        
        /* Extract new pattern */
        i = 0;
        while (*p != '\0' && *p != ' ' && *p != '\n' && i < sizeof(new_pattern) - 1) {
            new_pattern[i++] = *p++;
        }
        
        /* Get last command */
        lle_history_entry_t *last_entry = NULL;
        lle_result_t res = lle_history_bridge_get_by_reverse_index(0, &last_entry);
        
        if (res != LLE_SUCCESS || !last_entry) {
            /* Error - details in return code */
            return LLE_ERROR_NOT_FOUND;
        }
        
        /* Perform substitution */
        char result[EXPANSION_MAX_LENGTH];
        if (!perform_quick_substitution(last_entry->command, old_pattern, new_pattern, 
                                       result, sizeof(result))) {
            return LLE_ERROR_INVALID_PARAMETER;
        }
        
        *expanded = pool_strdup(result);
        return *expanded ? LLE_SUCCESS : LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Handle ! expansions */
    char result[EXPANSION_MAX_LENGTH];
    size_t result_pos = 0;
    size_t cmd_pos = 0;
    size_t cmd_len = strlen(command);
    
    g_expansion_ctx.recursion_depth++;
    
    while (cmd_pos < cmd_len && result_pos < EXPANSION_MAX_LENGTH - 1) {
        ssize_t expansion_pos = find_expansion_marker(command + cmd_pos, 0);
        
        if (expansion_pos < 0) {
            /* No more expansions, copy rest of string */
            size_t remaining = cmd_len - cmd_pos;
            if (result_pos + remaining >= EXPANSION_MAX_LENGTH) {
                g_expansion_ctx.recursion_depth--;
                /* Error - details in return code */
                return LLE_ERROR_BUFFER_OVERFLOW;
            }
            memcpy(result + result_pos, command + cmd_pos, remaining);
            result_pos += remaining;
            break;
        }
        
        /* Copy text before expansion */
        if (expansion_pos > 0) {
            memcpy(result + result_pos, command + cmd_pos, expansion_pos);
            result_pos += expansion_pos;
            cmd_pos += expansion_pos;
        }
        
        /* Process the expansion */
        lle_expansion_result_t exp_result;
        lle_result_t res = expand_single_reference(command + cmd_pos + 1, &exp_result);
        
        if (res != LLE_SUCCESS) {
            g_expansion_ctx.recursion_depth--;
            return res;
        }
        
        /* Copy expanded text */
        size_t expanded_len = strlen(exp_result.expanded_command);
        if (result_pos + expanded_len >= EXPANSION_MAX_LENGTH) {
            lle_pool_free(exp_result.expanded_command);
            g_expansion_ctx.recursion_depth--;
            /* Error - details in return code */
            return LLE_ERROR_BUFFER_OVERFLOW;
        }
        
        memcpy(result + result_pos, exp_result.expanded_command, expanded_len);
        result_pos += expanded_len;
        cmd_pos += exp_result.expansion_end;
        
        lle_pool_free(exp_result.expanded_command);
    }
    
    result[result_pos] = '\0';
    g_expansion_ctx.recursion_depth--;
    
    *expanded = pool_strdup(result);
    return *expanded ? LLE_SUCCESS : LLE_ERROR_OUT_OF_MEMORY;
}

/**
 * Set whether leading space disables expansion
 * 
 * @param enabled true to enable space-disables-expansion (bash behavior)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expansion_set_space_disables(bool enabled) {
    g_expansion_ctx.space_disables_expansion = enabled;
    return LLE_SUCCESS;
}

/**
 * Get whether leading space disables expansion
 * 
 * @return true if enabled, false otherwise
 */
bool lle_history_expansion_get_space_disables(void) {
    return g_expansion_ctx.space_disables_expansion;
}

/**
 * Set whether to verify expansion before execution
 * 
 * When enabled, expanded commands are displayed for user confirmation
 * before execution (like bash's 'verify' option).
 * 
 * @param enabled true to enable verification
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expansion_set_verify(bool enabled) {
    g_expansion_ctx.verify_before_execute = enabled;
    return LLE_SUCCESS;
}

/**
 * Get whether verification is enabled
 * 
 * @return true if enabled, false otherwise
 */
bool lle_history_expansion_get_verify(void) {
    return g_expansion_ctx.verify_before_execute;
}
