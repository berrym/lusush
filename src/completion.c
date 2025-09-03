#include "../include/completion.h"

#include "../include/alias.h"
#include "../include/builtins.h"
#include "../include/config.h"
#include "../include/libhashtable/ht.h"
#include "../include/readline_integration.h"
#include "../include/lusush.h"
#include "../include/network.h"
#include "../include/symtable.h"
#include "../include/termcap.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

// ============================================================================
// COMPLETION CACHE
// ============================================================================

#define COMPLETION_CACHE_SIZE 8
#define COMPLETION_CACHE_TTL 30  // seconds

typedef struct {
    char *query;
    lusush_completions_t completions;
    time_t timestamp;
} completion_cache_entry_t;

static completion_cache_entry_t completion_cache[COMPLETION_CACHE_SIZE] = {0};
static int cache_index = 0;

/**
 * Fuzzy matching algorithm for enhanced completion
 * Returns a score (0-100) indicating how well the candidate matches the pattern
 */
static int fuzzy_match_score(const char *pattern, const char *candidate) {
    if (!pattern || !candidate) {
        return 0;
    }

    int pattern_len = strlen(pattern);
    int candidate_len = strlen(candidate);

    if (pattern_len == 0) {
        return 100; // Empty pattern matches everything
    }

    if (candidate_len == 0) {
        return 0; // Empty candidate matches nothing
    }

    // Simple prefix matching gets highest score
    if (strncmp(pattern, candidate, pattern_len) == 0) {
        return 95;
    }

    // Case-insensitive prefix matching
    int case_match = 1;
    for (int i = 0; i < pattern_len; i++) {
        if (tolower(pattern[i]) != tolower(candidate[i])) {
            case_match = 0;
            break;
        }
    }
    if (case_match) {
        return 90;
    }

    // Subsequence matching - all pattern chars must appear in order
    int matches = 0;
    int pattern_idx = 0;
    int consecutive_matches = 0;
    int max_consecutive = 0;

    for (int i = 0; i < candidate_len && pattern_idx < pattern_len; i++) {
        if (tolower(candidate[i]) == tolower(pattern[pattern_idx])) {
            matches++;
            pattern_idx++;
            consecutive_matches++;
            if (consecutive_matches > max_consecutive) {
                max_consecutive = consecutive_matches;
            }
        } else {
            consecutive_matches = 0;
        }
    }

    // All pattern characters must be found
    if (pattern_idx < pattern_len) {
        return 0;
    }

    // Score based on match ratio and consecutive matches
    int match_ratio = (matches * 100) / pattern_len;
    int consecutive_bonus = (max_consecutive * 20) / pattern_len;

    return (match_ratio + consecutive_bonus) / 2;
}

/**
 * Forward declarations
 */

static void prioritize_completions(lusush_completions_t *lc,
                                   const char *pattern);

/**
 * Enhanced completion function that uses fuzzy matching
 */
static void add_fuzzy_completion(lusush_completions_t *lc, const char *pattern,
                                 const char *candidate, const char *suffix,
                                 int min_score) {
    int score = fuzzy_match_score(pattern, candidate);
    if (score >= min_score) {
        add_completion_with_suffix(lc, candidate, suffix);
    }
}

/**
 * Main completion callback for readline
 */
void lusush_completion_callback(const char *buf, lusush_completions_t *lc) {
    if (!buf || !lc) {
        return;
    }

    // Get terminal capabilities for completion optimization
    const terminal_info_t *term_info = termcap_get_info();
    bool has_terminal = term_info && term_info->is_tty;
    int terminal_width = term_info ? term_info->cols : 80;

    int start_pos = 0;
    char *word = get_completion_word(buf, &start_pos);
    if (!word) {
        return;
    }

    // Enhanced completion with fuzzy matching support

    // Check for network command completion first
    char *command = get_first_command(buf);
    if (command && is_network_command(command) &&
        !is_command_position(buf, start_pos)) {
        // Complete network command arguments (SSH hosts, etc.)
        complete_network_command_args_with_context(command, word, lc, buf, start_pos);
        free(command);

        // If we got network completions, we're done
        if (lc->len > 0) {
            free(word);
            return;
        }
    }

    if (command) {
        free(command);
    }

    // Determine what kind of completion to do based on context
    if (is_command_position(buf, start_pos)) {
        // Complete commands (builtins, aliases, executables)
        complete_builtins(word, lc);
        complete_aliases(word, lc);
        complete_commands(word, lc);
    } else if (word[0] == '$') {
        // Complete variables
        complete_variables(word, lc);
    } else {
        // Complete files/directories
        complete_files(word, lc);
    }

    // Always try history completion as a fallback
    if (lc->len == 0) {
        complete_history(word, lc);
    }

    // Prioritize completions by relevance
    if (lc->len > 1) {
        prioritize_completions(lc, word);
    }

    // Limit completions based on terminal width for better display
    if (has_terminal && lc->len > 0) {
        // For narrow terminals, limit number of completions shown
        if (terminal_width < 60 && lc->len > 8) {
            lc->len = 8;
        } else if (terminal_width < 100 && lc->len > 16) {
            lc->len = 16;
        }
    }

    // Completion context is now handled by readline completion
    // No need to call display_completion_context here

    free(word);
}

/**
 * Extract the word being completed from the buffer
 */
char *get_completion_word(const char *buf, int *start_pos) {
    if (!buf) {
        return NULL;
    }

    int len = strlen(buf);
    int end = len;
    int start = end;

    // Find the start of the current word
    while (start > 0 && !isspace(buf[start - 1]) && buf[start - 1] != '|' &&
           buf[start - 1] != ';' && buf[start - 1] != '&' &&
           buf[start - 1] != '(' && buf[start - 1] != ')') {
        start--;
    }

    *start_pos = start;

    if (end <= start) {
        return strdup("");
    }

    char *word = malloc(end - start + 1);
    if (!word) {
        return NULL;
    }

    strncpy(word, buf + start, end - start);
    word[end - start] = '\0';

    return word;
}

/**
 * Check if the current position is a command position (first word or after
 * pipe/;/&)
 */
int is_command_position(const char *buf, int pos) {
    if (pos == 0) {
        return 1; // First word
    }

    // Look backwards for command separators
    for (int i = pos - 1; i >= 0; i--) {
        if (buf[i] == '|' || buf[i] == ';' || buf[i] == '&') {
            return 1;
        }
        if (!isspace(buf[i])) {
            return 0; // Found non-whitespace, so not command position
        }
    }

    return 1; // Only found whitespace, so it's command position
}

/**
 * Complete builtin commands
 */
void complete_builtins(const char *text, lusush_completions_t *lc) {
    if (!text) {
        return;
    }

    size_t text_len = strlen(text);

    // Enhanced fuzzy matching for builtins
    for (size_t i = 0; i < builtins_count; i++) {
        // Exact prefix match gets priority
        if (strncmp(builtins[i].name, text, text_len) == 0) {
            add_completion_with_suffix(lc, builtins[i].name, " ");
        } else {
            // Fuzzy matching fallback
            add_fuzzy_completion(lc, text, builtins[i].name, " ", 30);
        }
    }
}

/**
 * Complete command aliases
 */
void complete_aliases(const char *text, lusush_completions_t *lc) {
    if (!text) {
        return;
    }

    size_t text_len = strlen(text);

    // Get all aliases using hash table enumeration
    extern ht_strstr_t *aliases; // from alias.c
    if (!aliases) {
        return;
    }

    ht_enum_t *aliases_e = ht_strstr_enum_create(aliases);
    if (!aliases_e) {
        return;
    }

    const char *alias_name = NULL, *alias_value = NULL;
    while (ht_strstr_enum_next(aliases_e, &alias_name, &alias_value)) {
        if (alias_name) {
            // Exact prefix match gets priority
            if (strncmp(alias_name, text, text_len) == 0) {
                add_completion_with_suffix(lc, alias_name, " ");
            } else {
                // Try fuzzy matching
                add_fuzzy_completion(lc, text, alias_name, " ", 60);
            }
        }
    }

    ht_strstr_enum_destroy(aliases_e);
}

/**
 * Complete executable commands from PATH
 */
void complete_commands(const char *text, lusush_completions_t *lc) {
    if (!text) {
        return;
    }

    // Check cache for recent identical queries
    time_t now = time(NULL);
    for (int i = 0; i < COMPLETION_CACHE_SIZE; i++) {
        if (completion_cache[i].query && 
            strcmp(completion_cache[i].query, text) == 0 &&
            (now - completion_cache[i].timestamp) < COMPLETION_CACHE_TTL) {
            
            // Copy cached completions
            for (size_t j = 0; j < completion_cache[i].completions.len; j++) {
                add_completion_with_suffix(lc, completion_cache[i].completions.cvec[j], " ");
            }
            return;
        }
    }

    char *path_env = getenv("PATH");
    if (!path_env) {
        return;
    }

    char *path = strdup(path_env);
    if (!path) {
        return;
    }

    size_t text_len = strlen(text);
    char *dir = strtok(path, ":");
    
    // Performance optimization: limit completions for short prefixes
    int max_completions = (text_len <= 2) ? 20 : 100;
    int found_count = 0;

    while (dir && found_count < max_completions) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL && found_count < max_completions) {
                // Quick rejection: check prefix first before any I/O
                if (entry->d_name[0] == '.' || 
                    strncmp(entry->d_name, text, text_len) != 0) {
                    continue;
                }
                
                // Only check executability for matching prefixes
                char full_path[strlen(dir) + strlen(entry->d_name) + 2];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, entry->d_name);

                // Use access() instead of stat() for faster executable check
                if (access(full_path, X_OK) == 0) {
                    // Exact prefix match - no fuzzy matching for performance
                    if (strlen(entry->d_name) > text_len) {
                        add_completion_with_suffix(lc, entry->d_name, " ");
                        found_count++;
                    }
                }
            }
            closedir(d);
        }
        dir = strtok(NULL, ":");
    }

    free(path);

    // Cache the results for future queries (only for short prefixes)
    if (strlen(text) <= 3 && lc->len > 0) {
        // Free old cache entry
        completion_cache_entry_t *entry = &completion_cache[cache_index];
        if (entry->query) {
            free(entry->query);
            lusush_free_completions(&entry->completions);
        }
        
        // Store new cache entry
        entry->query = strdup(text);
        entry->completions.len = 0;
        entry->completions.cvec = NULL;
        entry->timestamp = time(NULL);
        
        // Copy completions to cache
        for (size_t i = 0; i < lc->len; i++) {
            lusush_add_completion(&entry->completions, lc->cvec[i]);
        }
        
        cache_index = (cache_index + 1) % COMPLETION_CACHE_SIZE;
    }
}

/**
 * Initialize completion cache
 */
void lusush_completion_cache_init(void) {
    for (int i = 0; i < COMPLETION_CACHE_SIZE; i++) {
        completion_cache[i].query = NULL;
        completion_cache[i].completions.len = 0;
        completion_cache[i].completions.cvec = NULL;
        completion_cache[i].timestamp = 0;
    }
    cache_index = 0;
}

/**
 * Cleanup completion cache
 */
void lusush_completion_cache_cleanup(void) {
    for (int i = 0; i < COMPLETION_CACHE_SIZE; i++) {
        if (completion_cache[i].query) {
            free(completion_cache[i].query);
            completion_cache[i].query = NULL;
        }
        lusush_free_completions(&completion_cache[i].completions);
        completion_cache[i].timestamp = 0;
    }
    cache_index = 0;
}

/**
 * Complete files and directories
 */
void complete_files(const char *text, lusush_completions_t *lc) {
    if (!text) {
        return;
    }

    char *dir_path = NULL;
    char *file_prefix = NULL;

    // Split text into directory and filename parts
    char *last_slash = strrchr(text, '/');
    if (last_slash) {
        dir_path = strndup(text, last_slash - text + 1);
        file_prefix = strdup(last_slash + 1);
    } else {
        dir_path = strdup("./");
        file_prefix = strdup(text);
    }

    if (!dir_path || !file_prefix) {
        free(dir_path);
        free(file_prefix);
        return;
    }

    DIR *d = opendir(dir_path);
    if (d) {
        struct dirent *entry;
        size_t prefix_len = strlen(file_prefix);

        while ((entry = readdir(d)) != NULL) {
            // Skip hidden files unless user typed a dot
            if (entry->d_name[0] == '.' && file_prefix[0] != '.') {
                continue;
            }

            // Enhanced file completion with fuzzy matching
            bool exact_match =
                (strncmp(entry->d_name, file_prefix, prefix_len) == 0);
            bool fuzzy_match = false;

            if (!exact_match && prefix_len > 0) {
                int score = fuzzy_match_score(file_prefix, entry->d_name);
                fuzzy_match = (score >= 50); // Lower threshold for files
            }

            if (exact_match || fuzzy_match) {
                char *full_completion;
                if (strcmp(dir_path, "./") == 0) {
                    full_completion = strdup(entry->d_name);
                } else {
                    size_t len = strlen(dir_path) + strlen(entry->d_name) + 1;
                    full_completion = malloc(len);
                    if (full_completion) {
                        snprintf(full_completion, len, "%s%s", dir_path,
                                 entry->d_name);
                    }
                }

                if (full_completion) {
                    // Add trailing slash for directories
                    struct stat st;
                    char check_path[strlen(dir_path) + strlen(entry->d_name) +
                                    1];
                    snprintf(check_path, sizeof(check_path), "%s%s", dir_path,
                             entry->d_name);

                    if (stat(check_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                        add_completion_with_suffix(lc, full_completion, "/");
                    } else {
                        add_completion_with_suffix(lc, full_completion, " ");
                    }

                    free(full_completion);
                }
            }
        }
        closedir(d);
    }

    free(dir_path);
    free(file_prefix);
}

/**
 * Complete shell variables
 */
void complete_variables(const char *text, lusush_completions_t *lc) {
    if (!text || text[0] != '$') {
        return;
    }

    const char *var_prefix = text + 1; // Skip the $
    size_t prefix_len = strlen(var_prefix);

    // Complete environment variables
    extern char **environ;
    for (char **env = environ; *env; env++) {
        char *eq = strchr(*env, '=');
        if (eq) {
            size_t var_len = eq - *env;
            // Enhanced variable completion with fuzzy matching
            bool exact_match = (strncmp(*env, var_prefix, prefix_len) == 0 &&
                                var_len > prefix_len);
            bool fuzzy_match = false;

            if (!exact_match && prefix_len > 0) {
                char var_part[var_len + 1];
                strncpy(var_part, *env, var_len);
                var_part[var_len] = '\0';
                int score = fuzzy_match_score(var_prefix, var_part);
                fuzzy_match = (score >= 60);
            }

            if (exact_match || fuzzy_match) {
                char var_name[var_len + 2]; // +2 for $ and null terminator
                var_name[0] = '$';
                strncpy(var_name + 1, *env, var_len);
                var_name[var_len + 1] = '\0';
                lusush_add_completion(lc, var_name);
            }
        }
    }

    // Complete special variables
    const char *special_vars[] = {"$?",   "$$",      "$0",    "$#",     "$1",
                                  "$2",   "$3",      "$4",    "$5",     "$6",
                                  "$7",   "$8",      "$9",    "$HOME",  "$PATH",
                                  "$PWD", "$OLDPWD", "$USER", "$SHELL", NULL};

    for (int i = 0; special_vars[i]; i++) {
        if (strncmp(special_vars[i], text, strlen(text)) == 0) {
            lusush_add_completion(lc, special_vars[i]);
        }
    }
}

/**
 * Complete from history (fallback)
 */
void complete_history(const char *text, lusush_completions_t *lc) {
    if (!text) {
        return;
    }

    size_t text_len = strlen(text);
    size_t i = 0;
    char *line = NULL;

    while ((line = (char*)lusush_history_get(i)) != NULL) {
        if (strncmp(line, text, text_len) == 0 && strlen(line) > text_len) {
            lusush_add_completion(lc, line);
        }
        free(line);
        i++;
    }
}

/**
 * Enhanced completion display with smart formatting
 */

/**
 * Add completion with appropriate suffix and context awareness
 */
void add_completion_with_suffix(lusush_completions_t *lc,
                                const char *completion, const char *suffix) {
    if (!completion || !suffix) {
        return;
    }

    // Avoid duplicate completions
    for (size_t i = 0; i < lc->len; i++) {
        if (strcmp(lc->cvec[i], completion) == 0) {
            return; // Already exists
        }
    }

    size_t total_len = strlen(completion) + strlen(suffix) + 1;
    char *full_completion = malloc(total_len);
    if (full_completion) {
        snprintf(full_completion, total_len, "%s%s", completion, suffix);
        lusush_add_completion(lc, full_completion);
        free(full_completion);
    }
}

/**
 * Extract the first command from buffer for network completion
 */
char *get_first_command(const char *buf) {
    if (!buf) {
        return NULL;
    }

    // Skip leading whitespace
    while (*buf && isspace(*buf)) {
        buf++;
    }

    if (*buf == '\0') {
        return NULL;
    }

    // Find end of first word
    const char *end = buf;
    while (*end && !isspace(*end) && *end != '|' && *end != ';' &&
           *end != '&') {
        end++;
    }

    if (end == buf) {
        return NULL;
    }

    // Copy first command
    size_t len = end - buf;
    char *command = malloc(len + 1);
    if (command) {
        strncpy(command, buf, len);
        command[len] = '\0';
    }

    return command;
}

/**
 * Smart completion prioritization - sorts completions by relevance
 */
static void prioritize_completions(lusush_completions_t *lc,
                                   const char *pattern) {
    if (!lc || lc->len <= 1 || !pattern) {
        return;
    }

    // Simple bubble sort by fuzzy match score (for small lists)
    for (size_t i = 0; i < lc->len - 1; i++) {
        for (size_t j = 0; j < lc->len - i - 1; j++) {
            int score1 = fuzzy_match_score(pattern, lc->cvec[j]);
            int score2 = fuzzy_match_score(pattern, lc->cvec[j + 1]);

            if (score1 < score2) {
                // Swap
                char *temp = lc->cvec[j];
                lc->cvec[j] = lc->cvec[j + 1];
                lc->cvec[j + 1] = temp;
            }
        }
    }
}

/*
 * ==============================================================================
 *                              HINTS SYSTEM
 * ==============================================================================
 */

/**
 * Check if hints should be shown for the current buffer
 * We show hints when:
 * - Buffer is not empty
 * - Buffer doesn't end with a space (user is still typing)
 * - Buffer is reasonably short (to avoid performance issues)
 */
int should_show_hints(const char *buf) {
    // Check if hints are enabled in configuration
    if (!config.hints_enabled) {
        return 0;
    }

    if (!buf || strlen(buf) == 0) {
        return 0;
    }

    size_t len = strlen(buf);
    if (len > 100) { // Don't show hints for very long commands
        return 0;
    }

    // Don't show hints if buffer ends with space (user finished typing word)
    if (buf[len - 1] == ' ') {
        return 0;
    }

    return 1;
}

/**
 * Get the best matching completion for a given text
 * This is used to generate hints by finding the top completion match
 */
char *get_best_completion_match(const char *text) {
    if (!text || strlen(text) == 0) {
        return NULL;
    }

    lusush_completions_t lc = {0, NULL};
    char *best_match = NULL;
    int best_score = 0;

    // Try different completion types and find the best match
    complete_commands(text, &lc);
    complete_builtins(text, &lc);
    complete_files(text, &lc);
    complete_variables(text, &lc);

    // Find the best scoring match
    for (size_t i = 0; i < lc.len; i++) {
        int score = fuzzy_match_score(text, lc.cvec[i]);
        if (score > best_score && score >= 70) { // Minimum threshold
            best_score = score;
            free(best_match);
            best_match = strdup(lc.cvec[i]);
        }
    }

    // Clean up completions
    lusush_free_completions(&lc);

    return best_match;
}

/**
 * Generate hint for commands (builtins and executables)
 */
char *generate_command_hint(const char *buf) {
    if (!buf) {
        return NULL;
    }

    // Extract the current word being typed
    int start_pos = 0;
    char *word = get_completion_word(buf, &start_pos);
    if (!word) {
        return NULL;
    }

    // Only show hints for command position
    if (!is_command_position(buf, start_pos)) {
        free(word);
        return NULL;
    }

    char *match = get_best_completion_match(word);
    free(word);

    if (!match) {
        return NULL;
    }

    // Create hint by showing the rest of the command
    char *current_word = get_completion_word(buf, &start_pos);
    if (!current_word) {
        free(match);
        return NULL;
    }

    size_t current_len = strlen(current_word);
    size_t match_len = strlen(match);

    char *hint = NULL;
    if (match_len > current_len &&
        strncmp(current_word, match, current_len) == 0) {
        hint = strdup(match + current_len);
    }

    free(current_word);
    free(match);
    return hint;
}

/**
 * Generate hint for file/directory completion
 */
char *generate_file_hint(const char *buf) {
    if (!buf) {
        return NULL;
    }

    int start_pos = 0;
    char *word = get_completion_word(buf, &start_pos);
    if (!word) {
        return NULL;
    }

    // Only show file hints for non-command positions
    if (is_command_position(buf, start_pos)) {
        free(word);
        return NULL;
    }

    lusush_completions_t lc = {0, NULL};
    complete_files(word, &lc);

    char *hint = NULL;
    if (lc.len > 0) {
        // Find the best match
        int best_score = 0;
        size_t best_idx = 0;

        for (size_t i = 0; i < lc.len; i++) {
            int score = fuzzy_match_score(word, lc.cvec[i]);
            if (score > best_score) {
                best_score = score;
                best_idx = i;
            }
        }

        if (best_score >= 70) { // Minimum threshold
            size_t word_len = strlen(word);
            size_t match_len = strlen(lc.cvec[best_idx]);

            if (match_len > word_len &&
                strncmp(word, lc.cvec[best_idx], word_len) == 0) {
                hint = strdup(lc.cvec[best_idx] + word_len);
            }
        }
    }

    // Clean up
    lusush_free_completions(&lc);
    free(word);

    return hint;
}

/**
 * Generate hint for variable completion
 */
char *generate_variable_hint(const char *buf) {
    if (!buf) {
        return NULL;
    }

    int start_pos = 0;
    char *word = get_completion_word(buf, &start_pos);
    if (!word) {
        return NULL;
    }

    // Only show variable hints if word starts with $
    if (word[0] != '$') {
        free(word);
        return NULL;
    }

    lusush_completions_t lc = {0, NULL};
    complete_variables(word, &lc);

    char *hint = NULL;
    if (lc.len > 0) {
        // Find the best match
        int best_score = 0;
        size_t best_idx = 0;

        for (size_t i = 0; i < lc.len; i++) {
            int score = fuzzy_match_score(word, lc.cvec[i]);
            if (score > best_score) {
                best_score = score;
                best_idx = i;
            }
        }

        if (best_score >= 70) { // Minimum threshold
            size_t word_len = strlen(word);
            size_t match_len = strlen(lc.cvec[best_idx]);

            if (match_len > word_len &&
                strncmp(word, lc.cvec[best_idx], word_len) == 0) {
                hint = strdup(lc.cvec[best_idx] + word_len);
            }
        }
    }

    // Clean up
    lusush_free_completions(&lc);
    free(word);

    return hint;
}

/**
 * Generate hint for builtin commands with usage information
 */
char *generate_builtin_hint(const char *buf) {
    if (!buf) {
        return NULL;
    }

    int start_pos = 0;
    char *word = get_completion_word(buf, &start_pos);
    if (!word) {
        return NULL;
    }

    // Only show builtin hints for command position
    if (!is_command_position(buf, start_pos)) {
        free(word);
        return NULL;
    }

    char *hint = NULL;

    // Check if current word matches a builtin and provide usage hint
    if (strncmp(word, "cd", strlen(word)) == 0 && strlen(word) < 2) {
        hint = strdup("d [directory]");
    } else if (strncmp(word, "echo", strlen(word)) == 0 && strlen(word) < 4) {
        hint = strdup("ho [text...]");
    } else if (strncmp(word, "export", strlen(word)) == 0 && strlen(word) < 6) {
        hint = strdup("port [var=value]");
    } else if (strncmp(word, "pwd", strlen(word)) == 0 && strlen(word) < 3) {
        hint = strdup("wd");
    } else if (strncmp(word, "set", strlen(word)) == 0 && strlen(word) < 3) {
        hint = strdup("t [options]");
    } else if (strncmp(word, "unset", strlen(word)) == 0 && strlen(word) < 5) {
        hint = strdup("set [variable]");
    } else if (strncmp(word, "exit", strlen(word)) == 0 && strlen(word) < 4) {
        hint = strdup("it [code]");
    } else if (strncmp(word, "source", strlen(word)) == 0 && strlen(word) < 6) {
        hint = strdup("ource [file]");
    } else if (strncmp(word, "alias", strlen(word)) == 0 && strlen(word) < 5) {
        hint = strdup("ias [name=value]");
    } else if (strncmp(word, "unalias", strlen(word)) == 0 &&
               strlen(word) < 7) {
        hint = strdup("alias [name]");
    } else if (strncmp(word, "history", strlen(word)) == 0 &&
               strlen(word) < 7) {
        hint = strdup("istory [n]");
    } else if (strncmp(word, "jobs", strlen(word)) == 0 && strlen(word) < 4) {
        hint = strdup("obs");
    } else if (strncmp(word, "fg", strlen(word)) == 0 && strlen(word) < 2) {
        hint = strdup("g [job]");
    } else if (strncmp(word, "bg", strlen(word)) == 0 && strlen(word) < 2) {
        hint = strdup("g [job]");
    } else if (strncmp(word, "config", strlen(word)) == 0 && strlen(word) < 6) {
        hint = strdup("onfig [command]");
    } else if (strncmp(word, "theme", strlen(word)) == 0 && strlen(word) < 5) {
        hint = strdup("heme [command]");
    }

    free(word);
    return hint;
}

/**
 * Main hints callback function for linenoise
 * This function is called to generate hints that appear to the right of the
 * cursor
 */
char *lusush_hints_callback(const char *buf, int *color, int *bold) {
    if (!buf || !should_show_hints(buf)) {
        return NULL;
    }

    // Set default hint appearance (dim gray)
    *color = 37; // White/gray
    *bold = 0;   // Not bold (dim)

    char *hint = NULL;

    // Try different hint generators in priority order
    if (!hint) {
        hint = generate_builtin_hint(buf);
    }

    if (!hint) {
        hint = generate_command_hint(buf);
    }

    if (!hint) {
        hint = generate_variable_hint(buf);
    }

    if (!hint) {
        hint = generate_file_hint(buf);
    }

    // If we have a hint, make it more visually distinct
    if (hint) {
        // Use a slightly darker gray for hints
        *color = 90; // Dark gray
        *bold = 0;   // Keep it dim
    }

    return hint;
}

/**
 * Free hints callback - called to free hints returned by lusush_hints_callback
 */
void lusush_free_hints_callback(void *hint) {
    if (hint) {
        free(hint);
    }
}
