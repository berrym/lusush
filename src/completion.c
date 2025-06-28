#include "../include/completion.h"

#include "../include/alias.h"
#include "../include/builtins.h"
#include "../include/libhashtable/ht.h"
#include "../include/linenoise/linenoise.h"
#include "../include/lusush.h"
#include "../include/network.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

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
static void display_completion_context(linenoiseCompletions *lc,
                                       const char *word);
static void prioritize_completions(linenoiseCompletions *lc,
                                   const char *pattern);

/**
 * Enhanced completion function that uses fuzzy matching
 */
static void add_fuzzy_completion(linenoiseCompletions *lc, const char *pattern,
                                 const char *candidate, const char *suffix,
                                 int min_score) {
    int score = fuzzy_match_score(pattern, candidate);
    if (score >= min_score) {
        add_completion_with_suffix(lc, candidate, suffix);
    }
}

/**
 * Main completion callback for linenoise
 */
void lusush_completion_callback(const char *buf, linenoiseCompletions *lc) {
    if (!buf || !lc) {
        return;
    }

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
        complete_network_command_args(command, word, lc);
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

    // Show completion context for better user experience
    if (lc->len > 0) {
        display_completion_context(lc, word);
    }

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
void complete_builtins(const char *text, linenoiseCompletions *lc) {
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
            // Try fuzzy matching with lower threshold
            add_fuzzy_completion(lc, text, builtins[i].name, " ", 60);
        }
    }
}

/**
 * Complete command aliases
 */
void complete_aliases(const char *text, linenoiseCompletions *lc) {
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
void complete_commands(const char *text, linenoiseCompletions *lc) {
    if (!text) {
        return;
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

    while (dir) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL) {
                if (entry->d_name[0] != '.') {
                    // Check if it's executable first
                    char full_path[strlen(dir) + strlen(entry->d_name) + 2];
                    snprintf(full_path, sizeof(full_path), "%s/%s", dir,
                             entry->d_name);

                    struct stat st;
                    if (stat(full_path, &st) == 0 && (st.st_mode & S_IXUSR)) {
                        // Exact prefix match gets priority
                        if (strncmp(entry->d_name, text, text_len) == 0 &&
                            strlen(entry->d_name) > text_len) {
                            add_completion_with_suffix(lc, entry->d_name, " ");
                        } else {
                            // Try fuzzy matching for commands
                            add_fuzzy_completion(lc, text, entry->d_name, " ",
                                                 70);
                        }
                    }
                }
            }
            closedir(d);
        }
        dir = strtok(NULL, ":");
    }

    free(path);
}

/**
 * Complete files and directories
 */
void complete_files(const char *text, linenoiseCompletions *lc) {
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
void complete_variables(const char *text, linenoiseCompletions *lc) {
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
                linenoiseAddCompletion(lc, var_name);
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
            linenoiseAddCompletion(lc, special_vars[i]);
        }
    }
}

/**
 * Complete from history (fallback)
 */
void complete_history(const char *text, linenoiseCompletions *lc) {
    if (!text) {
        return;
    }

    size_t text_len = strlen(text);
    size_t i = 0;
    char *line = NULL;

    while ((line = linenoiseHistoryGet(i)) != NULL) {
        if (strncmp(line, text, text_len) == 0 && strlen(line) > text_len) {
            linenoiseAddCompletion(lc, line);
        }
        free(line);
        i++;
    }
}

/**
 * Enhanced completion display with smart formatting
 */
static void display_completion_context(linenoiseCompletions *lc,
                                       const char *word
                                       __attribute__((unused))) {
    if (!lc || lc->len == 0) {
        return;
    }

    // For many completions, show count
    if (lc->len > 10) {
        printf("\n[%zu completions available - press TAB again to show all]\n",
               lc->len);
    }
}

/**
 * Add completion with appropriate suffix and context awareness
 */
void add_completion_with_suffix(linenoiseCompletions *lc,
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
        linenoiseAddCompletion(lc, full_completion);
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
static void prioritize_completions(linenoiseCompletions *lc,
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
