/**
 * Auto-Correction System for Lusush Shell
 *
 * Phase 3 Target 1: Intelligent command spell checking and correction
 * suggestions Provides "Did you mean...?" functionality with user confirmation
 * prompts Integrates with existing completion algorithms and configuration
 * system
 */

#ifndef AUTOCORRECT_H
#define AUTOCORRECT_H

#include <stdbool.h>
#include <stddef.h>

// Maximum number of correction suggestions to offer
#define MAX_CORRECTIONS 5

// Minimum similarity score to consider for suggestions (0-100)
#define MIN_SIMILARITY_SCORE 40

// Maximum command length to process for corrections
#define MAX_COMMAND_LENGTH 256

// Auto-correction configuration structure
typedef struct {
    bool enabled;             // Enable/disable auto-correction
    int max_suggestions;      // Maximum number of suggestions (1-5)
    int similarity_threshold; // Minimum similarity score (0-100)
    bool interactive_prompts; // Show "Did you mean?" prompts
    bool learn_from_history;  // Learn from command history
    bool correct_builtins;    // Suggest builtin corrections
    bool correct_external;    // Suggest external command corrections
    bool case_sensitive;      // Case-sensitive matching
} autocorrect_config_t;

// Correction suggestion structure
typedef struct {
    char *command;      // Suggested command
    int score;          // Similarity score (0-100)
    const char *source; // Source: "builtin", "history", "path", "function"
} correction_t;

// Correction results structure
typedef struct {
    correction_t suggestions[MAX_CORRECTIONS]; // Array of suggestions
    int count;                                 // Number of suggestions found
    char *original_command;                    // Original misspelled command
} correction_results_t;

// Forward declarations for executor integration
typedef struct executor executor_t;

/**
 * Initialize auto-correction system
 * Must be called once during shell startup
 * @return 0 on success, non-zero on error
 */
int autocorrect_init(void);

/**
 * Cleanup auto-correction system
 * Called during shell shutdown
 */
void autocorrect_cleanup(void);

/**
 * Load auto-correction configuration from config system
 * @param config Configuration values structure
 * @return 0 on success, non-zero on error
 */
int autocorrect_load_config(const autocorrect_config_t *config);

/**
 * Check if auto-correction is enabled
 * @return true if enabled, false otherwise
 */
bool autocorrect_is_enabled(void);

/**
 * Find correction suggestions for a misspelled command
 * @param executor Executor context for accessing functions/variables
 * @param command The misspelled command to correct
 * @param results Output structure to store correction results
 * @return Number of suggestions found (0 if none)
 */
int autocorrect_find_suggestions(executor_t *executor, const char *command,
                                 correction_results_t *results);

/**
 * Present correction suggestions to user and get selection
 * @param results Correction results from autocorrect_find_suggestions
 * @param selected_command Output buffer for selected command
 * (MAX_COMMAND_LENGTH)
 * @return true if user selected a correction, false if cancelled/declined
 */
bool autocorrect_prompt_user(const correction_results_t *results,
                             char *selected_command);

/**
 * Free memory allocated in correction results
 * @param results Results structure to clean up
 */
void autocorrect_free_results(correction_results_t *results);

/**
 * Calculate similarity score between two commands using multiple algorithms
 * @param command1 First command
 * @param command2 Second command
 * @param case_sensitive Whether to use case-sensitive comparison
 * @return Similarity score (0-100, higher is more similar)
 */
int autocorrect_similarity_score(const char *command1, const char *command2,
                                 bool case_sensitive);

/**
 * Add command to learning history for future corrections
 * @param command Command that was successfully executed
 */
void autocorrect_learn_command(const char *command);

/**
 * Check if a command exists in the system (builtin, function, or PATH)
 * @param executor Executor context
 * @param command Command to check
 * @return true if command exists, false otherwise
 */
bool autocorrect_command_exists(executor_t *executor, const char *command);

// Similarity algorithm functions (internal)

/**
 * Calculate Levenshtein distance between two strings
 * @param s1 First string
 * @param s2 Second string
 * @return Edit distance (lower is more similar)
 */
int autocorrect_levenshtein_distance(const char *s1, const char *s2);

/**
 * Calculate Jaro-Winkler similarity score
 * @param s1 First string
 * @param s2 Second string
 * @return Similarity score (0-100)
 */
int autocorrect_jaro_winkler_score(const char *s1, const char *s2);

/**
 * Calculate common prefix length
 * @param s1 First string
 * @param s2 Second string
 * @param case_sensitive Whether to use case-sensitive comparison
 * @return Length of common prefix
 */
int autocorrect_common_prefix_length(const char *s1, const char *s2,
                                     bool case_sensitive);

/**
 * Check if one string is a subsequence of another (fuzzy matching)
 * @param pattern Pattern string (usually shorter)
 * @param text Text to search in
 * @param case_sensitive Whether to use case-sensitive comparison
 * @return Score based on subsequence match quality (0-100)
 */
int autocorrect_subsequence_score(const char *pattern, const char *text,
                                  bool case_sensitive);

// Correction source functions

/**
 * Find builtin command suggestions
 * @param command Misspelled command
 * @param suggestions Output array
 * @param max_suggestions Maximum suggestions to return
 * @param case_sensitive Case-sensitive matching
 * @return Number of suggestions found
 */
int autocorrect_suggest_builtins(const char *command, correction_t *suggestions,
                                 int max_suggestions, bool case_sensitive);

/**
 * Find function suggestions from executor context
 * @param executor Executor context
 * @param command Misspelled command
 * @param suggestions Output array
 * @param max_suggestions Maximum suggestions to return
 * @param case_sensitive Case-sensitive matching
 * @return Number of suggestions found
 */
int autocorrect_suggest_functions(executor_t *executor, const char *command,
                                  correction_t *suggestions,
                                  int max_suggestions, bool case_sensitive);

/**
 * Find PATH command suggestions
 * @param command Misspelled command
 * @param suggestions Output array
 * @param max_suggestions Maximum suggestions to return
 * @param case_sensitive Case-sensitive matching
 * @return Number of suggestions found
 */
int autocorrect_suggest_path_commands(const char *command,
                                      correction_t *suggestions,
                                      int max_suggestions, bool case_sensitive);

/**
 * Find history-based suggestions
 * @param command Misspelled command
 * @param suggestions Output array
 * @param max_suggestions Maximum suggestions to return
 * @param case_sensitive Case-sensitive matching
 * @return Number of suggestions found
 */
int autocorrect_suggest_from_history(const char *command,
                                     correction_t *suggestions,
                                     int max_suggestions, bool case_sensitive);

// Configuration helper functions

/**
 * Get default auto-correction configuration
 * @param config Output configuration structure
 */
void autocorrect_get_default_config(autocorrect_config_t *config);

/**
 * Validate auto-correction configuration values
 * @param config Configuration to validate
 * @return true if valid, false otherwise
 */
bool autocorrect_validate_config(const autocorrect_config_t *config);

/**
 * Apply auto-correction configuration changes at runtime
 * @param config New configuration values
 * @return 0 on success, non-zero on error
 */
int autocorrect_apply_config(const autocorrect_config_t *config);

// Debug and statistics functions

/**
 * Get auto-correction statistics
 * @param corrections_offered Total corrections offered
 * @param corrections_accepted Total corrections accepted by user
 * @param commands_learned Total commands learned from history
 */
void autocorrect_get_stats(int *corrections_offered, int *corrections_accepted,
                           int *commands_learned);

/**
 * Reset auto-correction statistics
 */
void autocorrect_reset_stats(void);

/**
 * Enable/disable debug output for auto-correction
 * @param enabled Whether to enable debug output
 */
void autocorrect_set_debug(bool enabled);

#endif // AUTOCORRECT_H
