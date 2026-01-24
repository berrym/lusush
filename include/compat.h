/**
 * @file compat.h
 * @brief Shell compatibility database system
 *
 * Provides a database of behavioral differences between POSIX sh, Bash, Zsh,
 * and Lush shells. This database powers the static analyzer's portability
 * checking, allowing scripts to be verified for cross-shell compatibility.
 *
 * The compatibility system integrates with the shell mode system (shell_mode.h)
 * and debug analysis system (debug/debug_analysis.h) to provide comprehensive
 * portability warnings.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#ifndef COMPAT_H
#define COMPAT_H

#include <stdbool.h>
#include <stddef.h>
#include "shell_mode.h"

/* ============================================================================
 * Compatibility Entry Types
 * ============================================================================ */

/**
 * @brief Compatibility entry categories
 *
 * Categories for organizing compatibility entries by the type of shell
 * construct they describe.
 */
typedef enum {
    COMPAT_CATEGORY_BUILTIN,   /**< Built-in command differences (echo, test) */
    COMPAT_CATEGORY_EXPANSION, /**< Variable/parameter expansion differences */
    COMPAT_CATEGORY_QUOTING,   /**< Quoting and escaping differences */
    COMPAT_CATEGORY_SYNTAX,    /**< Syntax differences (arrays, functions) */
    COMPAT_CATEGORY_COUNT      /**< Number of categories (for array sizing) */
} compat_category_t;

/**
 * @brief Lint severity levels
 *
 * Severity levels for compatibility warnings, used by the analyzer to
 * determine how to report issues.
 */
typedef enum {
    COMPAT_SEVERITY_INFO,    /**< Informational - no action needed */
    COMPAT_SEVERITY_WARNING, /**< May cause portability issues */
    COMPAT_SEVERITY_ERROR,   /**< Will definitely fail in target shell */
    COMPAT_SEVERITY_COUNT    /**< Number of severity levels */
} compat_severity_t;

/**
 * @brief Shell behavior description
 *
 * Describes how a specific shell handles a particular construct.
 */
typedef struct {
    const char *posix; /**< POSIX sh behavior */
    const char *bash;  /**< Bash behavior */
    const char *zsh;   /**< Zsh behavior */
    const char *lush;  /**< Lush behavior */
} compat_behavior_t;

/**
 * @brief Lint configuration for a compatibility entry
 *
 * Defines how the analyzer should detect and report this compatibility
 * issue.
 */
typedef struct {
    compat_severity_t severity; /**< Severity level */
    const char *message;        /**< Message shown to user */
    const char *suggestion;     /**< Suggested fix (optional) */
    const char *pattern;        /**< Regex pattern to detect (optional) */
} compat_lint_t;

/**
 * @brief Compatibility database entry
 *
 * A single entry in the compatibility database, describing a behavioral
 * difference between shells.
 */
typedef struct {
    const char *id;              /**< Unique identifier */
    compat_category_t category;  /**< Category of this entry */
    const char *feature;         /**< Feature name (e.g., "echo", "arrays") */
    const char *description;     /**< Human-readable description */
    compat_behavior_t behavior;  /**< Per-shell behavior descriptions */
    compat_lint_t lint;          /**< Lint configuration */
} compat_entry_t;

/**
 * @brief Compatibility check result
 *
 * Result of checking a construct for compatibility with a target shell.
 */
typedef struct {
    bool is_portable;             /**< True if construct is portable to target */
    const compat_entry_t *entry;  /**< Entry that matched (if not portable) */
    shell_mode_t target;          /**< Target shell that was checked against */
    int line;                     /**< Line number where issue was found */
    int column;                   /**< Column number where issue was found */
} compat_result_t;

/* ============================================================================
 * Database Management
 * ============================================================================ */

/**
 * @brief Initialize the compatibility database
 *
 * Loads all compatibility data from TOML files in the data directory.
 * Should be called early in shell initialization.
 *
 * @param data_dir Path to data directory (NULL for default)
 * @return 0 on success, -1 on error
 */
int compat_init(const char *data_dir);

/**
 * @brief Clean up the compatibility database
 *
 * Frees all resources associated with the compatibility database.
 */
void compat_cleanup(void);

/**
 * @brief Reload the compatibility database
 *
 * Reloads all compatibility data from disk. Useful if TOML files have
 * been modified.
 *
 * @return 0 on success, -1 on error
 */
int compat_reload(void);

/* ============================================================================
 * Entry Query Functions
 * ============================================================================ */

/**
 * @brief Get a compatibility entry by ID
 *
 * @param id Entry ID to look up
 * @return Pointer to entry, or NULL if not found
 */
const compat_entry_t *compat_get_entry(const char *id);

/**
 * @brief Get all entries for a category
 *
 * @param category Category to query
 * @param entries Output array (caller provides)
 * @param max_entries Maximum entries to return
 * @return Number of entries returned
 */
size_t compat_get_by_category(compat_category_t category,
                              const compat_entry_t **entries,
                              size_t max_entries);

/**
 * @brief Get all entries for a feature
 *
 * @param feature Feature name to query
 * @param entries Output array (caller provides)
 * @param max_entries Maximum entries to return
 * @return Number of entries returned
 */
size_t compat_get_by_feature(const char *feature,
                             const compat_entry_t **entries,
                             size_t max_entries);

/**
 * @brief Get total number of entries in database
 *
 * @return Number of entries
 */
size_t compat_get_entry_count(void);

/**
 * @brief Iterate over all entries
 *
 * Calls the callback function for each entry in the database.
 *
 * @param callback Function to call for each entry
 * @param user_data User data passed to callback
 */
void compat_foreach_entry(void (*callback)(const compat_entry_t *entry,
                                           void *user_data),
                          void *user_data);

/* ============================================================================
 * Portability Checking
 * ============================================================================ */

/**
 * @brief Check if a construct is portable to a target shell
 *
 * Checks if the given shell construct will work correctly in the target
 * shell mode.
 *
 * @param construct The construct to check (e.g., "echo -e", "${arr[0]}")
 * @param target Target shell mode to check against
 * @param result Output result (optional, may be NULL)
 * @return true if portable, false otherwise
 */
bool compat_is_portable(const char *construct, shell_mode_t target,
                        compat_result_t *result);

/**
 * @brief Check a line of script for compatibility issues
 *
 * Scans a line of shell script for constructs that may not be portable
 * to the target shell.
 *
 * @param line Line to check
 * @param target Target shell mode
 * @param results Output array for results
 * @param max_results Maximum results to return
 * @return Number of issues found
 */
size_t compat_check_line(const char *line, shell_mode_t target,
                         compat_result_t *results, size_t max_results);

/**
 * @brief Check a script for compatibility issues
 *
 * Scans an entire script for constructs that may not be portable to the
 * target shell.
 *
 * @param script Script content to check
 * @param target Target shell mode
 * @param results Output array for results
 * @param max_results Maximum results to return
 * @return Number of issues found
 */
size_t compat_check_script(const char *script, shell_mode_t target,
                           compat_result_t *results, size_t max_results);

/* ============================================================================
 * Strict Mode Support
 * ============================================================================ */

/**
 * @brief Enable strict compatibility mode
 *
 * When strict mode is enabled, compatibility warnings are treated as errors.
 *
 * @param strict true to enable strict mode
 */
void compat_set_strict(bool strict);

/**
 * @brief Check if strict mode is enabled
 *
 * @return true if strict mode is enabled
 */
bool compat_is_strict(void);

/**
 * @brief Get effective severity for an entry
 *
 * Returns the effective severity, accounting for strict mode.
 * In strict mode, warnings are elevated to errors.
 *
 * @param entry Entry to check
 * @return Effective severity
 */
compat_severity_t compat_effective_severity(const compat_entry_t *entry);

/* ============================================================================
 * Target Shell Support
 * ============================================================================ */

/**
 * @brief Set the target shell for compatibility checking
 *
 * Sets the default target shell used when checking for compatibility.
 *
 * @param target Target shell mode
 */
void compat_set_target(shell_mode_t target);

/**
 * @brief Get the current target shell
 *
 * @return Current target shell mode
 */
shell_mode_t compat_get_target(void);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Get category name as string
 *
 * @param category Category to get name for
 * @return Category name string (static, do not free)
 */
const char *compat_category_name(compat_category_t category);

/**
 * @brief Get severity name as string
 *
 * @param severity Severity to get name for
 * @return Severity name string (static, do not free)
 */
const char *compat_severity_name(compat_severity_t severity);

/**
 * @brief Parse a category name string
 *
 * @param name Category name to parse
 * @param category Output parameter for parsed category
 * @return true on success, false if name is not recognized
 */
bool compat_category_parse(const char *name, compat_category_t *category);

/**
 * @brief Parse a severity name string
 *
 * @param name Severity name to parse
 * @param severity Output parameter for parsed severity
 * @return true on success, false if name is not recognized
 */
bool compat_severity_parse(const char *name, compat_severity_t *severity);

/**
 * @brief Format a compatibility result as a message
 *
 * Formats a compatibility result into a human-readable message suitable
 * for display to the user.
 *
 * @param result Result to format
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Number of characters written
 */
int compat_format_result(const compat_result_t *result, char *buffer,
                         size_t size);

/* ============================================================================
 * Debugging
 * ============================================================================ */

/**
 * @brief Print database statistics
 *
 * Outputs statistics about the loaded compatibility database to stderr.
 */
void compat_debug_print_stats(void);

/**
 * @brief Print an entry for debugging
 *
 * Outputs the full contents of an entry to stderr.
 *
 * @param entry Entry to print
 */
void compat_debug_print_entry(const compat_entry_t *entry);

#endif /* COMPAT_H */
