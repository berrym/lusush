#include "config.h"
#include "posix_history.h"
#include "symtable.h"

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @file enhanced_history.c
 * @brief Enhanced POSIX History Builtin with bash/zsh Compatibility
 *
 * This module implements an enhanced history builtin that provides full
 * bash/zsh compatibility while maintaining POSIX compliance. It supports
 * all standard history operations including range specifications, file
 * operations, and advanced history management.
 *
 * Enhanced history command syntax:
 * - history [n]                    # Show last n commands (or all)
 * - history -c                     # Clear history
 * - history -d offset              # Delete specific entry
 * - history -r [filename]          # Read history from file
 * - history -w [filename]          # Write history to file
 * - history -a [filename]          # Append new entries to file
 * - history -n [filename]          # Read new entries from file
 * - history -t                     # Show timestamps
 *
 * @author Michael Berry
 * @version 1.0
 */

// ============================================================================
// Global History Manager (shared with shell)
// ============================================================================

extern posix_history_manager_t *global_posix_history;

// Forward declarations for POSIX compliance
bool is_posix_mode_enabled(void);

// ============================================================================
// Enhanced History Implementation
// ============================================================================

/**
 * @brief Execute enhanced history command
 *
 * @param manager History manager
 * @param options History command options
 * @return Exit status (0 for success, non-zero for error)
 */
int posix_history_execute(posix_history_manager_t *manager,
                          const posix_history_options_t *options) {
    if (!manager || !options) {
        fprintf(stderr, "history: invalid parameters\n");
        return 1;
    }

    // Handle clear history option
    if (options->clear_history) {
        if (!posix_history_clear(manager)) {
            fprintf(stderr, "history: failed to clear history\n");
            return 1;
        }
        printf("History cleared\n");
        return 0;
    }

    // Handle delete entry option
    if (options->delete_entry) {
        if (!posix_history_delete(manager, options->delete_offset)) {
            const char *error = posix_history_get_last_error();
            fprintf(stderr, "history: %s\n",
                    error ? error : "failed to delete entry");
            return 1;
        }
        printf("Deleted history entry %d\n", options->delete_offset);
        return 0;
    }

    // Handle file operations
    if (options->read_file) {
        int result = posix_history_load(manager, options->filename, true);
        if (result < 0) {
            const char *error = posix_history_get_last_error();
            fprintf(stderr, "history: %s\n",
                    error ? error : "failed to read history file");
            return 1;
        }
        printf("Read %d history entries\n", result);
        return 0;
    }

    if (options->write_file) {
        int result = posix_history_save(manager, options->filename,
                                        options->show_timestamps);
        if (result < 0) {
            const char *error = posix_history_get_last_error();
            fprintf(stderr, "history: %s\n",
                    error ? error : "failed to write history file");
            return 1;
        }
        printf("Wrote %d history entries\n", result);
        return 0;
    }

    if (options->append_file) {
        int result = posix_history_append_new(manager, options->filename);
        if (result < 0) {
            const char *error = posix_history_get_last_error();
            fprintf(stderr, "history: %s\n",
                    error ? error : "failed to append to history file");
            return 1;
        }
        printf("Appended %d new entries\n", result);
        return 0;
    }

    if (options->read_new) {
        int result = posix_history_read_new(manager, options->filename);
        if (result < 0) {
            const char *error = posix_history_get_last_error();
            fprintf(stderr, "history: %s\n",
                    error ? error : "failed to read new entries");
            return 1;
        }
        printf("Read %d new entries\n", result);
        return 0;
    }

    // Default action: list history
    return posix_history_list(manager, options->count,
                              options->show_timestamps);
}

/**
 * @brief List history entries with optional count limit
 *
 * @param manager History manager
 * @param count Number of entries to show (0 for all)
 * @param show_timestamps Whether to include timestamps
 * @return 0 on success, non-zero on error
 */
int posix_history_list(posix_history_manager_t *manager, int count,
                       bool show_timestamps) {
    if (!manager) {
        fprintf(stderr, "history: invalid history manager\n");
        return 1;
    }

    if (manager->count == 0) {
        printf("No history available\n");
        return 0;
    }

    // Determine starting index
    size_t start_index = 0;
    if (count > 0 && (size_t)count < manager->count) {
        start_index = manager->count - count;
    }

    // List entries
    for (size_t i = start_index; i < manager->count; i++) {
        posix_history_entry_t *entry = &manager->entries[i];

        if (show_timestamps) {
            char time_str[64];
            struct tm *tm_info = localtime(&entry->timestamp);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("%5d  %s  %s\n", entry->number, time_str, entry->command);
        } else {
            printf("%5d  %s\n", entry->number, entry->command);
        }
    }

    return 0;
}

// ============================================================================
// History Command Parsing and Validation
// ============================================================================

/**
 * @brief Parse count argument for history listing
 *
 * @param arg Argument string
 * @return Parsed count, or -1 on error
 */
static int parse_count_argument(const char *arg) {
    if (!arg) {
        return 0; // Default: show all
    }

    // Check if argument is a valid number
    char *endptr;
    long count = strtol(arg, &endptr, 10);

    if (*endptr != '\0' || count < 0 || count > POSIX_HISTORY_MAX_ENTRIES) {
        return -1;
    }

    return (int)count;
}

/**
 * @brief Parse delete offset argument
 *
 * @param arg Argument string
 * @return Parsed offset, or -1 on error
 */
static int parse_delete_offset(const char *arg) {
    if (!arg) {
        return -1;
    }

    char *endptr;
    long offset = strtol(arg, &endptr, 10);

    if (*endptr != '\0' || offset <= 0) {
        return -1;
    }

    return (int)offset;
}

/**
 * @brief Get default history filename
 *
 * @return Default filename (must be freed by caller)
 */
static char *get_default_history_filename(void) {
    const char *home = symtable_get_global_default("HOME", "");

    // Use POSIX-compliant history file in posix mode
    const char *history_file =
        is_posix_mode_enabled() ? ".sh_history" : POSIX_HISTORY_DEFAULT_FILE;

    if (!home || !*home) {
        return strdup(history_file);
    }

    char *filename = malloc(strlen(home) + strlen(history_file) + 2);
    if (!filename) {
        return NULL;
    }

    sprintf(filename, "%s/%s", home, history_file);
    return filename;
}

// ============================================================================
// Main History Command Entry Point
// ============================================================================

/**
 * @brief Print enhanced history command usage information
 */
static void enhanced_history_usage(void) {
    fprintf(stderr, "usage: history [options] [n]\n");
    fprintf(stderr, "       history -c\n");
    fprintf(stderr, "       history -d offset\n");
    fprintf(stderr, "       history -r [filename]\n");
    fprintf(stderr, "       history -w [filename]\n");
    fprintf(stderr, "       history -a [filename]\n");
    fprintf(stderr, "       history -n [filename]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -c         Clear the history list\n");
    fprintf(stderr, "  -d offset  Delete the history entry at offset\n");
    fprintf(
        stderr,
        "  -r [file]  Read history from file (default: ~/.lusush_history)\n");
    fprintf(
        stderr,
        "  -w [file]  Write history to file (default: ~/.lusush_history)\n");
    fprintf(stderr, "  -a [file]  Append new entries to file\n");
    fprintf(stderr, "  -n [file]  Read new entries from file\n");
    fprintf(stderr, "  -t         Show timestamps with entries\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "arguments:\n");
    fprintf(stderr,
            "  n          Show last n history entries (default: all)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "examples:\n");
    fprintf(stderr, "  history           Show all history\n");
    fprintf(stderr, "  history 20        Show last 20 commands\n");
    fprintf(stderr, "  history -c        Clear all history\n");
    fprintf(stderr, "  history -d 15     Delete entry number 15\n");
    fprintf(stderr, "  history -w        Save history to default file\n");
    fprintf(stderr, "  history -r backup Load history from backup file\n");
}

/**
 * @brief Main enhanced history command implementation
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit status
 */
int bin_enhanced_history(int argc, char **argv) {
    // Check if LLE is enabled - ehistory command uses GNU readline/POSIX
    // history API
    extern config_values_t config;
    if (config.use_lle) {
        fprintf(stderr, "ehistory: command disabled when LLE is enabled\n");
        fprintf(stderr,
                "ehistory: LLE will have its own history system (Spec 09)\n");
        fprintf(stderr, "ehistory: use 'display lle disable' to switch back to "
                        "GNU readline\n");
        return 1;
    }

    // Initialize history manager if not already done
    if (!global_posix_history) {
        global_posix_history = posix_history_create(0);
        if (!global_posix_history) {
            fprintf(stderr, "history: failed to initialize history manager\n");
            return 1;
        }

        // Set default filename
        char *default_filename = get_default_history_filename();
        if (default_filename) {
            posix_history_set_filename(global_posix_history, default_filename);
            free(default_filename);
        }
    }

    // Initialize options
    posix_history_options_t options = {0};

    // Parse command line options
    int opt;
    optind = 1; // Reset getopt
    while ((opt = getopt(argc, argv, "cd:r::w::a::n::t")) != -1) {
        switch (opt) {
        case 'c':
            options.clear_history = true;
            break;
        case 'd':
            options.delete_entry = true;
            options.delete_offset = parse_delete_offset(optarg);
            if (options.delete_offset == -1) {
                fprintf(stderr, "history: invalid offset for -d option\n");
                return 1;
            }
            break;
        case 'r':
            options.read_file = true;
            if (optarg) {
                options.filename = strdup(optarg);
            }
            break;
        case 'w':
            options.write_file = true;
            if (optarg) {
                options.filename = strdup(optarg);
            }
            break;
        case 'a':
            options.append_file = true;
            if (optarg) {
                options.filename = strdup(optarg);
            }
            break;
        case 'n':
            options.read_new = true;
            if (optarg) {
                options.filename = strdup(optarg);
            }
            break;
        case 't':
            options.show_timestamps = true;
            break;
        default:
            enhanced_history_usage();
            return 1;
        }
    }

    // Parse count argument if no options specified
    if (!options.clear_history && !options.delete_entry && !options.read_file &&
        !options.write_file && !options.append_file && !options.read_new) {

        if (optind < argc) {
            options.count = parse_count_argument(argv[optind]);
            if (options.count == -1) {
                fprintf(stderr, "history: invalid count argument '%s'\n",
                        argv[optind]);
                return 1;
            }
        }
    }

    // Validate mutually exclusive options
    int operation_count = 0;
    if (options.clear_history)
        operation_count++;
    if (options.delete_entry)
        operation_count++;
    if (options.read_file)
        operation_count++;
    if (options.write_file)
        operation_count++;
    if (options.append_file)
        operation_count++;
    if (options.read_new)
        operation_count++;

    if (operation_count > 1) {
        fprintf(stderr, "history: conflicting options specified\n");
        free(options.filename);
        return 1;
    }

    // Execute history command
    int status = posix_history_execute(global_posix_history, &options);

    // Cleanup
    free(options.filename);
    return status;
}

// ============================================================================
// Legacy History Interface Compatibility
// ============================================================================

/**
 * @brief Legacy history print function for backward compatibility
 */
void enhanced_history_print(void) {
    if (!global_posix_history) {
        printf("No history available\n");
        return;
    }

    posix_history_list(global_posix_history, 0, false);
}

/**
 * @brief Legacy history lookup function for backward compatibility
 *
 * @param s String representation of history number
 * @return Command string, or NULL if not found
 */
char *enhanced_history_lookup(const char *s) {
    if (!global_posix_history || !s) {
        return NULL;
    }

    int number = posix_history_resolve_number(global_posix_history, s);
    if (number == -1) {
        return NULL;
    }

    posix_history_entry_t *entry =
        posix_history_get(global_posix_history, number);
    return entry ? entry->command : NULL;
}

/**
 * @brief Legacy history usage function for backward compatibility
 */
void enhanced_history_usage_legacy(void) {
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "history\t\t\t(print the command history)\n");
    fprintf(stderr, "history index\t\t(execute command in history)\n");
    fprintf(stderr, "history -c\t\t(clear history)\n");
    fprintf(stderr, "history -d offset\t(delete entry at offset)\n");
    fprintf(stderr, "history -w [file]\t(write history to file)\n");
    fprintf(stderr, "history -r [file]\t(read history from file)\n");
    fprintf(stderr, "\nFor full bash/zsh compatibility, use: help history\n");
}

/**
 * @brief Initialize enhanced history system
 *
 * This function sets up the enhanced history system and loads existing
 * history from the default file.
 */
void enhanced_history_init(void) {
    if (global_posix_history) {
        return; // Already initialized
    }

    global_posix_history = posix_history_create(0);
    if (!global_posix_history) {
        fprintf(stderr,
                "Warning: Failed to initialize enhanced history system\n");
        return;
    }

    // Set default filename
    char *default_filename = get_default_history_filename();
    if (default_filename) {
        posix_history_set_filename(global_posix_history, default_filename);

        // Load existing history
        posix_history_load(global_posix_history, default_filename, false);

        free(default_filename);
    }

    // Enable duplicate detection by default
    posix_history_set_no_duplicates(global_posix_history, true);
}

/**
 * @brief Add command to enhanced history
 *
 * @param command Command to add
 */
void enhanced_history_add(const char *command) {
    if (!global_posix_history) {
        enhanced_history_init();
    }

    if (global_posix_history && command) {
        posix_history_add(global_posix_history, command);
    }
}

/**
 * @brief Save enhanced history to file
 */
void enhanced_history_save(void) {
    if (global_posix_history) {
        posix_history_save(global_posix_history, NULL, false);
    }
}

/**
 * @brief Cleanup enhanced history system
 */
void enhanced_history_cleanup(void) {
    if (global_posix_history) {
        posix_history_destroy(global_posix_history);
        global_posix_history = NULL;
    }
}