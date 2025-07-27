/**
 * Auto-Correction System Implementation for Lusush Shell
 *
 * Phase 3 Target 1: Intelligent command spell checking and correction
 * suggestions Provides "Did you mean...?" functionality with user confirmation
 * prompts Integrates with existing completion algorithms and configuration
 * system
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "../include/autocorrect.h"

#include "../include/builtins.h"
#include "../include/completion.h"
#include "../include/config.h"
#include "../include/executor.h"
#include "../include/history.h"
#include "../include/linenoise_replacement.h"
#include "../include/lusush.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

// Global auto-correction configuration
static autocorrect_config_t autocorrect_config = {0};

// Statistics tracking
static struct {
    int corrections_offered;
    int corrections_accepted;
    int commands_learned;
} autocorrect_stats = {0};

// Debug flag
static bool debug_enabled = false;

// Learned commands cache (simple array for now)
#define MAX_LEARNED_COMMANDS 1000
static char *learned_commands[MAX_LEARNED_COMMANDS];
static int learned_commands_count = 0;

// Internal helper functions
static int min3(int a, int b, int c);
static double jaro_similarity(const char *s1, const char *s2);
static void sort_corrections_by_score(correction_t *corrections, int count);
static bool is_executable_file(const char *path);

/**
 * Initialize auto-correction system
 */
int autocorrect_init(void) {
    // Set default configuration
    autocorrect_get_default_config(&autocorrect_config);

    // Initialize learned commands array
    for (int i = 0; i < MAX_LEARNED_COMMANDS; i++) {
        learned_commands[i] = NULL;
    }
    learned_commands_count = 0;

    // Reset statistics
    autocorrect_reset_stats();

    if (debug_enabled) {
        printf("DEBUG: Auto-correction system initialized\n");
    }

    return 0;
}

/**
 * Cleanup auto-correction system
 */
void autocorrect_cleanup(void) {
    // Free learned commands
    for (int i = 0; i < learned_commands_count; i++) {
        if (learned_commands[i]) {
            free(learned_commands[i]);
            learned_commands[i] = NULL;
        }
    }
    learned_commands_count = 0;

    if (debug_enabled) {
        printf("DEBUG: Auto-correction system cleaned up\n");
    }
}

/**
 * Load auto-correction configuration
 */
int autocorrect_load_config(const autocorrect_config_t *config) {
    if (!config) {
        return -1;
    }

    if (!autocorrect_validate_config(config)) {
        return -1;
    }

    autocorrect_config = *config;

    if (debug_enabled) {
        printf("DEBUG: Auto-correction config loaded - enabled: %s, "
               "max_suggestions: %d\n",
               autocorrect_config.enabled ? "true" : "false",
               autocorrect_config.max_suggestions);
    }

    return 0;
}

/**
 * Check if auto-correction is enabled
 */
bool autocorrect_is_enabled(void) { return autocorrect_config.enabled; }

/**
 * Find correction suggestions for a misspelled command
 */
int autocorrect_find_suggestions(executor_t *executor, const char *command,
                                 correction_results_t *results) {
    if (!command || !results || !autocorrect_config.enabled) {
        return 0;
    }

    // Initialize results
    memset(results, 0, sizeof(correction_results_t));
    results->original_command = strdup(command);

    // Temporary array to collect all suggestions
    correction_t
        temp_suggestions[MAX_CORRECTIONS * 4]; // Extra space for sorting
    int temp_count = 0;

    // Find builtin suggestions
    if (autocorrect_config.correct_builtins) {
        int builtin_count = autocorrect_suggest_builtins(
            command, &temp_suggestions[temp_count],
            MAX_CORRECTIONS - temp_count, autocorrect_config.case_sensitive);
        temp_count += builtin_count;

        if (debug_enabled && builtin_count > 0) {
            printf("DEBUG: Found %d builtin suggestions for '%s'\n",
                   builtin_count, command);
        }
    }

    // Find function suggestions
    if (executor && temp_count < MAX_CORRECTIONS) {
        int function_count = autocorrect_suggest_functions(
            executor, command, &temp_suggestions[temp_count],
            MAX_CORRECTIONS - temp_count, autocorrect_config.case_sensitive);
        temp_count += function_count;

        if (debug_enabled && function_count > 0) {
            printf("DEBUG: Found %d function suggestions for '%s'\n",
                   function_count, command);
        }
    }

    // Find PATH command suggestions
    if (autocorrect_config.correct_external && temp_count < MAX_CORRECTIONS) {
        int path_count = autocorrect_suggest_path_commands(
            command, &temp_suggestions[temp_count],
            MAX_CORRECTIONS - temp_count, autocorrect_config.case_sensitive);
        temp_count += path_count;

        if (debug_enabled && path_count > 0) {
            printf("DEBUG: Found %d PATH suggestions for '%s'\n", path_count,
                   command);
        }
    }

    // Find history suggestions
    if (autocorrect_config.learn_from_history && temp_count < MAX_CORRECTIONS) {
        int history_count = autocorrect_suggest_from_history(
            command, &temp_suggestions[temp_count],
            MAX_CORRECTIONS - temp_count, autocorrect_config.case_sensitive);
        temp_count += history_count;

        if (debug_enabled && history_count > 0) {
            printf("DEBUG: Found %d history suggestions for '%s'\n",
                   history_count, command);
        }
    }

    // Sort suggestions by score (highest first)
    sort_corrections_by_score(temp_suggestions, temp_count);

    // Copy top suggestions to results, filtering by threshold and max count
    int max_to_copy = (autocorrect_config.max_suggestions < MAX_CORRECTIONS)
                          ? autocorrect_config.max_suggestions
                          : MAX_CORRECTIONS;

    for (int i = 0; i < temp_count && results->count < max_to_copy; i++) {
        if (temp_suggestions[i].score >=
            autocorrect_config.similarity_threshold) {
            results->suggestions[results->count] = temp_suggestions[i];
            results->count++;
        }
    }

    // Update statistics
    if (results->count > 0) {
        autocorrect_stats.corrections_offered++;
    }

    if (debug_enabled) {
        printf("DEBUG: Returning %d suggestions for '%s'\n", results->count,
               command);
    }

    return results->count;
}

/**
 * Present correction suggestions to user and get selection
 */
bool autocorrect_prompt_user(const correction_results_t *results,
                             char *selected_command) {
    if (!results || !selected_command ||
        !autocorrect_config.interactive_prompts) {
        return false;
    }

    if (results->count == 0) {
        return false;
    }

    printf("lusush: command not found: %s\n", results->original_command);
    printf("\nDid you mean:\n");

    for (int i = 0; i < results->count; i++) {
        printf("  %d) %s", i + 1, results->suggestions[i].command);
        if (debug_enabled) {
            printf(" (score: %d, source: %s)", results->suggestions[i].score,
                   results->suggestions[i].source);
        }
        printf("\n");
    }

    printf("  0) Cancel\n");
    printf("\nSelect (0-%d): ", results->count);
    fflush(stdout);

    char input[10];
    if (!fgets(input, sizeof(input), stdin)) {
        return false;
    }

    int choice = atoi(input);
    if (choice >= 1 && choice <= results->count) {
        strncpy(selected_command, results->suggestions[choice - 1].command,
                MAX_COMMAND_LENGTH - 1);
        selected_command[MAX_COMMAND_LENGTH - 1] = '\0';

        autocorrect_stats.corrections_accepted++;

        if (debug_enabled) {
            printf("DEBUG: User selected correction: '%s'\n", selected_command);
        }

        return true;
    }

    return false;
}

/**
 * Free memory allocated in correction results
 */
void autocorrect_free_results(correction_results_t *results) {
    if (!results) {
        return;
    }

    if (results->original_command) {
        free(results->original_command);
        results->original_command = NULL;
    }

    for (int i = 0; i < results->count; i++) {
        if (results->suggestions[i].command) {
            free(results->suggestions[i].command);
            results->suggestions[i].command = NULL;
        }
    }

    results->count = 0;
}

/**
 * Calculate similarity score between two commands
 */
int autocorrect_similarity_score(const char *command1, const char *command2,
                                 bool case_sensitive) {
    if (!command1 || !command2) {
        return 0;
    }

    // If commands are identical, return perfect score
    int cmp_result;
    if (case_sensitive) {
        cmp_result = strcmp(command1, command2);
    } else {
        // Manual case-insensitive comparison
        cmp_result = 0;
        const char *s1 = command1, *s2 = command2;
        while (*s1 && *s2) {
            char c1 = tolower(*s1);
            char c2 = tolower(*s2);
            if (c1 != c2) {
                cmp_result = c1 - c2;
                break;
            }
            s1++;
            s2++;
        }
        if (cmp_result == 0) {
            cmp_result = *s1 - *s2;
        }
    }
    if (cmp_result == 0) {
        return 100;
    }

    int len1 = strlen(command1);
    int len2 = strlen(command2);

    // If one string is empty, return 0
    if (len1 == 0 || len2 == 0) {
        return 0;
    }

    // Combine multiple similarity algorithms for better results

    // 1. Levenshtein distance (edit distance)
    int edit_distance = autocorrect_levenshtein_distance(command1, command2);
    int max_len = (len1 > len2) ? len1 : len2;
    int levenshtein_score = ((max_len - edit_distance) * 100) / max_len;
    if (levenshtein_score < 0) {
        levenshtein_score = 0;
    }

    // 2. Jaro-Winkler similarity
    int jaro_score = autocorrect_jaro_winkler_score(command1, command2);

    // 3. Common prefix bonus
    int prefix_len =
        autocorrect_common_prefix_length(command1, command2, case_sensitive);
    int prefix_score = (prefix_len * 100) / ((len1 + len2) / 2);
    if (prefix_score > 100) {
        prefix_score = 100;
    }

    // 4. Subsequence matching (fuzzy)
    int subseq_score =
        autocorrect_subsequence_score(command1, command2, case_sensitive);

    // Weighted combination of all scores
    int final_score = (levenshtein_score * 4 + jaro_score * 3 +
                       prefix_score * 2 + subseq_score * 1) /
                      10;

    if (final_score > 100) {
        final_score = 100;
    }
    if (final_score < 0) {
        final_score = 0;
    }

    return final_score;
}

/**
 * Add command to learning history
 */
void autocorrect_learn_command(const char *command) {
    if (!command || !autocorrect_config.learn_from_history) {
        return;
    }

    // Check if command already exists
    for (int i = 0; i < learned_commands_count; i++) {
        if (learned_commands[i] && strcmp(learned_commands[i], command) == 0) {
            return; // Already learned
        }
    }

    // Add new command if there's space
    if (learned_commands_count < MAX_LEARNED_COMMANDS) {
        learned_commands[learned_commands_count] = strdup(command);
        if (learned_commands[learned_commands_count]) {
            learned_commands_count++;
            autocorrect_stats.commands_learned++;

            if (debug_enabled) {
                printf("DEBUG: Learned command: '%s'\n", command);
            }
        }
    }
}

/**
 * Check if a command exists in the system
 */
bool autocorrect_command_exists(executor_t *executor, const char *command) {
    if (!command) {
        return false;
    }

    // Check if it's a builtin (only when compiled with full shell)
#ifndef AUTOCORRECT_STANDALONE_TEST
    if (is_builtin(command)) {
        return true;
    }
#endif

    // Check if it's a function (requires executor context)
    if (executor) {
        // This would need to be implemented with access to executor's function
        // table For now, we'll skip function checking
    }

    // Check PATH
    char *path = getenv("PATH");
    if (!path) {
        return false;
    }

    char *path_copy = strdup(path);
    if (!path_copy) {
        return false;
    }

    char *dir = strtok(path_copy, ":");
    while (dir) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);

        if (is_executable_file(full_path)) {
            free(path_copy);
            return true;
        }

        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return false;
}

/**
 * Calculate Levenshtein distance
 */
int autocorrect_levenshtein_distance(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    // Create matrix
    int **matrix = malloc((len1 + 1) * sizeof(int *));
    if (!matrix) {
        return len1 + len2; // Worst case
    }

    for (int i = 0; i <= len1; i++) {
        matrix[i] = malloc((len2 + 1) * sizeof(int));
        if (!matrix[i]) {
            // Cleanup and return worst case
            for (int j = 0; j < i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            return len1 + len2;
        }
    }

    // Initialize matrix
    for (int i = 0; i <= len1; i++) {
        matrix[i][0] = i;
    }
    for (int j = 0; j <= len2; j++) {
        matrix[0][j] = j;
    }

    // Fill matrix
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (tolower(s1[i - 1]) == tolower(s2[j - 1])) ? 0 : 1;
            matrix[i][j] = min3(matrix[i - 1][j] + 1,       // deletion
                                matrix[i][j - 1] + 1,       // insertion
                                matrix[i - 1][j - 1] + cost // substitution
            );
        }
    }

    int result = matrix[len1][len2];

    // Cleanup
    for (int i = 0; i <= len1; i++) {
        free(matrix[i]);
    }
    free(matrix);

    return result;
}

/**
 * Calculate Jaro-Winkler similarity score
 */
int autocorrect_jaro_winkler_score(const char *s1, const char *s2) {
    double jaro = jaro_similarity(s1, s2);

    // Jaro-Winkler adds prefix bonus
    int prefix_len = autocorrect_common_prefix_length(s1, s2, false);
    if (prefix_len > 4) {
        prefix_len = 4; // Max prefix bonus is 4 chars
    }

    double jaro_winkler = jaro + (0.1 * prefix_len * (1 - jaro));

    return (int)(jaro_winkler * 100);
}

/**
 * Calculate common prefix length
 */
int autocorrect_common_prefix_length(const char *s1, const char *s2,
                                     bool case_sensitive) {
    int len = 0;
    while (s1[len] && s2[len]) {
        char c1 = case_sensitive ? s1[len] : tolower(s1[len]);
        char c2 = case_sensitive ? s2[len] : tolower(s2[len]);
        if (c1 != c2) {
            break;
        }
        len++;
    }
    return len;
}

/**
 * Check subsequence match quality
 */
int autocorrect_subsequence_score(const char *pattern, const char *text,
                                  bool case_sensitive) {
    int pattern_len = strlen(pattern);
    int text_len = strlen(text);

    if (pattern_len == 0) {
        return 100;
    }
    if (text_len == 0) {
        return 0;
    }

    int matches = 0;
    int text_idx = 0;

    for (int p = 0; p < pattern_len && text_idx < text_len; p++) {
        char pc = case_sensitive ? pattern[p] : tolower(pattern[p]);

        for (; text_idx < text_len; text_idx++) {
            char tc = case_sensitive ? text[text_idx] : tolower(text[text_idx]);
            if (pc == tc) {
                matches++;
                text_idx++;
                break;
            }
        }
    }

    return (matches * 100) / pattern_len;
}

/**
 * Find builtin command suggestions
 */
int autocorrect_suggest_builtins(const char *command, correction_t *suggestions,
                                 int max_suggestions, bool case_sensitive) {
#ifdef AUTOCORRECT_STANDALONE_TEST
    // For standalone testing, skip suggestions to avoid dependencies
    (void)command;
    (void)suggestions;
    (void)max_suggestions;
    (void)case_sensitive;
    return 0;
#else
    int count = 0;

    // Get list of builtin commands
    const char *builtins[] = {
        "cd",     "pwd",   "echo",   "printf",  "test",     "[",
        "true",   "false", ":",      ".",       "break",    "continue",
        "return", "exit",  "export", "set",     "unset",    "shift",
        "eval",   "exec",  "hash",   "type",    "ulimit",   "umask",
        "times",  "trap",  "wait",   "getopts", "readonly", "config"};

    int builtin_count = sizeof(builtins) / sizeof(builtins[0]);

    for (int i = 0; i < builtin_count && count < max_suggestions; i++) {
        int score =
            autocorrect_similarity_score(command, builtins[i], case_sensitive);
        if (score >= MIN_SIMILARITY_SCORE) {
            suggestions[count].command = strdup(builtins[i]);
            suggestions[count].score = score;
            suggestions[count].source = "builtin";
            count++;
        }
    }

    return count;
#endif
}

/**
 * Find function suggestions (stub for now)
 */
int autocorrect_suggest_functions(executor_t *executor, const char *command,
                                  correction_t *suggestions,
                                  int max_suggestions, bool case_sensitive) {
    // This would require access to executor's function table
    // For Phase 3 Target 1, we'll focus on builtins and PATH commands
    (void)executor;
    (void)command;
    (void)suggestions;
    (void)max_suggestions;
    (void)case_sensitive;
    return 0;
}

/**
 * Find PATH command suggestions
 */
int autocorrect_suggest_path_commands(const char *command,
                                      correction_t *suggestions,
                                      int max_suggestions,
                                      bool case_sensitive) {
    int count = 0;
    char *path = getenv("PATH");

    if (!path) {
        return 0;
    }

    char *path_copy = strdup(path);
    if (!path_copy) {
        return 0;
    }

    char *dir = strtok(path_copy, ":");
    while (dir && count < max_suggestions) {
        DIR *dp = opendir(dir);
        if (!dp) {
            dir = strtok(NULL, ":");
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dp)) && count < max_suggestions) {
            if (entry->d_name[0] == '.') {
                continue; // Skip hidden files
            }

            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, entry->d_name);

            if (is_executable_file(full_path)) {
                int score = autocorrect_similarity_score(command, entry->d_name,
                                                         case_sensitive);
                if (score >= MIN_SIMILARITY_SCORE) {
                    suggestions[count].command = strdup(entry->d_name);
                    suggestions[count].score = score;
                    suggestions[count].source = "path";
                    count++;
                }
            }
        }

        closedir(dp);
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return count;
}

/**
 * Find history-based suggestions
 */
int autocorrect_suggest_from_history(const char *command,
                                     correction_t *suggestions,
                                     int max_suggestions, bool case_sensitive) {
    int count = 0;

    // Check learned commands
    for (int i = 0; i < learned_commands_count && count < max_suggestions;
         i++) {
        if (learned_commands[i]) {
            int score = autocorrect_similarity_score(
                command, learned_commands[i], case_sensitive);
            if (score >= MIN_SIMILARITY_SCORE) {
                suggestions[count].command = strdup(learned_commands[i]);
                suggestions[count].score = score;
                suggestions[count].source = "history";
                count++;
            }
        }
    }

    return count;
}

/**
 * Get default configuration
 */
void autocorrect_get_default_config(autocorrect_config_t *config) {
    if (!config) {
        return;
    }

    config->enabled = true;
    config->max_suggestions = 3;
    config->similarity_threshold = MIN_SIMILARITY_SCORE;
    config->interactive_prompts = true;
    config->learn_from_history = true;
    config->correct_builtins = true;
    config->correct_external = true;
    config->case_sensitive = false;
}

/**
 * Validate configuration
 */
bool autocorrect_validate_config(const autocorrect_config_t *config) {
    if (!config) {
        return false;
    }

    if (config->max_suggestions < 1 ||
        config->max_suggestions > MAX_CORRECTIONS) {
        return false;
    }

    if (config->similarity_threshold < 0 ||
        config->similarity_threshold > 100) {
        return false;
    }

    return true;
}

/**
 * Apply configuration changes
 */
int autocorrect_apply_config(const autocorrect_config_t *config) {
    return autocorrect_load_config(config);
}

/**
 * Get statistics
 */
void autocorrect_get_stats(int *corrections_offered, int *corrections_accepted,
                           int *commands_learned) {
    if (corrections_offered) {
        *corrections_offered = autocorrect_stats.corrections_offered;
    }
    if (corrections_accepted) {
        *corrections_accepted = autocorrect_stats.corrections_accepted;
    }
    if (commands_learned) {
        *commands_learned = autocorrect_stats.commands_learned;
    }
}

/**
 * Reset statistics
 */
void autocorrect_reset_stats(void) {
    autocorrect_stats.corrections_offered = 0;
    autocorrect_stats.corrections_accepted = 0;
    autocorrect_stats.commands_learned = 0;
}

/**
 * Set debug mode
 */
void autocorrect_set_debug(bool enabled) { debug_enabled = enabled; }

// Internal helper functions

static int min3(int a, int b, int c) {
    int min_ab = (a < b) ? a : b;
    return (min_ab < c) ? min_ab : c;
}

static double jaro_similarity(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    if (len1 == 0 && len2 == 0) {
        return 1.0;
    }
    if (len1 == 0 || len2 == 0) {
        return 0.0;
    }

    int match_window = ((len1 > len2) ? len1 : len2) / 2 - 1;
    if (match_window < 0) {
        match_window = 0;
    }

    bool *s1_matches = calloc(len1, sizeof(bool));
    bool *s2_matches = calloc(len2, sizeof(bool));

    if (!s1_matches || !s2_matches) {
        free(s1_matches);
        free(s2_matches);
        return 0.0;
    }

    int matches = 0;

    // Find matches
    for (int i = 0; i < len1; i++) {
        int start = (i - match_window > 0) ? i - match_window : 0;
        int end = (i + match_window < len2) ? i + match_window : len2 - 1;

        for (int j = start; j <= end; j++) {
            if (s2_matches[j] || tolower(s1[i]) != tolower(s2[j])) {
                continue;
            }
            s1_matches[i] = s2_matches[j] = true;
            matches++;
            break;
        }
    }

    if (matches == 0) {
        free(s1_matches);
        free(s2_matches);
        return 0.0;
    }

    // Count transpositions
    int transpositions = 0;
    int k = 0;
    for (int i = 0; i < len1; i++) {
        if (!s1_matches[i]) {
            continue;
        }
        while (!s2_matches[k]) {
            k++;
        }
        if (tolower(s1[i]) != tolower(s2[k])) {
            transpositions++;
        }
        k++;
    }

    free(s1_matches);
    free(s2_matches);

    double jaro = ((double)matches / len1 + (double)matches / len2 +
                   (double)(matches - transpositions / 2) / matches) /
                  3.0;

    return jaro;
}

static void sort_corrections_by_score(correction_t *corrections, int count) {
    // Simple bubble sort by score (descending)
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (corrections[j].score < corrections[j + 1].score) {
                correction_t temp = corrections[j];
                corrections[j] = corrections[j + 1];
                corrections[j + 1] = temp;
            }
        }
    }
}

static bool is_executable_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }

    return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
}
