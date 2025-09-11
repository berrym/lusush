#include "../../include/posix_history.h"
#include "../../include/errors.h"
#include "../../include/executor.h"
#include "../../include/strings.h"
#include "../../include/symtable.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

/**
 * @file fc.c
 * @brief POSIX fc Command Builtin Implementation
 *
 * This module implements the POSIX fc (fix command) builtin for Lusush shell.
 * The fc command provides POSIX-compliant history editing, listing, and
 * re-execution capabilities.
 *
 * POSIX fc command syntax:
 * - fc [-r] [-e editor] [first [last]]    # Edit and re-execute
 * - fc -l [-nr] [first [last]]            # List commands
 * - fc -s [old=new] [first]               # Substitute and re-execute
 *
 * @author Michael Berry
 * @version 1.0
 */

// ============================================================================
// Global History Manager (shared with shell)
// ============================================================================

extern posix_history_manager_t *global_posix_history;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Parse old=new substitution pattern
 *
 * @param pattern Substitution pattern (old=new)
 * @param old Pointer to receive old pattern (must be freed)
 * @param new Pointer to receive new pattern (must be freed)
 * @return true on success, false on parse error
 */
static bool parse_substitution_pattern(const char *pattern, char **old, char **new) {
    if (!pattern || !old || !new) {
        return false;
    }

    const char *equals = strchr(pattern, '=');
    if (!equals) {
        // No equals sign, treat entire pattern as old with empty new
        *old = strdup(pattern);
        *new = strdup("");
        return (*old && *new);
    }

    size_t old_len = equals - pattern;
    size_t new_len = strlen(equals + 1);

    *old = malloc(old_len + 1);
    *new = malloc(new_len + 1);

    if (!*old || !*new) {
        free(*old);
        free(*new);
        return false;
    }

    strncpy(*old, pattern, old_len);
    (*old)[old_len] = '\0';
    strcpy(*new, equals + 1);

    return true;
}

/**
 * @brief Execute a command and return its exit status
 *
 * @param command Command to execute
 * @return Exit status of command
 */
static int execute_command(const char *command) {
    if (!command || !*command) {
        return 1;
    }

    // Use the shell's executor to run the command
    executor_t *executor = executor_new();
    if (!executor) {
        fprintf(stderr, "fc: failed to create executor\n");
        return 1;
    }

    int exit_status = executor_execute_command_line(executor, command);

    if (executor_has_error(executor)) {
        fprintf(stderr, "fc: %s\n", executor_error(executor));
    }

    executor_free(executor);
    return exit_status;
}

// ============================================================================
// fc Command Implementations
// ============================================================================

/**
 * @brief List history entries with fc formatting
 *
 * @param manager History manager
 * @param range Range of entries to list
 * @param reverse_order Whether to list in reverse order
 * @param suppress_numbers Whether to suppress line numbers
 * @return 0 on success, non-zero on error
 */
int posix_fc_list(posix_history_manager_t *manager, const posix_history_range_t *range,
                  bool reverse_order, bool suppress_numbers) {
    if (!manager || !range || !range->valid) {
        fprintf(stderr, "fc: invalid range for list operation\n");
        return 1;
    }

    // Collect entries in range
    posix_history_entry_t *entries[POSIX_HISTORY_MAX_ENTRIES];
    size_t count = 0;

    for (size_t i = 0; i < manager->count && count < POSIX_HISTORY_MAX_ENTRIES; i++) {
        int number = manager->entries[i].number;
        if (number >= range->first && number <= range->last) {
            entries[count++] = &manager->entries[i];
        }
    }

    if (count == 0) {
        fprintf(stderr, "fc: no commands found in range %d-%d\n", range->first, range->last);
        return 1;
    }

    // Print entries in requested order
    if (reverse_order) {
        for (int i = (int)count - 1; i >= 0; i--) {
            if (suppress_numbers) {
                printf("%s\n", entries[i]->command);
            } else {
                printf("%5d  %s\n", entries[i]->number, entries[i]->command);
            }
        }
    } else {
        for (size_t i = 0; i < count; i++) {
            if (suppress_numbers) {
                printf("%s\n", entries[i]->command);
            } else {
                printf("%5d  %s\n", entries[i]->number, entries[i]->command);
            }
        }
    }

    return 0;
}

/**
 * @brief Edit and re-execute history entries
 *
 * @param manager History manager
 * @param range Range of entries to edit
 * @param editor Editor command to use (NULL for default)
 * @return 0 on success, non-zero on error
 */
int posix_fc_edit(posix_history_manager_t *manager, const posix_history_range_t *range,
                  const char *editor) {
    if (!manager || !range || !range->valid) {
        fprintf(stderr, "fc: invalid range for edit operation\n");
        return 1;
    }

    // Collect commands in range
    char *content = NULL;
    size_t content_size = 0;
    size_t content_capacity = 1024;

    content = malloc(content_capacity);
    if (!content) {
        fprintf(stderr, "fc: memory allocation failed\n");
        return 1;
    }
    content[0] = '\0';

    for (size_t i = 0; i < manager->count; i++) {
        int number = manager->entries[i].number;
        if (number >= range->first && number <= range->last) {
            const char *command = manager->entries[i].command;
            size_t command_len = strlen(command);
            
            // Ensure buffer capacity
            if (content_size + command_len + 2 > content_capacity) {
                content_capacity = (content_size + command_len + 2) * 2;
                char *new_content = realloc(content, content_capacity);
                if (!new_content) {
                    free(content);
                    fprintf(stderr, "fc: memory allocation failed\n");
                    return 1;
                }
                content = new_content;
            }
            
            // Append command
            strcat(content, command);
            strcat(content, "\n");
            content_size += command_len + 1;
        }
    }

    if (content_size == 0) {
        free(content);
        fprintf(stderr, "fc: no commands found in range %d-%d\n", range->first, range->last);
        return 1;
    }

    // Create temporary file
    char *temp_filename = NULL;
    if (!posix_history_create_temp_file(content, &temp_filename)) {
        free(content);
        fprintf(stderr, "fc: failed to create temporary file\n");
        return 1;
    }
    free(content);

    // Determine editor to use
    char *editor_cmd = NULL;
    if (editor) {
        editor_cmd = strdup(editor);
    } else {
        editor_cmd = posix_history_get_default_editor();
    }

    if (!editor_cmd) {
        unlink(temp_filename);
        free(temp_filename);
        fprintf(stderr, "fc: no editor available\n");
        return 1;
    }

    // Build editor command
    char editor_command[POSIX_HISTORY_MAX_EDITOR_COMMAND];
    snprintf(editor_command, sizeof(editor_command), "%s %s", editor_cmd, temp_filename);
    free(editor_cmd);

    // Execute editor
    int editor_status = system(editor_command);
    if (editor_status != 0) {
        unlink(temp_filename);
        free(temp_filename);
        fprintf(stderr, "fc: editor failed with status %d\n", editor_status);
        return 1;
    }

    // Read back edited content
    char *edited_content = posix_history_read_file_content(temp_filename);
    unlink(temp_filename);
    free(temp_filename);

    if (!edited_content) {
        fprintf(stderr, "fc: failed to read edited content\n");
        return 1;
    }

    // Execute edited commands line by line
    int final_status = 0;
    char *line_start = edited_content;
    char *line_end;

    while ((line_end = strchr(line_start, '\n')) != NULL) {
        *line_end = '\0';
        
        // Skip empty lines
        char *trimmed = line_start;
        while (*trimmed && isspace(*trimmed)) trimmed++;
        if (*trimmed) {
            // Add to history and execute
            posix_history_add(manager, trimmed);
            printf("%s\n", trimmed);
            final_status = execute_command(trimmed);
        }
        
        line_start = line_end + 1;
    }

    // Handle last line if no trailing newline
    if (*line_start) {
        char *trimmed = line_start;
        while (*trimmed && isspace(*trimmed)) trimmed++;
        if (*trimmed) {
            posix_history_add(manager, trimmed);
            printf("%s\n", trimmed);
            final_status = execute_command(trimmed);
        }
    }

    free(edited_content);
    return final_status;
}

/**
 * @brief Substitute and re-execute history command
 *
 * @param manager History manager
 * @param number History number to operate on
 * @param old_pattern Pattern to replace
 * @param new_pattern Replacement pattern
 * @return 0 on success, non-zero on error
 */
int posix_fc_substitute(posix_history_manager_t *manager, int number,
                       const char *old_pattern, const char *new_pattern) {
    if (!manager || !old_pattern || !new_pattern) {
        fprintf(stderr, "fc: invalid parameters for substitution\n");
        return 1;
    }

    posix_history_entry_t *entry = posix_history_get(manager, number);
    if (!entry) {
        fprintf(stderr, "fc: history entry %d not found\n", number);
        return 1;
    }

    // Perform substitution
    const char *original = entry->command;
    const char *match = strstr(original, old_pattern);
    
    if (!match) {
        fprintf(stderr, "fc: pattern '%s' not found in command\n", old_pattern);
        return 1;
    }

    // Build new command with substitution
    size_t old_len = strlen(old_pattern);
    size_t new_len = strlen(new_pattern);
    size_t original_len = strlen(original);
    size_t prefix_len = match - original;
    size_t suffix_len = original_len - prefix_len - old_len;
    
    char *new_command = malloc(prefix_len + new_len + suffix_len + 1);
    if (!new_command) {
        fprintf(stderr, "fc: memory allocation failed\n");
        return 1;
    }

    // Copy prefix
    strncpy(new_command, original, prefix_len);
    new_command[prefix_len] = '\0';
    
    // Add replacement
    strcat(new_command, new_pattern);
    
    // Add suffix
    strcat(new_command, match + old_len);

    // Print the substituted command
    printf("%s\n", new_command);

    // Add to history and execute
    posix_history_add(manager, new_command);
    int status = execute_command(new_command);
    
    free(new_command);
    return status;
}

/**
 * @brief Execute fc command with given options
 *
 * @param manager History manager
 * @param options fc command options
 * @return Exit status (0 for success, non-zero for error)
 */
int posix_fc_execute(posix_history_manager_t *manager, const posix_fc_options_t *options) {
    if (!manager || !options) {
        fprintf(stderr, "fc: invalid parameters\n");
        return 1;
    }

    if (options->list_mode) {
        return posix_fc_list(manager, &options->range, options->reverse_order, 
                            options->suppress_numbers);
    } else if (options->substitute_mode) {
        // For substitute mode, use first number from range
        return posix_fc_substitute(manager, options->range.first, 
                                  options->old_pattern, options->new_pattern);
    } else {
        // Edit mode
        return posix_fc_edit(manager, &options->range, options->editor);
    }
}

// ============================================================================
// Main fc Command Entry Point
// ============================================================================

/**
 * @brief Print fc command usage information
 */
static void fc_usage(void) {
    fprintf(stderr, "usage: fc [-e editor] [-r] [first [last]]\n");
    fprintf(stderr, "       fc -l [-nr] [first [last]]\n");
    fprintf(stderr, "       fc -s [old=new] [first]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -e editor  Use specified editor\n");
    fprintf(stderr, "  -l         List commands instead of editing\n");
    fprintf(stderr, "  -n         Suppress line numbers in list mode\n");
    fprintf(stderr, "  -r         Reverse order (newest first)\n");
    fprintf(stderr, "  -s         Substitute old with new and re-execute\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "range specifiers:\n");
    fprintf(stderr, "  number     Specific history number\n");
    fprintf(stderr, "  -offset    Relative to current (e.g., -1 = last)\n");
    fprintf(stderr, "  string     Most recent command starting with string\n");
}

/**
 * @brief Main fc command implementation
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit status
 */
int bin_fc(int argc, char **argv) {
    // Initialize history manager if not already done
    if (!global_posix_history) {
        global_posix_history = posix_history_create(0);
        if (!global_posix_history) {
            fprintf(stderr, "fc: failed to initialize history manager\n");
            return 1;
        }
    }

    // Initialize options
    posix_fc_options_t options = {0};
    
    // Parse command line options
    int opt;
    while ((opt = getopt(argc, argv, "e:lnrs")) != -1) {
        switch (opt) {
            case 'e':
                options.editor = strdup(optarg);
                break;
            case 'l':
                options.list_mode = true;
                break;
            case 'n':
                options.suppress_numbers = true;
                break;
            case 'r':
                options.reverse_order = true;
                break;
            case 's':
                options.substitute_mode = true;
                break;
            default:
                fc_usage();
                return 1;
        }
    }

    // Handle substitute mode pattern parsing
    if (options.substitute_mode) {
        if (optind < argc) {
            if (!parse_substitution_pattern(argv[optind], 
                                          &options.old_pattern, 
                                          &options.new_pattern)) {
                fprintf(stderr, "fc: invalid substitution pattern\n");
                return 1;
            }
            optind++;
        } else {
            // Default substitution pattern (empty old, empty new)
            options.old_pattern = strdup("");
            options.new_pattern = strdup("");
        }
    }

    // Parse range arguments
    const char *first_str = (optind < argc) ? argv[optind] : NULL;
    const char *last_str = (optind + 1 < argc) ? argv[optind + 1] : NULL;
    
    if (!posix_history_parse_range(global_posix_history, first_str, last_str, &options.range)) {
        const char *error = posix_history_get_last_error();
        fprintf(stderr, "fc: %s\n", error ? error : "invalid range");
        
        // Cleanup
        free(options.editor);
        free(options.old_pattern);
        free(options.new_pattern);
        return 1;
    }

    // Execute fc command
    int status = posix_fc_execute(global_posix_history, &options);

    // Cleanup
    free(options.editor);
    free(options.old_pattern);
    free(options.new_pattern);

    return status;
}