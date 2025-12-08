#include "../../include/builtins.h"

#include "../../include/alias.h"
#include "../../include/config.h"
#include "../../include/debug.h"
#include "../../include/errors.h"
#include "../../include/executor.h"
#include "../../include/history.h"
#include "../../include/libhashtable/ht.h"
#include "../../include/readline_integration.h"
#include "../../include/display_integration.h"
#include "../../include/lusush.h"
#include "../../include/network.h"
#include "../../include/prompt.h"
#include "../../include/signals.h"
#include "../../include/symtable.h"
#include "../../include/termcap.h"
#include "../../include/themes.h"
#include "../../include/version.h"
#include "../../include/autosuggestions.h"
#include "../../include/input.h"
#include "../../include/lusush_memory_pool.h"
#include "../../include/lle/lle_editor.h"
#include "../../include/lle/history.h"
#include "../../include/lle/keybinding.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Forward declarations for job control builtins
int bin_jobs(int argc, char **argv);
int bin_fg(int argc, char **argv);
int bin_bg(int argc, char **argv);
int bin_colon(int argc, char **argv);
int bin_readonly(int argc, char **argv);
int bin_config(int argc, char **argv);
int bin_hash(int argc, char **argv);
int bin_theme(int argc, char **argv);
int bin_display(int argc, char **argv);
int bin_network(int argc, char **argv);
int bin_debug(int argc, char **argv);

// Forward declarations for POSIX compliance
bool is_posix_mode_enabled(void);

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Hash table for remembered command paths
ht_strstr_t *command_hash = NULL;

// Table of builtin commands
builtin builtins[] = {
    {    "exit",                       "exit shell",     bin_exit},
    {    "help",                     "builtin help",     bin_help},
    {      "cd",                 "change directory",       bin_cd},
    {     "pwd",          "print working directory",      bin_pwd},
    { "history",            "print command history",  bin_history},
    {      "fc",         "POSIX history edit/list",      bin_fc},
    {"ehistory",       "enhanced history commands", bin_enhanced_history},
    {   "alias",                     "set an alias",    bin_alias},
    { "unalias",                   "unset an alias",  bin_unalias},
    {   "clear",                 "clear the screen",    bin_clear},
    { "termcap",      "terminal capability testing",  bin_termcap},

    {    "type",             "display command type",     bin_type},
    {   "unset",           "unset a shell variable",    bin_unset},

    {    "echo",              "echo text to stdout",     bin_echo},
    {  "printf",                 "formatted output",   bin_printf},
    {  "export",           "export shell variables",   bin_export},
    {  "source",                  "source a script",   bin_source},
    {       ".",                  "source a script",   bin_source},
    {    "test",                 "test expressions",     bin_test},
    {       "[",                 "test expressions",     bin_test},
    {    "read",                  "read user input",     bin_read},
    {    "eval",               "evaluate arguments",     bin_eval},
    {    "true",            "return success status",     bin_true},
    {   "false",            "return failure status",    bin_false},
    {     "set",                "set shell options",      bin_set},
    {    "jobs",                 "list active jobs",     bin_jobs},
    {      "fg",          "bring job to foreground",       bin_fg},
    {      "bg",           "send job to background",       bin_bg},
    {   "shift",      "shift positional parameters",    bin_shift},
    {   "break",               "break out of loops",    bin_break},
    {"continue",  "continue to next loop iteration", bin_continue},
    {  "return",            "return from functions",   bin_return},
    {"return_value",     "set function return value", bin_return_value},
    {    "trap",              "set signal handlers",     bin_trap},
    {    "exec",       "replace shell with command",     bin_exec},
    {    "wait",         "wait for background jobs",     bin_wait},
    {   "umask",   "set/display file creation mask",    bin_umask},
    {  "ulimit",      "set/display resource limits",   bin_ulimit},
    {   "times",            "display process times",    bin_times},
    { "getopts",            "parse command options",  bin_getopts},
    {   "local",          "declare local variables",    bin_local},
    {       ":",             "null command (no-op)",    bin_colon},
    {"readonly",       "create read-only variables", bin_readonly},
    {  "config",       "manage shell configuration",   bin_config},
    {    "hash",       "remember utility locations",     bin_hash},
    {   "theme",              "manage shell themes",    bin_theme},
    { "display",       "manage layered display system", bin_display},
    { "network",     "manage network and SSH hosts",  bin_network},
    {   "debug", "advanced debugging and profiling",    bin_debug},
};

const size_t builtins_count = sizeof(builtins) / sizeof(builtins[0]);

/**
 * bin_colon:
 *      Null command - does nothing and returns success.
 *      Used for parameter expansions and as a no-op.
 */
int bin_colon(int argc __attribute__((unused)),
              char **argv __attribute__((unused))) {
    if (getenv("PARAM_EXPANSION_DEBUG")) {
        fprintf(stderr, "DEBUG: colon builtin received %d arguments:\n", argc);
        for (int i = 0; i < argc; i++) {
            fprintf(stderr, "  argv[%d] = '%s'\n", i,
                    argv[i] ? argv[i] : "(null)");
        }
    }
    return 0;
}

/**
 * bin_exit:
 *      Exit the shell.
 */
int bin_exit(int argc, char **argv) {
    int exit_code = 0;

    // Parse exit code argument if provided
    if (argc > 1) {
        exit_code = atoi(argv[1]);
    }

    // Execute EXIT traps before terminating
    execute_exit_traps();

    // Exit with the specified code
    exit(exit_code);
}

/**
 * bin_help:
 *      Print a list of builtins and their description.
 */
int bin_help(int argc __attribute__((unused)),
             char **argv __attribute__((unused))) {
    for (size_t i = 0; i < builtins_count; i++) {
        fprintf(stderr, "\t%-10s%-40s\n", builtins[i].name, builtins[i].doc);
    }

    return 0;
}

/**
 * canonicalize_logical_path:
 *      Canonicalize a path by resolving . and .. components logically
 *      (without following symlinks). Returns a malloc'd string or NULL on error.
 */
static char *canonicalize_logical_path(const char *path) {
    if (!path) return NULL;
    
    size_t path_len = strlen(path);
    char *result = malloc(path_len + 1);
    if (!result) return NULL;
    
    strcpy(result, path);
    
    // Simple canonicalization: remove /./  and resolve /../
    char *src = result;
    char *dst = result;
    
    while (*src) {
        if (*src == '/') {
            // Skip multiple slashes
            while (*src == '/') src++;
            if (dst > result || dst == result) *dst++ = '/';
            
            // Check for . and ..
            if (*src == '.') {
                if (src[1] == '/' || src[1] == '\0') {
                    // Skip ./
                    src++;
                    continue;
                } else if (src[1] == '.' && (src[2] == '/' || src[2] == '\0')) {
                    // Handle ../
                    src += 2;
                    // Remove last component from dst
                    if (dst > result + 1) {
                        dst--; // Back up from the /
                        while (dst > result && dst[-1] != '/') dst--;
                    }
                    continue;
                }
            }
        }
        *dst++ = *src++;
    }
    
    // Remove trailing slash unless it's root
    if (dst > result + 1 && dst[-1] == '/') {
        dst--;
    }
    
    *dst = '\0';
    
    // Handle empty path
    if (dst == result) {
        strcpy(result, "/");
    }
    
    return result;
}

/**
 * bin_cd:
 *      Change working directory.
 */
int bin_cd(int argc __attribute__((unused)),
           char **argv __attribute__((unused))) {
    static char *previous_dir = NULL;
    char *current_dir = NULL;
    char *target_dir = NULL;

    // Privileged mode security check
    if (shell_opts.privileged_mode) {
        fprintf(stderr, "cd: restricted command in privileged mode\n");
        return 1;
    }

    // Get current directory before changing
    current_dir = getcwd(NULL, 0);
    if (!current_dir && errno != ENOENT) {
        error_return("cd: getcwd");
        return 1;
    }

    if (argc == 1) {
        // cd with no arguments - go to HOME
        target_dir = getenv("HOME");
        if (!target_dir) {
            error_message("cd: HOME not set");
            free(current_dir);
            return 1;
        }
    } else if (argc == 2) {
        if (strcmp(argv[1], "-") == 0) {
            // cd - : go to previous directory
            if (!previous_dir) {
                error_message("cd: OLDPWD not set");
                free(current_dir);
                return 1;
            }
            target_dir = previous_dir;
            // Print the directory we're changing to (standard behavior)
            printf("%s\n", target_dir);
        } else {
            target_dir = argv[1];
        }
    } else {
        error_message("usage: cd [pathname | -]");
        free(current_dir);
        return 1;
    }

    // Attempt to change directory
    if (chdir(target_dir) < 0) {
        error_return("cd");
        free(current_dir);
        return 1;
    }

    // Update previous directory
    if (previous_dir) {
        free(previous_dir);
    }
    previous_dir = current_dir;

    // Set OLDPWD variable according to current mode
    if (previous_dir) {
        if (shell_opts.physical_mode) {
            // In physical mode, resolve OLDPWD to physical path
            char *resolved_prev = realpath(previous_dir, NULL);
            if (resolved_prev) {
                symtable_set_global("OLDPWD", resolved_prev);
                free(resolved_prev);
            } else {
                symtable_set_global("OLDPWD", previous_dir);
            }
        } else {
            // In logical mode, use logical path
            symtable_set_global("OLDPWD", previous_dir);
        }
    }

    // Set PWD variable according to current mode
    if (shell_opts.physical_mode) {
        // In physical mode, resolve PWD to physical path
        char *resolved_dir = realpath(".", NULL);
        if (resolved_dir) {
            symtable_set_global("PWD", resolved_dir);
            free(resolved_dir);
        }
    } else {
        // In logical mode, preserve the logical path taken
        if (argc == 2 && strcmp(argv[1], "-") == 0) {
            // cd - case: PWD becomes old OLDPWD (already handled above in cd - logic)
            char *new_dir = getcwd(NULL, 0);
            if (new_dir) {
                symtable_set_global("PWD", new_dir);
                free(new_dir);
            }
        } else if (target_dir && target_dir[0] == '/') {
            // Absolute path - canonicalize it in logical mode
            char *canonical_path = canonicalize_logical_path(target_dir);
            if (canonical_path) {
                symtable_set_global("PWD", canonical_path);
                free(canonical_path);
            } else {
                symtable_set_global("PWD", target_dir);
            }
        } else if (target_dir) {
            // Relative path - build logical path from current PWD
            char *current_pwd = symtable_get_global("PWD");
            if (current_pwd && strlen(current_pwd) > 0) {
                size_t pwd_len = strlen(current_pwd);
                size_t target_len = strlen(target_dir);
                char *logical_path = malloc(pwd_len + target_len + 2);
                if (logical_path) {
                    strcpy(logical_path, current_pwd);
                    if (logical_path[pwd_len - 1] != '/') {
                        strcat(logical_path, "/");
                    }
                    strcat(logical_path, target_dir);
                    
                    // Canonicalize the logical path to handle . and ..
                    char *canonical_path = canonicalize_logical_path(logical_path);
                    if (canonical_path) {
                        symtable_set_global("PWD", canonical_path);
                        free(canonical_path);
                    } else {
                        symtable_set_global("PWD", logical_path);
                    }
                    free(logical_path);
                } else {
                    // Fallback to getcwd if malloc fails
                    char *new_dir = getcwd(NULL, 0);
                    if (new_dir) {
                        symtable_set_global("PWD", new_dir);
                        free(new_dir);
                    }
                }
            } else {
                // No current PWD - fallback to getcwd
                char *new_dir = getcwd(NULL, 0);
                if (new_dir) {
                    symtable_set_global("PWD", new_dir);
                    free(new_dir);
                }
            }
        } else {
            // cd with no arguments - go to HOME
            char *home = getenv("HOME");
            if (home) {
                symtable_set_global("PWD", home);
            } else {
                char *new_dir = getcwd(NULL, 0);
                if (new_dir) {
                    symtable_set_global("PWD", new_dir);
                    free(new_dir);
                }
            }
        }
    }

    return 0;
}

/**
 * bin_pwd:
 *      Print working directory.
 */
int bin_pwd(int argc __attribute__((unused)),
            char **argv __attribute__((unused))) {
    if (shell_opts.physical_mode) {
        // In physical mode, resolve symlinks and show physical path
        char *physical_path = realpath(".", NULL);
        if (physical_path) {
            printf("%s\n", physical_path);
            free(physical_path);
            return 0;
        }
        // Fall through to error handling if realpath fails
    } else {
        // In logical mode, use PWD from symbol table if available
        char *pwd_value = symtable_get_global("PWD");
        if (pwd_value) {
            printf("%s\n", pwd_value);
            return 0;
        }
        // Fall through to getcwd if PWD not available
    }

    // Fallback - use getcwd
    char cwd[MAXLINE] = {'\0'};
    if (getcwd(cwd, MAXLINE) == NULL) {
        error_return("pwd");
        return 1;
    }

    printf("%s\n", cwd);

    return 0;
}

/**
 * bin_history:
 *      Implementation of a history command.
 */
int bin_history(int argc __attribute__((unused)),
                char **argv __attribute__((unused))) {
    // Check if LLE is enabled - history command uses GNU readline API
    extern config_values_t config;
    if (config.use_lle) {
        fprintf(stderr, "history: command disabled when LLE is enabled\n");
        fprintf(stderr, "history: LLE will have its own history system (Spec 09)\n");
        fprintf(stderr, "history: use 'display lle disable' to switch back to GNU readline\n");
        return 1;
    }
    
    char *line = NULL;

    switch (argc) {
    case 1:
        history_print();
        break;
    case 2:
        // Lookup a history entry
        line = history_lookup(argv[1]);
        if (line == NULL) {
            error_message("history: unable to find entry %s", argv[1]);
            return 1;
        }

        // Add the retrieved command to history
        if (*line) {
            history_add(line);
            history_save();
        }

        // Execute the history entry
        parse_and_execute(line);
        break;
    default:
        history_usage();
        break;
    }

    return 0;
}

/**
 * bin_termcap:
 *      Test and demonstrate enhanced termcap functionality.
 */
int bin_termcap(int argc, char **argv) {
    extern int termcap_run_all_tests(void);
    extern int termcap_interactive_demo(void);
    extern void termcap_dump_capabilities(void);
    
    if (argc == 1) {
        // No arguments - show capabilities
        termcap_dump_capabilities();
        return 0;
    }
    
    if (argc == 2) {
        if (strcmp(argv[1], "test") == 0 || strcmp(argv[1], "--test") == 0) {
            return termcap_run_all_tests();
        } else if (strcmp(argv[1], "demo") == 0 || strcmp(argv[1], "--demo") == 0) {
            return termcap_interactive_demo();
        } else if (strcmp(argv[1], "capabilities") == 0 || strcmp(argv[1], "--capabilities") == 0) {
            termcap_dump_capabilities();
            return 0;
        } else if (strcmp(argv[1], "integration") == 0 || strcmp(argv[1], "--integration") == 0) {
            // Enhanced shell integration demo
            printf("Lusush Enhanced Termcap Integration Demo\n");
            printf("=========================================\n\n");
            
            const terminal_info_t *term_info = termcap_get_info();
            if (term_info && term_info->is_tty) {
                success_message("Terminal detected successfully");
                info_message("Terminal size: %dx%d", term_info->cols, term_info->rows);
                
                if (termcap_supports_colors()) {
                    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "Color support: ");
                    printf("OK Active\n");
                    
                    if (termcap_supports_256_colors()) {
                        termcap_print_colored(TERMCAP_CYAN, TERMCAP_DEFAULT, "256-color mode: ");
                        printf("OK Available\n");
                    }
                    
                    if (termcap_supports_truecolor()) {
                        termcap_print_colored(TERMCAP_MAGENTA, TERMCAP_DEFAULT, "True color mode: ");
                        printf("OK Available\n");
                    }
                } else {
                    warning_message("Color support not available");
                }
                
                printf("\nEnhanced prompt example:\n");
                termcap_print_colored(TERMCAP_CYAN, TERMCAP_DEFAULT, "lusush");
                printf(":");
                termcap_print_colored(TERMCAP_BLUE, TERMCAP_DEFAULT, "~/project");
                printf(" ");
                termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "$");
                printf(" ");
                termcap_reset_all_formatting();
                printf("\n");
                
            } else {
                warning_message("No terminal detected - running in non-interactive mode");
            }
            
            return 0;
        } else if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("termcap - Enhanced terminal capability testing and demonstration\n\n");
            printf("Usage: termcap [option]\n\n");
            printf("Options:\n");
            printf("  test         Run comprehensive test suite\n");
            printf("  demo         Run interactive demonstration\n");
            printf("  capabilities Show terminal capabilities (default)\n");
            printf("  integration  Show enhanced shell integration demo\n");
            printf("  help         Show this help message\n\n");
            printf("The termcap command showcases lusush's enhanced terminal handling,\n");
            printf("including colors, cursor control, and advanced features.\n");
            return 0;
        } else {
            error_message("termcap: unknown option '%s'", argv[1]);
            error_message("Run 'termcap help' for usage information");
            return 1;
        }
    }
    
    error_message("termcap: too many arguments");
    error_message("Run 'termcap help' for usage information");
    return 1;
}

/**
 * bin_clear:
 *      Clear the screen.
 */
int bin_clear(int argc __attribute__((unused)),
              char **argv __attribute__((unused))) {
    display_integration_clear_screen();
    return 0;
}

/**
 * bin_unset:
 *       Remove an entry from global symbol table.
 */
int bin_unset(int argc __attribute__((unused)),
              char **argv __attribute__((unused))) {
    if (argc != 2) {
        error_message("usage: unset var");
        return 1;
    }

    // Use legacy API function for unsetting variables
    symtable_unset_global(argv[1]);
    return 0;
}

/**
 * bin_type:
 *      Display the type of a command (builtin, function, file, alias, etc.)
 */
int bin_type(int argc, char **argv) {
    bool type_only = false;   // -t flag: output only the type
    bool path_only = false;   // -p flag: output only the path
    bool show_all = false;    // -a flag: show all locations
    int name_start = 1;
    
    // Parse options
    for (int i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            type_only = true;
            name_start = i + 1;
        } else if (strcmp(argv[i], "-p") == 0) {
            path_only = true;
            name_start = i + 1;
        } else if (strcmp(argv[i], "-a") == 0) {
            show_all = true;
            name_start = i + 1;
        } else if (strcmp(argv[i], "--") == 0) {
            name_start = i + 1;
            break;
        } else {
            error_message("type: invalid option: %s", argv[i]);
            return 1;
        }
    }
    
    if (name_start >= argc) {
        error_message("usage: type [-t] [-p] [-a] name [name ...]");
        return 1;
    }

    int result = 0;
    for (int i = name_start; i < argc; i++) {
        const char *name = argv[i];

        bool found_any = false;
        
        // Check if it's a builtin command
        if (is_builtin(name)) {
            found_any = true;
            if (type_only) {
                printf("builtin\n");
            } else if (path_only) {
                // -p flag: builtins have no path, so output nothing
            } else {
                printf("%s is a shell builtin\n", name);
            }
            if (!show_all) continue;
        }

        // Check if it's an alias (simplified - would need full alias parsing)
        char *alias_value = symtable_get_global("alias");
        if (alias_value) {
            found_any = true;
            if (type_only) {
                printf("alias\n");
            } else if (path_only) {
                // -p flag: aliases have no path, so output nothing
            } else {
                printf("%s is aliased\n", name);
            }
            if (!show_all) continue;
        }

        // Check if it's a function (stored in symbol table)
        char *func_value = symtable_get_global(name);
        if (func_value && strstr(func_value, "function")) {
            found_any = true;
            if (type_only) {
                printf("function\n");
            } else if (path_only) {
                // -p flag: functions have no path, so output nothing
            } else {
                printf("%s is a function\n", name);
            }
            if (!show_all) continue;
        }

        // Check if it's an executable file in PATH
        char *path_env = getenv("PATH");
        if (path_env) {
            char *path_copy = strdup(path_env);
            char *dir = strtok(path_copy, ":");
            bool found_in_path = false;

            while (dir != NULL) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, name);

                if (access(full_path, X_OK) == 0) {
                    found_any = true;
                    found_in_path = true;
                    
                    if (type_only) {
                        printf("file\n");
                    } else if (path_only) {
                        printf("%s\n", full_path);
                    } else {
                        printf("%s is %s\n", name, full_path);
                    }
                    
                    if (!show_all) break;
                }
                dir = strtok(NULL, ":");
            }

            free(path_copy);
            if (found_in_path && !show_all) {
                continue;
            }
        }

        // Not found anywhere
        if (!found_any) {
            if (type_only || path_only) {
                // For -t and -p, output nothing for not found commands
            } else {
                printf("%s: not found\n", name);
            }
            result = 1;
        }
    }

    return result;
}



/**
 * process_escape_sequences:
 *      Process escape sequences in a string.
 */
static char *process_escape_sequences(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) {
        return NULL;
    }

    const char *src = str;
    char *dst = result;

    while (*src) {
        if (*src == '\\' && *(src + 1)) {
            src++; // Skip the backslash
            switch (*src) {
            case 'n':
                *dst++ = '\n';
                break;
            case 't':
                *dst++ = '\t';
                break;
            case 'r':
                *dst++ = '\r';
                break;
            case 'b':
                *dst++ = '\b';
                break;
            case 'a':
                *dst++ = '\a';
                break;
            case 'v':
                *dst++ = '\v';
                break;
            case 'f':
                *dst++ = '\f';
                break;
            case '\\':
                *dst++ = '\\';
                break;
            case '"':
                *dst++ = '"';
                break;
            case '\'':
                *dst++ = '\'';
                break;
            default:
                *dst++ = '\\';
                *dst++ = *src;
                break;
            }
        } else {
            *dst++ = *src;
        }
        src++;
    }

    *dst = '\0';
    return result;
}

/**
 * bin_echo:
 *      Echo arguments to stdout with escape sequence processing.
 */
int bin_echo(int argc, char **argv) {
    bool interpret_escapes = false; // POSIX: echo does not interpret escapes by default
    bool no_newline = false;
    int arg_start = 1;

    // Parse options
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0) {
            interpret_escapes = true;
            arg_start = i + 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            no_newline = true;
            arg_start = i + 1;
        } else if (strcmp(argv[i], "-E") == 0) {
            interpret_escapes = false;
            arg_start = i + 1;
        } else if (argv[i][0] == '-') {
            // Unknown option, treat as argument
            break;
        } else {
            break;
        }
    }

    // Print arguments
    for (int i = arg_start; i < argc; i++) {
        if (i > arg_start) {
            printf(" ");
        }

        if (interpret_escapes) {
            char *processed = process_escape_sequences(argv[i]);
            if (processed) {
                printf("%s", processed);
                free(processed);
            } else {
                printf("%s", argv[i]);
            }
        } else {
            printf("%s", argv[i]);
        }
    }

    if (!no_newline) {
        printf("\n");
    }

    return 0;
}

/**
 * bin_printf:
 *      Printf builtin with POSIX format specifier support.
 *      Handles width specifiers like %0100s for compatibility.
 */
int bin_printf(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "printf: usage: printf format [arguments ...]\n");
        return 1;
    }

    const char *format = argv[1];
    int arg_index = 2;

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%' && format[i + 1] != '\0') {
            i++; // Skip the %

            // Handle literal %
            if (format[i] == '%') {
                putchar('%');
                continue;
            }

            // Parse flags, width, precision
            int width = 0;
            int precision = -1;
            bool zero_pad = false;
            bool left_align = false;

            // Parse flags
            while (format[i] == '-' || format[i] == '+' || format[i] == ' ' ||
                   format[i] == '#' || format[i] == '0') {
                if (format[i] == '0') {
                    zero_pad = true;
                }
                if (format[i] == '-') {
                    left_align = true;
                }
                i++;
            }

            // Parse width
            if (format[i] == '*') {
                // Dynamic width from argument
                if (arg_index < argc) {
                    width = atoi(argv[arg_index]);
                    if (width < 0) {
                        left_align = true;
                        width = -width;
                    }
                    arg_index++;
                }
                i++;
            } else if (isdigit(format[i])) {
                while (isdigit(format[i])) {
                    width = width * 10 + (format[i] - '0');
                    i++;
                }
            }

            // Parse precision
            if (format[i] == '.') {
                i++;
                if (format[i] == '*') {
                    // Dynamic precision from argument
                    if (arg_index < argc) {
                        precision = atoi(argv[arg_index]);
                        arg_index++;
                    }
                    i++;
                } else {
                    precision = 0;
                    while (isdigit(format[i])) {
                        precision = precision * 10 + (format[i] - '0');
                        i++;
                    }
                }
            }

            // Handle conversion specifier
            char specifier = format[i];
            // Get the argument for the format specifier (after width/precision parsing)
            const char *format_arg = (arg_index < argc) ? argv[arg_index] : "";

            switch (specifier) {
            case 's': {
                // String format
                int len = strlen(format_arg);
                int padding = (width > len) ? width - len : 0;

                if (!left_align && padding > 0) {
                    // Right-align with padding
                    char pad_char = zero_pad ? '0' : ' ';
                    for (int p = 0; p < padding; p++) {
                        putchar(pad_char);
                    }
                }

                // Print the string (truncated if precision specified)
                if (precision >= 0 && precision < len) {
                    for (int j = 0; j < precision; j++) {
                        putchar(format_arg[j]);
                    }
                } else {
                    fputs(format_arg, stdout);
                }

                if (left_align && padding > 0) {
                    // Left-align with padding
                    for (int p = 0; p < padding; p++) {
                        putchar(' ');
                    }
                }

                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'd':
            case 'i': {
                // Integer format
                int value = (arg_index < argc) ? atoi(format_arg) : 0;
                int effective_width = left_align ? -width : width;
                printf("%*d", effective_width, value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'c': {
                // Character format
                int value = (arg_index < argc) ? atoi(format_arg) : 0;
                
                if (!left_align && width > 1) {
                    // Right-align with padding
                    for (int p = 1; p < width; p++) {
                        putchar(' ');
                    }
                }
                
                putchar(value);
                
                if (left_align && width > 1) {
                    // Left-align with padding
                    for (int p = 1; p < width; p++) {
                        putchar(' ');
                    }
                }
                
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'x':
            case 'X': {
                // Hexadecimal format
                unsigned int value = (arg_index < argc)
                                         ? (unsigned int)strtoul(format_arg, NULL, 10)
                                         : 0;
                int effective_width = left_align ? -width : width;
                printf(specifier == 'x' ? "%*x" : "%*X", effective_width, value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'o': {
                // Octal format
                unsigned int value = (arg_index < argc)
                                         ? (unsigned int)strtoul(format_arg, NULL, 10)
                                         : 0;
                int effective_width = left_align ? -width : width;
                printf("%*o", effective_width, value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'u': {
                // Unsigned integer format
                unsigned int value = (arg_index < argc)
                                         ? (unsigned int)strtoul(format_arg, NULL, 10)
                                         : 0;
                int effective_width = left_align ? -width : width;
                printf("%*u", effective_width, value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'f':
            case 'F': {
                // Float format
                double value = (arg_index < argc) ? strtod(format_arg, NULL) : 0.0;
                int effective_width = left_align ? -width : width;
                if (precision >= 0) {
                    printf("%*.*f", effective_width, precision, value);
                } else {
                    printf("%*f", effective_width, value);
                }
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'g':
            case 'G': {
                // General float format
                double value = (arg_index < argc) ? strtod(format_arg, NULL) : 0.0;
                int effective_width = left_align ? -width : width;
                if (precision >= 0) {
                    printf(specifier == 'g' ? "%*.*g" : "%*.*G", effective_width,
                           precision, value);
                } else {
                    printf(specifier == 'g' ? "%*g" : "%*G", effective_width, value);
                }
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'e':
            case 'E': {
                // Scientific notation
                double value = (arg_index < argc) ? strtod(format_arg, NULL) : 0.0;
                int effective_width = left_align ? -width : width;
                if (precision >= 0) {
                    printf(specifier == 'e' ? "%*.*e" : "%*.*E", effective_width,
                           precision, value);
                } else {
                    printf(specifier == 'e' ? "%*e" : "%*E", effective_width, value);
                }
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            default:
                // Unknown specifier - just print as is
                putchar('%');
                putchar(specifier);
                break;
            }
        } else if (format[i] == '\\' && format[i + 1] != '\0') {
            // Handle escaped characters
            i++;
            switch (format[i]) {
            case 'n':
                putchar('\n');
                break;
            case 't':
                putchar('\t');
                break;
            case 'r':
                putchar('\r');
                break;
            case 'b':
                putchar('\b');
                break;
            case 'f':
                putchar('\f');
                break;
            case 'a':
                putchar('\a');
                break;
            case 'v':
                putchar('\v');
                break;
            case '\\':
                putchar('\\');
                break;
            case '"':
                putchar('"');
                break;
            case '\'':
                putchar('\'');
                break;
            default:
                // Unknown escape - print as literal
                putchar('\\');
                putchar(format[i]);
                break;
            }
        } else {
            // Regular character
            putchar(format[i]);
        }
    }

    return 0;
}

/**
 * is_valid_identifier:
 *      Check if a string is a valid shell variable identifier.
 */
static int is_valid_identifier(const char *name) {
    if (!name || !*name) {
        return 0;
    }

    // First character must be letter or underscore
    if (!isalpha(*name) && *name != '_') {
        return 0;
    }

    // Subsequent characters must be alphanumeric or underscore
    for (const char *p = name + 1; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            return 0;
        }
    }

    return 1;
}

/**
 * bin_export:
 *      Export shell variables to environment.
 */
int bin_export(int argc, char **argv) {
    if (argc == 1) {
        // Print all exported variables
        extern char **environ;
        for (char **env = environ; *env; env++) {
            printf("export %s\n", *env);
        }
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (eq) {
            // Variable assignment: VAR=value
            size_t name_len = eq - argv[i];
            char *name = malloc(name_len + 1);
            if (!name) {
                error_message("export: memory allocation failed");
                return 1;
            }
            strncpy(name, argv[i], name_len);
            name[name_len] = '\0';

            const char *value = eq + 1;

            // Validate variable name
            if (!is_valid_identifier(name)) {
                error_message("export: invalid variable name: %s", name);
                free(name);
                return 1;
            }

            // Set variable value using modern API
            symtable_set_global(name, value);

            // Export the variable using modern API
            symtable_export_global(name);

            free(name);
        } else if (i + 2 < argc && strcmp(argv[i + 1], "=") == 0) {
            // Handle tokenized assignment: VAR = value
            const char *name = argv[i];
            const char *value = argv[i + 2];

            // Validate variable name
            if (!is_valid_identifier(name)) {
                error_message("export: invalid variable name: %s", name);
                return 1;
            }

            // Set variable value using modern API
            symtable_set_global(name, value);

            // Export the variable using modern API
            symtable_export_global(name);

            // Skip the = and value tokens
            i += 2;
        } else {
            // Just export existing variable
            if (!is_valid_identifier(argv[i])) {
                error_message("export: '%s' not a valid identifier", argv[i]);
                return 1;
            }

            // Check if variable exists and get its value
            char *current_value = symtable_get_global(argv[i]);
            if (current_value) {
                // Variable exists - just export it
                symtable_export_global(argv[i]);
            } else {
                // Variable doesn't exist - create with empty value and export
                symtable_set_global(argv[i], "");
                symtable_export_global(argv[i]);
            }
        }
    }

    return 0;
}

/**
 * bin_source:
 *      Source (execute) a script file.
 */
int bin_source(int argc, char **argv) {
    if (argc < 2) {
        error_message("source: usage: source filename");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        error_message("source: cannot open '%s'", argv[1]);
        return 1;
    }

    // Get global executor for script context tracking
    executor_t *executor = get_global_executor();
    if (!executor) {
        fclose(file);
        error_message("source: no execution context available");
        return 1;
    }

    // Set script execution context for debugging
    executor_set_script_context(executor, argv[1], 1);

    extern char *get_input_complete(FILE *in);
    char *complete_input;
    int result = 0;
    int construct_number = 1;

    // Read complete multi-line constructs instead of line by line
    while ((complete_input = get_input_complete(file)) != NULL) {
        // Skip empty constructs
        char *trimmed = complete_input;
        while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n') trimmed++;
        if (*trimmed == '\0') {
            free(complete_input);
            construct_number++;
            continue;
        }

        // Update script context for debugging
        executor_set_script_context(executor, argv[1], construct_number);

        // Parse and execute the complete construct
        int construct_result = parse_and_execute(complete_input);
        if (construct_result != 0) {
            result = construct_result;
        }

        free(complete_input);
        construct_number++;
    }

    // Clear script execution context
    executor_clear_script_context(executor);

    fclose(file);
    return result;
}

// Forward declarations for logical operator support
static int evaluate_test_expression(char **argv, int start, int end);
static int evaluate_single_test(char **argv, int start, int end);

/**
 * bin_test:
 *      Enhanced POSIX-compliant test expressions with logical operators.
 *      Supports negation (!), logical AND (-a), and logical OR (-o).
 */
int bin_test(int argc, char **argv) {
    if (argc == 1) {
        return 1; // False if no arguments
    }

    // Handle closing bracket for '[' command
    if (strcmp(argv[0], "[") == 0) {
        if (argc < 2 || strcmp(argv[argc - 1], "]") != 0) {
            error_message("test: '[' command missing closing ']'");
            return 2;
        }
        argc--; // Remove the closing bracket
    }

    // Use enhanced evaluation with logical operators
    return evaluate_test_expression(argv, 1, argc);
}

// Recursive evaluation of test expressions with logical operators
static int evaluate_test_expression(char **argv, int start, int end) {
    if (start >= end) {
        return 1; // Empty expression is false
    }

    // Handle negation operator
    if (start < end && strcmp(argv[start], "!") == 0) {
        int result = evaluate_test_expression(argv, start + 1, end);
        return (result == 0) ? 1 : 0; // Flip the result
    }

    // Find logical operators (-o has lower precedence than -a)
    // First pass: look for -o (OR)
    for (int i = start + 1; i < end - 1; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            int left = evaluate_test_expression(argv, start, i);
            int right = evaluate_test_expression(argv, i + 1, end);
            return (left == 0 || right == 0) ? 0 : 1;
        }
    }

    // Second pass: look for -a (AND)
    for (int i = start + 1; i < end - 1; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            int left = evaluate_test_expression(argv, start, i);
            int right = evaluate_test_expression(argv, i + 1, end);
            return (left == 0 && right == 0) ? 0 : 1;
        }
    }

    // No logical operators found, evaluate as single test
    return evaluate_single_test(argv, start, end);
}

// Evaluate a single test condition (unary or binary operators)
static int evaluate_single_test(char **argv, int start, int end) {
    int argc = end - start;

    // Simple test implementations
    if (argc == 1) {
        // test STRING - true if string is non-empty
        return (strlen(argv[start]) > 0) ? 0 : 1;
    }

    if (argc == 2) {
        if (strcmp(argv[start], "-z") == 0) {
            // test -z STRING - true if string is empty
            return (strlen(argv[start + 1]) == 0) ? 0 : 1;
        } else if (strcmp(argv[start], "-n") == 0) {
            // test -n STRING - true if string is non-empty
            return (strlen(argv[start + 1]) > 0) ? 0 : 1;
        } else if (strcmp(argv[start], "-f") == 0) {
            // test -f FILE - true if file exists and is regular
            struct stat st;
            return (stat(argv[start + 1], &st) == 0 && S_ISREG(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[start], "-d") == 0) {
            // test -d DIR - true if directory exists
            struct stat st;
            return (stat(argv[start + 1], &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[start], "-e") == 0) {
            // test -e PATH - true if path exists
            struct stat st;
            return (stat(argv[start + 1], &st) == 0) ? 0 : 1;
        } else if (strcmp(argv[start], "-c") == 0) {
            // test -c FILE - true if file is character device
            struct stat st;
            return (stat(argv[start + 1], &st) == 0 && S_ISCHR(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[start], "-b") == 0) {
            // test -b FILE - true if file is block device
            struct stat st;
            return (stat(argv[start + 1], &st) == 0 && S_ISBLK(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[start], "-L") == 0 || strcmp(argv[start], "-h") == 0) {
            // test -L FILE or -h FILE - true if file is symbolic link
            struct stat st;
            return (lstat(argv[start + 1], &st) == 0 && S_ISLNK(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[start], "-p") == 0) {
            // test -p FILE - true if file is named pipe (FIFO)
            struct stat st;
            return (stat(argv[start + 1], &st) == 0 && S_ISFIFO(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[start], "-S") == 0) {
            // test -S FILE - true if file is socket
            struct stat st;
            return (stat(argv[start + 1], &st) == 0 && S_ISSOCK(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[start], "-r") == 0) {
            // test -r FILE - true if file is readable
            return (access(argv[start + 1], R_OK) == 0) ? 0 : 1;
        } else if (strcmp(argv[start], "-w") == 0) {
            // test -w FILE - true if file is writable
            return (access(argv[start + 1], W_OK) == 0) ? 0 : 1;
        } else if (strcmp(argv[start], "-x") == 0) {
            // test -x FILE - true if file is executable
            return (access(argv[start + 1], X_OK) == 0) ? 0 : 1;
        } else if (strcmp(argv[start], "-s") == 0) {
            // test -s FILE - true if file exists and is not empty
            struct stat st;
            return (stat(argv[start + 1], &st) == 0 && st.st_size > 0) ? 0 : 1;
        }
    }

    if (argc == 3) {
        if (strcmp(argv[start + 1], "=") == 0) {
            // test STRING1 = STRING2
            return (strcmp(argv[start], argv[start + 2]) == 0) ? 0 : 1;
        } else if (strcmp(argv[start + 1], "!=") == 0) {
            // test STRING1 != STRING2
            return (strcmp(argv[start], argv[start + 2]) != 0) ? 0 : 1;
        } else if (strcmp(argv[start + 1], "-eq") == 0) {
            // test NUM1 -eq NUM2
            int n1 = atoi(argv[start]);
            int n2 = atoi(argv[start + 2]);
            return (n1 == n2) ? 0 : 1;
        } else if (strcmp(argv[start + 1], "-ne") == 0) {
            // test NUM1 -ne NUM2
            int n1 = atoi(argv[start]);
            int n2 = atoi(argv[start + 2]);
            return (n1 != n2) ? 0 : 1;
        } else if (strcmp(argv[start + 1], "-lt") == 0) {
            // test NUM1 -lt NUM2
            int n1 = atoi(argv[start]);
            int n2 = atoi(argv[start + 2]);
            return (n1 < n2) ? 0 : 1;
        } else if (strcmp(argv[start + 1], "-le") == 0) {
            // test NUM1 -le NUM2
            int n1 = atoi(argv[start]);
            int n2 = atoi(argv[start + 2]);
            return (n1 <= n2) ? 0 : 1;
        } else if (strcmp(argv[start + 1], "-gt") == 0) {
            // test NUM1 -gt NUM2
            int n1 = atoi(argv[start]);
            int n2 = atoi(argv[start + 2]);
            return (n1 > n2) ? 0 : 1;
        } else if (strcmp(argv[start + 1], "-ge") == 0) {
            // test NUM1 -ge NUM2
            int n1 = atoi(argv[start]);
            int n2 = atoi(argv[start + 2]);
            return (n1 >= n2) ? 0 : 1;
        }
    }

    error_message("test: unknown test condition or invalid arguments");
    return 2;
}

/**
 * bin_read:
 *      Enhanced POSIX-compliant read user input into variables.
 *      Supports -p (prompt), -r (raw), -t (timeout), -n (nchars), -s (silent) options.
 *      Leverages existing input.c infrastructure.
 */
int bin_read(int argc, char **argv) {
    // Option flags
    char *prompt = NULL;
    bool raw_mode = false;
    int timeout = -1;
    int nchars = -1;
    bool silent_mode = false;
    
    int opt_index = 1;
    
    // Parse options
    while (opt_index < argc && argv[opt_index][0] == '-') {
        char *arg = argv[opt_index];
        
        if (strcmp(arg, "-p") == 0) {
            // -p prompt: Display prompt before reading
            if (opt_index + 1 >= argc) {
                error_message("read: -p requires a prompt string");
                return 1;
            }
            prompt = argv[++opt_index];
        } else if (strcmp(arg, "-r") == 0) {
            // -r: Raw mode (don't interpret backslashes)
            raw_mode = true;
        } else if (strcmp(arg, "-t") == 0) {
            // -t timeout: Timeout after specified seconds (not implemented yet)
            if (opt_index + 1 >= argc) {
                error_message("read: -t requires a timeout value");
                return 1;
            }
            timeout = atoi(argv[++opt_index]);
            if (timeout < 0) {
                error_message("read: invalid timeout value");
                return 1;
            }
            // TODO: Implement timeout functionality
        } else if (strcmp(arg, "-n") == 0) {
            // -n nchars: Read only specified number of characters (not implemented yet)
            if (opt_index + 1 >= argc) {
                error_message("read: -n requires a character count");
                return 1;
            }
            nchars = atoi(argv[++opt_index]);
            if (nchars <= 0) {
                error_message("read: invalid character count");
                return 1;
            }
            // TODO: Implement nchars functionality
        } else if (strcmp(arg, "-s") == 0) {
            // -s: Silent mode (don't echo input) - not implemented yet
            silent_mode = true;
            // TODO: Implement silent mode
        } else if (strcmp(arg, "--") == 0) {
            // End of options
            opt_index++;
            break;
        } else {
            error_message("read: invalid option: %s", arg);
            return 1;
        }
        opt_index++;
    }
    
    // Must have at least one variable name
    if (opt_index >= argc) {
        error_message("read: usage: read [-p prompt] [-r] variable_name");
        return 1;
    }

    // Validate variable name
    char *varname = argv[opt_index];
    if (!is_valid_identifier(varname)) {
        error_message("read: '%s' not a valid identifier", varname);
        return 1;
    }

    // Display prompt if specified
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    
    // Use existing input infrastructure
    char *line = get_input(stdin);
    
    if (!line) {
        // EOF or input error
        return 1;
    }
    
    // Process backslashes unless in raw mode
    if (!raw_mode && line) {
        char *processed = malloc(strlen(line) + 1);
        if (processed) {
            int j = 0;
            for (int i = 0; line[i]; i++) {
                if (line[i] == '\\' && line[i + 1]) {
                    // Process escape sequences
                    i++; // Skip the backslash
                    switch (line[i]) {
                        case 'n': processed[j++] = '\n'; break;
                        case 't': processed[j++] = '\t'; break;
                        case 'r': processed[j++] = '\r'; break;
                        case '\\': processed[j++] = '\\'; break;
                        default: 
                            processed[j++] = '\\';
                            processed[j++] = line[i];
                            break;
                    }
                } else {
                    processed[j++] = line[i];
                }
            }
            processed[j] = '\0';
            free(line);
            line = processed;
        }
    }

    // Set the variable using modern API
    symtable_set_global(varname, line ? line : "");

    if (line) free(line);
    return 0;

    // Suppress unused variable warnings for features not yet implemented
    (void)timeout;
    (void)nchars;
    (void)silent_mode;
}

/**
 * bin_eval:
 *      Evaluate arguments as shell commands.
 */
int bin_eval(int argc, char **argv) {
    if (argc < 2) {
        return 0;
    }

    // Concatenate all arguments
    size_t total_len = 0;
    for (int i = 1; i < argc; i++) {
        total_len += strlen(argv[i]) + 1; // +1 for space
    }

    char *command = malloc(total_len);
    if (!command) {
        return 1;
    }

    command[0] = '\0';
    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            strcat(command, " ");
        }
        strcat(command, argv[i]);
    }

    // Execute the command string
    int result = parse_and_execute(command);

    free(command);
    return result;
}

/**
 * is_builtin:
 *      Check if a command name is a builtin command.
 */
bool is_builtin(const char *name) {
    for (size_t i = 0; i < builtins_count; i++) {
        if (strcmp(name, builtins[i].name) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * bin_true:
 *      Always return success (exit status 0)
 */
int bin_true(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

/**
 * bin_false:
 *      Always return failure (exit status 1)
 */
int bin_false(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 1;
}

/**
 * bin_set:
 *      Manage shell options and behavior flags
 */
int bin_set(int argc, char **argv) { return builtin_set(argv); }

// Global executor pointer for job control builtins
extern executor_t *current_executor;

/**
 * bin_jobs:
 *      List active jobs
 */
int bin_jobs(int argc, char **argv) {
    if (current_executor) {
        return executor_builtin_jobs(current_executor, argv);
    }
    return 1;
}

/**
 * bin_fg:
 *      Bring job to foreground
 */
int bin_fg(int argc, char **argv) {
    if (current_executor) {
        return executor_builtin_fg(current_executor, argv);
    }
    fprintf(stderr, "fg: no current job\n");
    return 1;
}

/**
 * bin_bg:
 *      Send job to background
 */
int bin_bg(int argc, char **argv) {
    if (current_executor) {
        return executor_builtin_bg(current_executor, argv);
    }
    fprintf(stderr, "bg: no current job\n");
    return 1;
}

/**
 * bin_shift:
 *      Shift positional parameters left by n positions
 */
int bin_shift(int argc, char **argv) {
    int shift_count = 1; // Default shift by 1

    // Parse optional shift count argument
    if (argc > 1) {
        char *endptr;
        shift_count = strtol(argv[1], &endptr, 10);

        // Validate that the argument is a valid number
        if (*endptr != '\0' || shift_count < 0) {
            fprintf(stderr, "shift: %s: numeric argument required\n", argv[1]);
            return 1;
        }
    }

    // Get current positional parameters
    extern int shell_argc;
    extern char **shell_argv;

    // Calculate available parameters to shift (excluding script name at
    // argv[0])
    int available_params = shell_argc > 1 ? shell_argc - 1 : 0;

    // If shift count exceeds available parameters, limit to available count
    // This matches POSIX behavior - don't error, just shift what's available
    if (shift_count > available_params) {
        shift_count = available_params;
    }

    // Perform the shift by adjusting shell_argc and shell_argv
    if (shift_count > 0 && shell_argc > 1) {
        // Shift the argv array
        for (int i = 1; i < shell_argc - shift_count; i++) {
            shell_argv[i] = shell_argv[i + shift_count];
        }

        // Update argc to reflect the new parameter count
        shell_argc -= shift_count;

        // Update shell variables to reflect the new parameter count
        // Update $# (parameter count)
        char param_count_str[16];
        snprintf(param_count_str, sizeof(param_count_str), "%d",
                 shell_argc > 1 ? shell_argc - 1 : 0);
        symtable_set_global("#", param_count_str);

        // Update individual positional parameters ($1, $2, etc.)
        // Clear old parameters first
        for (int i = 1; i <= 9; i++) {
            char param_name[3];
            snprintf(param_name, sizeof(param_name), "%d", i);
            if (i < shell_argc && shell_argv[i]) {
                symtable_set_global(param_name, shell_argv[i]);
            } else {
                symtable_set_global(param_name, "");
            }
        }
    }

    return 0;
}
/**
 * bin_break:
 *      Break out of enclosing loop
 */
int bin_break(int argc, char **argv) {
    // Get the current executor to set loop control state
    extern executor_t *current_executor;

    if (!current_executor) {
        fprintf(stderr, "break: not currently in a loop\n");
        return 1;
    }

    // Check if we're actually in a loop
    if (current_executor->loop_depth <= 0) {
        fprintf(stderr, "break: not currently in a loop\n");
        return 1;
    }

    // Parse optional level argument (break n)
    int break_level = 1;
    if (argc > 1) {
        char *endptr;
        break_level = strtol(argv[1], &endptr, 10);

        if (*endptr != '\0' || break_level <= 0) {
            fprintf(stderr, "break: %s: numeric argument required\n", argv[1]);
            return 1;
        }

        if (break_level > current_executor->loop_depth) {
            fprintf(stderr,
                    "break: %d: cannot break %d levels (only %d nested)\n",
                    break_level, break_level, current_executor->loop_depth);
            return 1;
        }
    }

    // Set loop control state to break
    current_executor->loop_control = LOOP_BREAK;

    return 0;
}

/**
 * bin_continue:
 *      Continue to next iteration of enclosing loop
 */
int bin_continue(int argc, char **argv) {
    // Get the current executor to set loop control state
    extern executor_t *current_executor;

    if (!current_executor) {
        fprintf(stderr, "continue: not currently in a loop\n");
        return 1;
    }

    // Check if we're actually in a loop
    if (current_executor->loop_depth <= 0) {
        fprintf(stderr, "continue: not currently in a loop\n");
        return 1;
    }

    // Parse optional level argument (continue n)
    int continue_level = 1;
    if (argc > 1) {
        char *endptr;
        continue_level = strtol(argv[1], &endptr, 10);

        if (*endptr != '\0' || continue_level <= 0) {
            fprintf(stderr, "continue: %s: numeric argument required\n",
                    argv[1]);
            return 1;
        }

        if (continue_level > current_executor->loop_depth) {
            fprintf(
                stderr,
                "continue: %d: cannot continue %d levels (only %d nested)\n",
                continue_level, continue_level, current_executor->loop_depth);
            return 1;
        }
    }

    // Set loop control state to continue
    current_executor->loop_control = LOOP_CONTINUE;

    return 0;
}

/**
 * bin_return_value:
 *      Set a string return value for the current function
 */
int bin_return_value(int argc, char **argv) {
    // POSIX compliance: return_value is not available in strict POSIX mode
    if (is_posix_mode_enabled()) {
        fprintf(stderr, "return_value: not available in POSIX mode\n");
        return 1;
    }
    
    if (argc < 2) {
        fprintf(stderr, "return_value: missing value argument\n");
        return 1;
    }
    
    // Output the return value with a special marker that command substitution can recognize
    printf("__LUSUSH_RETURN__:%s:__END__\n", argv[1]);
    fflush(stdout);
    
    // Return success
    return 0;
}

/**
   * bin_return:
   *      Return from function with optional exit code
   */
int bin_return(int argc, char **argv) {
    int return_code = 0; // Default return code

    // Parse optional return code argument
    if (argc > 1) {
        char *endptr;
        return_code = strtol(argv[1], &endptr, 10);

        // Validate that the argument is a valid number
        if (*endptr != '\0') {
            fprintf(stderr, "return: %s: numeric argument required\n", argv[1]);
            return 1;
        }
    }

    // Set the last exit status to the return code
    extern int last_exit_status;
    last_exit_status = return_code;

    // Return a special exit code that the executor can recognize as "function
    // return" We'll use a specific value that doesn't conflict with normal exit
    // codes
    return 200 + (return_code & 0xFF); // 200-255 range for function returns
} /**
   * bin_trap:
   *      Set or display signal traps
   */
int bin_trap(int argc, char **argv) {
    // If no arguments, list all traps
    if (argc == 1) {
        list_traps();
        return 0;
    }

    // Handle special options
    if (argc == 2 && strcmp(argv[1], "-l") == 0) {
        // List available signals
        printf("EXIT  0) exit from shell\n");
        printf("HUP   1) hangup\n");
        printf("INT   2) interrupt\n");
        printf("QUIT  3) quit\n");
        printf("TERM  15) software termination signal\n");
        printf("USR1  10) user defined signal 1\n");
        printf("USR2  12) user defined signal 2\n");
        return 0;
    }

    // Parse arguments: trap [-l] [action] [signal ...]
    int arg_index = 1;

    // Skip -l option if present
    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        arg_index = 2;
    }

    // Need at least action argument
    if (arg_index >= argc) {
        fprintf(stderr, "trap: usage: trap [-l] [action] [signal ...]\n");
        return 1;
    }

    const char *action = argv[arg_index++];

    // If no signals specified, this is an error
    if (arg_index >= argc) {
        fprintf(stderr, "trap: usage: trap [-l] [action] [signal ...]\n");
        return 1;
    }

    // Process each signal
    for (int i = arg_index; i < argc; i++) {
        int signal = get_signal_number(argv[i]);

        if (signal < 0) {
            fprintf(stderr, "trap: %s: invalid signal specification\n",
                    argv[i]);
            return 1;
        }

        // Handle special cases
        if (strcmp(action, "-") == 0) {
            // Reset to default
            remove_trap(signal);
        } else if (strcmp(action, "") == 0 || strcmp(action, "\"\"") == 0) {
            // Ignore signal
            if (signal == 0) {
                // EXIT trap - remove it
                remove_trap(signal);
            } else {
                set_trap(signal, "");
            }
        } else {
            // Set trap command
            if (set_trap(signal, action) != 0) {
                fprintf(stderr, "trap: failed to set trap for signal %s\n",
                        argv[i]);
                return 1;
            }
        }
    }

    return 0;
} /**
   * bin_exec:
   *      Replace shell process with command or modify file descriptors
   */
int bin_exec(int argc, char **argv) {
    // Privileged mode security check
    if (shell_opts.privileged_mode) {
        fprintf(stderr, "exec: restricted command in privileged mode\n");
        return 1;
    }

    // If no arguments, exec does nothing and returns success
    if (argc == 1) {
        return 0;
    }

    // Check for redirection-only exec (exec < file, exec > file, etc.)
    bool has_redirections = false;
    bool has_command = false;

    // Scan arguments to determine if this is redirection-only or command exec
    for (int i = 1; i < argc; i++) {
        if (strchr(argv[i], '<') || strchr(argv[i], '>')) {
            has_redirections = true;
        } else if (argv[i][0] != '<' && argv[i][0] != '>' &&
                   !isdigit(argv[i][0])) {
            has_command = true;
            break;
        }
    }

    // If only redirections, handle file descriptor manipulation
    if (has_redirections && !has_command) {
        // TODO: Implement redirection-only exec
        // For now, we'll focus on command replacement exec
        fprintf(stderr, "exec: redirection-only exec not yet implemented\n");
        return 1;
    }

    // Command replacement exec - find the command to execute
    char *command = NULL;
    char **exec_argv = NULL;
    int exec_argc = 0;

    // Find the first non-redirection argument as the command
    int cmd_start = 1;
    while (cmd_start < argc &&
           (argv[cmd_start][0] == '<' || argv[cmd_start][0] == '>' ||
            isdigit(argv[cmd_start][0]))) {
        cmd_start++;
    }

    if (cmd_start >= argc) {
        fprintf(stderr, "exec: no command specified\n");
        return 1;
    }

    command = argv[cmd_start];
    exec_argc = argc - cmd_start;
    exec_argv = &argv[cmd_start];

    // Execute EXIT traps before replacing the process
    execute_exit_traps();

    // Flush all output streams before exec
    fflush(stdout);
    fflush(stderr);
    fflush(stdin);

    // Try to execute the command using execvp
    // This replaces the current process entirely
    execvp(command, exec_argv);

    // If we get here, exec failed
    perror("exec");

    // exec failure should exit the shell with error status
    exit(127);
} /**
   * bin_wait:
   *      Wait for background jobs to complete
   */
int bin_wait(int argc, char **argv) {
    // Get the current executor to access job control
    extern executor_t *current_executor;

    if (!current_executor) {
        // If no executor, there are no jobs to wait for
        return 0;
    }

    // If no arguments, wait for all background jobs
    if (argc == 1) {
        executor_update_job_status(current_executor);

        // Wait for all running jobs
        job_t *job = current_executor->jobs;
        int last_exit_status = 0;

        while (job) {
            if (job->state == JOB_RUNNING) {
                int status;
                pid_t result = waitpid(-job->pgid, &status, 0);

                if (result > 0) {
                    if (WIFEXITED(status)) {
                        last_exit_status = WEXITSTATUS(status);
                    } else if (WIFSIGNALED(status)) {
                        last_exit_status = 128 + WTERMSIG(status);
                    } else {
                        last_exit_status = 1;
                    }

                    // Mark job as done
                    job->state = JOB_DONE;
                }
            }
            job = job->next;
        }

        // Clean up completed jobs
        executor_update_job_status(current_executor);

        return last_exit_status;
    }

    // Wait for specific job(s) or process(es)
    int overall_exit_status = 0;

    for (int i = 1; i < argc; i++) {
        char *endptr;
        long target = strtol(argv[i], &endptr, 10);

        // Check for job ID syntax (%n)
        bool is_job_id = false;
        int job_or_pid = (int)target;

        if (argv[i][0] == '%') {
            is_job_id = true;
            // Re-parse without the % sign
            job_or_pid = (int)strtol(argv[i] + 1, &endptr, 10);
            if (*endptr != '\0' || job_or_pid <= 0) {
                fprintf(stderr, "wait: %s: not a valid job ID\n", argv[i]);
                return 1;
            }
        } else {
            if (*endptr != '\0' || target <= 0) {
                fprintf(stderr,
                        "wait: %s: arguments must be process or job IDs\n",
                        argv[i]);
                return 1;
            }
        }

        if (is_job_id) {
            // Wait for specific job
            job_t *job = executor_find_job(current_executor, job_or_pid);
            if (!job) {
                fprintf(stderr, "wait: %%%d: no such job\n", job_or_pid);
                return 127;
            }

            if (job->state == JOB_RUNNING) {
                int status;
                pid_t result = waitpid(-job->pgid, &status, 0);

                if (result > 0) {
                    if (WIFEXITED(status)) {
                        overall_exit_status = WEXITSTATUS(status);
                    } else if (WIFSIGNALED(status)) {
                        overall_exit_status = 128 + WTERMSIG(status);
                    } else {
                        overall_exit_status = 1;
                    }

                    job->state = JOB_DONE;
                }
            } else if (job->state == JOB_DONE) {
                // Job already completed - return 0
                overall_exit_status = 0;
            }
        } else {
            // Wait for specific PID
            int status;
            pid_t result = waitpid(job_or_pid, &status, 0);

            if (result == -1) {
                if (errno == ECHILD) {
                    // Process doesn't exist or not a child
                    fprintf(stderr,
                            "wait: pid %d is not a child of this shell\n",
                            job_or_pid);
                    return 127;
                } else {
                    perror("wait");
                    return 1;
                }
            } else if (result > 0) {
                if (WIFEXITED(status)) {
                    overall_exit_status = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    overall_exit_status = 128 + WTERMSIG(status);
                } else {
                    overall_exit_status = 1;
                }
            }
        }
    }

    // Clean up completed jobs
    executor_update_job_status(current_executor);

    return overall_exit_status;
}

/**
 * bin_umask:
 *      Set or display file creation mask
 */
int bin_umask(int argc, char **argv) {
    // If no arguments, display current umask
    if (argc == 1) {
        mode_t current_mask = umask(0); // Get current mask
        umask(current_mask);            // Restore it
        printf("%04o\n", current_mask);
        return 0;
    }

    // If one argument, set new umask
    if (argc == 2) {
        // Check for empty argument
        if (argv[1][0] == '\0') {
            fprintf(stderr, "umask: invalid mode\n");
            return 1;
        }

        char *endptr;
        long mask_val = strtol(argv[1], &endptr, 8); // Parse as octal

        // Validate argument
        if (*endptr != '\0' || mask_val < 0 || mask_val > 0777) {
            fprintf(stderr, "umask: %s: invalid mode\n", argv[1]);
            return 1;
        }

        umask((mode_t)mask_val);
        return 0;
    }

    // Too many arguments
    fprintf(stderr, "umask: too many arguments\n");
    return 1;
}

/**
 * bin_ulimit:
 *      Set or display resource limits
 */
int bin_ulimit(int argc, char **argv) {
    int opt;
    int resource = RLIMIT_FSIZE; // Default to file size limit
    bool show_all = false;
    bool hard_limit = false;
    char *limit_value = NULL;

    // Reset getopt for this call
    optind = 1;

    // Parse options
    while ((opt = getopt(argc, argv, "aHSfntsuvh")) != -1) {
        switch (opt) {
        case 'a':
            show_all = true;
            break;
        case 'H':
            hard_limit = true;
            break;
        case 'S':
            hard_limit = false; // Soft limit (default)
            break;
        case 'f':
            resource = RLIMIT_FSIZE;
            break;
        case 'n':
            resource = RLIMIT_NOFILE;
            break;
        case 't':
            resource = RLIMIT_CPU;
            break;
        case 's':
#ifdef RLIMIT_STACK
            resource = RLIMIT_STACK;
#else
            fprintf(stderr, "ulimit: -s not supported on this system\n");
            return 1;
#endif
            break;
        case 'u':
#ifdef RLIMIT_NPROC
            resource = RLIMIT_NPROC;
#else
            fprintf(stderr, "ulimit: -u not supported on this system\n");
            return 1;
#endif
            break;
        case 'v':
#ifdef RLIMIT_AS
            resource = RLIMIT_AS;
#else
            fprintf(stderr, "ulimit: -v not supported on this system\n");
            return 1;
#endif
            break;
        case 'h':
            printf("ulimit: set or display resource limits\n");
            printf("Options:\n");
            printf("  -a     Display all limits\n");
            printf("  -H     Set hard limit\n");
            printf("  -S     Set soft limit (default)\n");
            printf("  -f     File size limit (512-byte blocks)\n");
            printf("  -n     Number of open files\n");
            printf("  -t     CPU time limit (seconds)\n");
#ifdef RLIMIT_STACK
            printf("  -s     Stack size limit (1024-byte blocks)\n");
#endif
#ifdef RLIMIT_NPROC
            printf("  -u     Number of user processes\n");
#endif
#ifdef RLIMIT_AS
            printf("  -v     Virtual memory limit (1024-byte blocks)\n");
#endif
            return 0;
        default:
            fprintf(stderr, "ulimit: invalid option -%c\n", optopt);
            return 1;
        }
    }

    // Get remaining argument (limit value)
    if (optind < argc) {
        limit_value = argv[optind];
    }

    if (show_all) {
        // Display all limits
        struct rlimit rlim;

#ifdef RLIMIT_CORE
        printf("core file size          (blocks, -c) ");
        if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
            if (rlim.rlim_cur == RLIM_INFINITY) {
                printf("unlimited\n");
            } else {
                printf("%lu\n", (unsigned long)(rlim.rlim_cur / 512));
            }
        } else {
            printf("unknown\n");
        }
#endif

#ifdef RLIMIT_DATA
        printf("data seg size           (kbytes, -d) ");
        if (getrlimit(RLIMIT_DATA, &rlim) == 0) {
            if (rlim.rlim_cur == RLIM_INFINITY) {
                printf("unlimited\n");
            } else {
                printf("%lu\n", (unsigned long)(rlim.rlim_cur / 1024));
            }
        } else {
            printf("unknown\n");
        }
#endif

        printf("file size               (blocks, -f) ");
        if (getrlimit(RLIMIT_FSIZE, &rlim) == 0) {
            if (rlim.rlim_cur == RLIM_INFINITY) {
                printf("unlimited\n");
            } else {
                printf("%lu\n", (unsigned long)(rlim.rlim_cur / 512));
            }
        } else {
            printf("unknown\n");
        }

        printf("open files                    (-n) ");
        if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
            if (rlim.rlim_cur == RLIM_INFINITY) {
                printf("unlimited\n");
            } else {
                printf("%lu\n", (unsigned long)rlim.rlim_cur);
            }
        } else {
            printf("unknown\n");
        }

        printf("stack size              (kbytes, -s) ");
        if (getrlimit(RLIMIT_STACK, &rlim) == 0) {
            if (rlim.rlim_cur == RLIM_INFINITY) {
                printf("unlimited\n");
            } else {
                printf("%lu\n", (unsigned long)(rlim.rlim_cur / 1024));
            }
        } else {
            printf("unknown\n");
        }

        printf("cpu time               (seconds, -t) ");
        if (getrlimit(RLIMIT_CPU, &rlim) == 0) {
            if (rlim.rlim_cur == RLIM_INFINITY) {
                printf("unlimited\n");
            } else {
                printf("%lu\n", (unsigned long)rlim.rlim_cur);
            }
        } else {
            printf("unknown\n");
        }

#ifdef RLIMIT_NPROC
        printf("max user processes            (-u) ");
        if (getrlimit(RLIMIT_NPROC, &rlim) == 0) {
            if (rlim.rlim_cur == RLIM_INFINITY) {
                printf("unlimited\n");
            } else {
                printf("%lu\n", (unsigned long)rlim.rlim_cur);
            }
        } else {
            printf("unknown\n");
        }
#endif

#ifdef RLIMIT_AS
        printf("virtual memory          (kbytes, -v) ");
        if (getrlimit(RLIMIT_AS, &rlim) == 0) {
            if (rlim.rlim_cur == RLIM_INFINITY) {
                printf("unlimited\n");
            } else {
                printf("%lu\n", (unsigned long)(rlim.rlim_cur / 1024));
            }
        } else {
            printf("unknown\n");
        }
#endif

        return 0;
    }

    // Handle specific resource
    struct rlimit rlim;
    if (getrlimit(resource, &rlim) != 0) {
        perror("ulimit: getrlimit");
        return 1;
    }

    if (limit_value == NULL) {
        // Display current limit
        rlim_t current = hard_limit ? rlim.rlim_max : rlim.rlim_cur;
        if (current == RLIM_INFINITY) {
            printf("unlimited\n");
        } else {
            // Convert to appropriate units
            switch (resource) {
            case RLIMIT_FSIZE:
#ifdef RLIMIT_CORE
            case RLIMIT_CORE:
#endif
                printf("%lu\n",
                       (unsigned long)(current / 512)); // 512-byte blocks
                break;
#ifdef RLIMIT_STACK
            case RLIMIT_STACK:
#endif
#ifdef RLIMIT_DATA
            case RLIMIT_DATA:
#endif
#ifdef RLIMIT_AS
            case RLIMIT_AS:
                printf("%lu\n",
                       (unsigned long)(current / 1024)); // 1024-byte blocks
                break;
#endif
            default:
                printf("%lu\n", (unsigned long)current);
                break;
            }
        }
        return 0;
    }

    // Set new limit
    rlim_t new_limit;
    if (strcmp(limit_value, "unlimited") == 0) {
        new_limit = RLIM_INFINITY;
    } else {
        char *endptr;
        long val = strtol(limit_value, &endptr, 10);
        if (*endptr != '\0' || val < 0) {
            fprintf(stderr, "ulimit: %s: invalid limit\n", limit_value);
            return 1;
        }

        // Convert from display units to bytes
        switch (resource) {
        case RLIMIT_FSIZE:
#ifdef RLIMIT_CORE
        case RLIMIT_CORE:
#endif
            new_limit = val * 512; // 512-byte blocks
            break;
#ifdef RLIMIT_STACK
        case RLIMIT_STACK:
#endif
#ifdef RLIMIT_DATA
        case RLIMIT_DATA:
#endif
#ifdef RLIMIT_AS
        case RLIMIT_AS:
            new_limit = val * 1024; // 1024-byte blocks
            break;
#endif
        default:
            new_limit = val;
            break;
        }
    }

    // Set the limit
    if (hard_limit) {
        rlim.rlim_max = new_limit;
        // Can't set hard limit higher than current hard limit without
        // privileges
        if (new_limit > rlim.rlim_max) {
            rlim.rlim_cur = rlim.rlim_max;
        }
    } else {
        rlim.rlim_cur = new_limit;
        // Can't set soft limit higher than hard limit
        if (new_limit > rlim.rlim_max) {
            rlim.rlim_cur = rlim.rlim_max;
        }
    }

    if (setrlimit(resource, &rlim) != 0) {
        perror("ulimit: setrlimit");
        return 1;
    }

    return 0;
}

/**
 * bin_times:
 *      Display user and system times for shell and children
 */
int bin_times(int argc, char **argv) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning

    struct tms tms_buf;
    clock_t real_time;

    // Get process times
    real_time = times(&tms_buf);
    if (real_time == (clock_t)-1) {
        perror("times");
        return 1;
    }

    // Get clock ticks per second for conversion
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    if (ticks_per_sec <= 0) {
        ticks_per_sec = 100; // Default fallback
    }

    // Convert ticks to seconds and format output
    double user_time = (double)tms_buf.tms_utime / ticks_per_sec;
    double system_time = (double)tms_buf.tms_stime / ticks_per_sec;
    double child_user_time = (double)tms_buf.tms_cutime / ticks_per_sec;
    double child_system_time = (double)tms_buf.tms_cstime / ticks_per_sec;

    // Output in POSIX format: user_time system_time child_user_time
    // child_system_time
    printf("%.2dm%.3fs %.2dm%.3fs\n", (int)(user_time / 60),
           user_time - (int)(user_time / 60) * 60, (int)(system_time / 60),
           system_time - (int)(system_time / 60) * 60);
    printf("%.2dm%.3fs %.2dm%.3fs\n", (int)(child_user_time / 60),
           child_user_time - (int)(child_user_time / 60) * 60,
           (int)(child_system_time / 60),
           child_system_time - (int)(child_system_time / 60) * 60);

    return 0;
}

/**
 * bin_getopts:
 *      Parse command options for shell scripts
 */
int bin_getopts(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "getopts: usage: getopts optstring name [args...]\n");
        return 1;
    }

    char *optstring = argv[1];
    char *varname = argv[2];

    // Get current OPTIND value from environment
    char *optind_str = symtable_get_global("OPTIND");
    int current_optind = optind_str ? atoi(optind_str) : 1;

    // Determine arguments to parse
    char **parse_args;
    int parse_argc;

    if (argc > 3) {
        // Use provided arguments
        parse_args = &argv[3];
        parse_argc = argc - 3;
    } else {
        // Use positional parameters - get from shell environment
        // For now, use a simple implementation
        parse_args = NULL;
        parse_argc = 0;

        // Try to get positional parameters from shell variables
        char *argc_str = symtable_get_global("#");
        if (argc_str) {
            parse_argc = atoi(argc_str);
            if (parse_argc > 0) {
                parse_args = malloc((parse_argc + 1) * sizeof(char *));
                for (int i = 0; i < parse_argc; i++) {
                    char param_name[16];
                    snprintf(param_name, sizeof(param_name), "%d", i + 1);
                    char *param_val = symtable_get_global(param_name);
                    parse_args[i] = param_val ? strdup(param_val) : strdup("");
                }
                parse_args[parse_argc] = NULL;
            }
        }
    }

    // Check if we have arguments to parse
    if (parse_argc == 0 || current_optind > parse_argc) {
        // No more arguments - OPTIND should point to first non-option argument
        // Don't reset OPTIND to 1, leave it pointing to where non-options start
        if (argc <= 3 && parse_args) {
            for (int i = 0; i < parse_argc; i++) {
                free(parse_args[i]);
            }
            free(parse_args);
        }
        return 1;
    }

    // Get current argument to parse
    char *current_arg = parse_args[current_optind - 1];

    // Static variables to maintain state between calls
    static char *current_option_arg = NULL;
    static int option_pos = 0;

    // Check if we're continuing with a combined option (like -abc)
    if (option_pos == 0) {
        // Starting new argument
        if (!current_arg || current_arg[0] != '-' ||
            strcmp(current_arg, "-") == 0) {
            // Not an option or single dash - OPTIND should point to this
            // non-option Don't reset OPTIND to 1, it should point to current
            // position
            if (argc <= 3 && parse_args) {
                for (int i = 0; i < parse_argc; i++) {
                    free(parse_args[i]);
                }
                free(parse_args);
            }
            return 1;
        }

        if (strcmp(current_arg, "--") == 0) {
            // End of options marker
            char next_optind[16];
            snprintf(next_optind, sizeof(next_optind), "%d",
                     current_optind + 1);
            symtable_set_global("OPTIND", next_optind);
            if (argc <= 3 && parse_args) {
                for (int i = 0; i < parse_argc; i++) {
                    free(parse_args[i]);
                }
                free(parse_args);
            }
            return 1;
        }

        current_option_arg = current_arg;
        option_pos = 1; // Skip the initial '-'
    }

    // Get current option character
    char opt_char = current_option_arg[option_pos];
    if (opt_char == '\0') {
        // Move to next argument
        current_optind++;
        option_pos = 0;
        char next_optind[16];
        snprintf(next_optind, sizeof(next_optind), "%d", current_optind);
        symtable_set_global("OPTIND", next_optind);
        if (argc <= 3 && parse_args) {
            for (int i = 0; i < parse_argc; i++) {
                free(parse_args[i]);
            }
            free(parse_args);
        }
        return bin_getopts(argc,
                           argv); // Recursive call to process next argument
    }

    // Check if option is valid
    bool silent_mode = (optstring[0] == ':');
    char *opt_pos = strchr(silent_mode ? optstring + 1 : optstring, opt_char);

    if (!opt_pos) {
        // Invalid option
        if (silent_mode) {
            symtable_set_global(varname, "?");
            char optarg_val[2] = {opt_char, '\0'};
            symtable_set_global("OPTARG", optarg_val);
        } else {
            fprintf(stderr, "getopts: illegal option -- %c\n", opt_char);
            symtable_set_global(varname, "?");
            symtable_set_global("OPTARG", "");
        }
        option_pos++;

        // If we've finished processing this argument, move to next
        if (current_option_arg[option_pos] == '\0') {
            current_optind++;
            option_pos = 0;
        }

        char next_optind[16];
        snprintf(next_optind, sizeof(next_optind), "%d", current_optind);
        symtable_set_global("OPTIND", next_optind);
        if (argc <= 3 && parse_args) {
            for (int i = 0; i < parse_argc; i++) {
                free(parse_args[i]);
            }
            free(parse_args);
        }
        return 0;
    }

    // Check if option requires an argument
    bool needs_arg = (opt_pos[1] == ':');

    if (needs_arg) {
        char *arg_value = NULL;

        if (current_option_arg[option_pos + 1] != '\0') {
            // Argument is attached (like -fvalue)
            arg_value = &current_option_arg[option_pos + 1];
            option_pos = 0; // Move to next argument
            current_optind++;
        } else {
            // Argument should be in next parameter
            if (current_optind < parse_argc) {
                arg_value = parse_args[current_optind];
                current_optind++; // Move past the option argument
                option_pos = 0;
            } else {
                // Missing argument
                if (silent_mode) {
                    symtable_set_global(varname, ":");
                    char optarg_val[2] = {opt_char, '\0'};
                    symtable_set_global("OPTARG", optarg_val);
                } else {
                    fprintf(stderr,
                            "getopts: option requires an argument -- %c\n",
                            opt_char);
                    symtable_set_global(varname, "?");
                    symtable_set_global("OPTARG", "");
                }
                char next_optind[16];
                snprintf(next_optind, sizeof(next_optind), "%d",
                         current_optind);
                symtable_set_global("OPTIND", next_optind);
                if (argc <= 3 && parse_args) {
                    for (int i = 0; i < parse_argc; i++) {
                        free(parse_args[i]);
                    }
                    free(parse_args);
                }
                return 0;
            }
        }

        // Set option and argument
        char opt_val[2] = {opt_char, '\0'};
        symtable_set_global(varname, opt_val);
        symtable_set_global("OPTARG", arg_value ? arg_value : "");

        // For options with arguments, we need to increment current_optind again
        // since we consumed both the option and its argument
        current_optind++;
    } else {
        // Option doesn't take an argument
        char opt_val[2] = {opt_char, '\0'};
        symtable_set_global(varname, opt_val);
        symtable_set_global("OPTARG", "");
        option_pos++;

        // If we've finished processing this argument, move to next
        if (current_option_arg[option_pos] == '\0') {
            current_optind++;
            option_pos = 0;
        }
    }

    // Update OPTIND
    char next_optind[16];
    snprintf(next_optind, sizeof(next_optind), "%d", current_optind);
    symtable_set_global("OPTIND", next_optind);

    // Clean up if we allocated parse_args
    if (argc <= 3 && parse_args) {
        for (int i = 0; i < parse_argc; i++) {
            free(parse_args[i]);
        }
        free(parse_args);
    }

    return 0; // Found an option
}

/**
 * bin_local:
 *      Declare local variables within function scope.
 *      Usage: local [name[=value] ...]
 */
int bin_local(int argc, char **argv) {
    if (argc == 1) {
        // No arguments - just return success (bash behavior)
        return 0;
    }

    // Get the current symbol table manager
    symtable_manager_t *manager = symtable_get_global_manager();
    if (!manager) {
        error_message("local: symbol table not available");
        return 1;
    }

    // Check if we're in a function scope
    size_t current_level = symtable_current_level(manager);
    if (current_level == 0) {
        error_message("local: can only be used in a function");
        return 1;
    }

    // Process each argument
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        char *eq = strchr(arg, '=');

        if (eq) {
            // Assignment: local var=value
            size_t name_len = eq - arg;
            char *name = malloc(name_len + 1);
            if (!name) {
                error_message("local: memory allocation failed");
                return 1;
            }

            strncpy(name, arg, name_len);
            name[name_len] = '\0';

            // Validate variable name
            if (!name[0] || (!isalpha(name[0]) && name[0] != '_')) {
                error_message("local: invalid variable name");
                free(name);
                return 1;
            }

            for (size_t j = 1; j < name_len; j++) {
                if (!isalnum(name[j]) && name[j] != '_') {
                    error_message("local: invalid variable name");
                    free(name);
                    return 1;
                }
            }

            // Set the local variable
            char *value = eq + 1;
            if (symtable_set_local_var(manager, name, value) != 0) {
                error_message("local: failed to set variable");
                free(name);
                return 1;
            }

            free(name);
        } else {
            // Declaration only: local var
            // Validate variable name
            if (!arg[0] || (!isalpha(arg[0]) && arg[0] != '_')) {
                error_message("local: invalid variable name");
                return 1;
            }

            for (size_t j = 1; arg[j]; j++) {
                if (!isalnum(arg[j]) && arg[j] != '_') {
                    error_message("local: invalid variable name");
                    return 1;
                }
            }

            // Declare the local variable with empty value
            if (symtable_set_local_var(manager, arg, "") != 0) {
                error_message("local: failed to declare variable");
                return 1;
            }
        }
    }

    return 0;
}

/**
 * bin_readonly:
 *      Create read-only variables according to POSIX standards
 *      Usage: readonly [name[=value] ...]
 */
int bin_readonly(int argc, char **argv) {
    if (argc == 1) {
        // No arguments - print all readonly variables
        symtable_manager_t *manager = symtable_get_global_manager();
        if (!manager) {
            error_message("readonly: symbol table not available");
            return 1;
        }

        // Print readonly variables in the format: readonly name=value
        printf("readonly functionality not fully implemented for listing\n");
        return 0;
    }

    // Process each argument
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        char *equals = strchr(arg, '=');

        if (equals) {
            // Variable assignment: readonly var=value
            *equals = '\0';
            char *name = arg;
            char *value = equals + 1;

            // Validate variable name
            if (!is_valid_identifier(name)) {
                error_message("readonly: '%s' not a valid identifier", name);
                *equals = '='; // Restore the string
                return 1;
            }

            // Set the variable value
            symtable_set_global(name, value);

            // Mark as readonly (note: this is a simplified implementation)
            // In a full implementation, we would need to track readonly status
            // and prevent future modifications

            *equals = '='; // Restore the string
        } else {
            // No assignment: readonly var (make existing variable readonly)
            if (!is_valid_identifier(arg)) {
                error_message("readonly: '%s' not a valid identifier", arg);
                return 1;
            }

            // Check if variable exists
            char *value = symtable_get_global(arg);
            if (!value) {
                // Variable doesn't exist, create it with empty value
                symtable_set_global(arg, "");
            }

            // Mark as readonly (simplified implementation)
            // Note: Full readonly implementation would require symbol table
            // modifications to track and enforce readonly status
        }
    }

    return 0;
}

/**
 * bin_config:
 *      Manage shell configuration
 *      Usage: config [show|set|get|reload|save] [options]
 */
int bin_config(int argc, char **argv) {
    builtin_config(argc, argv);
    return 0;
}

/**
 * init_command_hash:
 *      Initialize the command hash table for remembering utility locations
 */
void init_command_hash(void) {
    if (command_hash == NULL) {
        command_hash = ht_strstr_create(HT_STR_CASECMP | HT_SEED_RANDOM);
    }
}

/**
 * free_command_hash:
 *      Free the command hash table
 */
void free_command_hash(void) {
    if (command_hash != NULL) {
        ht_strstr_destroy(command_hash);
        command_hash = NULL;
    }
}

/**
 * find_command_in_path:
 *      Search for a command in PATH and return its full path
 */
char *find_command_in_path(const char *command) {
    if (!command || command[0] == '\0') {
        return NULL;
    }

    // If command contains slash, check if it exists as-is
    if (strchr(command, '/')) {
        if (access(command, F_OK) == 0) {
            return strdup(command);
        }
        return NULL;
    }

    // Search in PATH
    const char *path_env = getenv("PATH");
    if (!path_env) {
        return NULL;
    }

    char *path_copy = strdup(path_env);
    if (!path_copy) {
        return NULL;
    }

    char *path_dir = strtok(path_copy, ":");
    char *result = NULL;

    while (path_dir) {
        // Construct full path
        size_t dir_len = strlen(path_dir);
        size_t cmd_len = strlen(command);
        char *full_path = malloc(dir_len + cmd_len + 2); // +2 for '/' and '\0'

        if (full_path) {
            snprintf(full_path, dir_len + cmd_len + 2, "%s/%s", path_dir,
                     command);

            // Check if file exists and is executable
            if (access(full_path, X_OK) == 0) {
                result = full_path;
                break;
            }

            free(full_path);
        }

        path_dir = strtok(NULL, ":");
    }

    free(path_copy);
    return result;
}

/**
 * bin_hash:
 *      POSIX hash builtin - remember or report utility locations
 *      Usage: hash [utility...]
 *             hash -r
 */
int bin_hash(int argc, char **argv) {
    init_command_hash();

    // Handle -r option (forget all remembered locations)
    if (argc == 2 && strcmp(argv[1], "-r") == 0) {
        if (command_hash) {
            ht_strstr_destroy(command_hash);
            command_hash = ht_strstr_create(HT_STR_CASECMP | HT_SEED_RANDOM);
        }
        return 0;
    }

    // Handle invalid options
    if (argc >= 2 && argv[1][0] == '-' && strcmp(argv[1], "-r") != 0) {
        error_message("hash: invalid option");
        return 2;
    }

    // No arguments - display all remembered locations
    if (argc == 1) {
        if (!command_hash) {
            return 0;
        }

        ht_enum_t *enumerator = ht_strstr_enum_create(command_hash);
        if (enumerator) {
            const char *key, *value;
            while (ht_strstr_enum_next(enumerator, &key, &value)) {
                if (key && value) {
                    printf("%s\t%s\n", key, value);
                }
            }
            ht_strstr_enum_destroy(enumerator);
        }
        return 0;
    }

    // Arguments provided - find and remember utility locations
    for (int i = 1; i < argc; i++) {
        const char *utility = argv[i];

        // Skip if it's a builtin (POSIX requirement)
        if (is_builtin(utility)) {
            continue;
        }

        // Find the utility in PATH
        char *path = find_command_in_path(utility);
        if (path) {
            // Remember this location
            ht_strstr_insert(command_hash, utility, path);
            free(path);
        } else {
            error_message("hash: %s: not found", utility);
            return 1;
        }
    }

    return 0;
}

/**
 * bin_theme:
 *      Manage shell themes - list, set, and configure themes
 *      Phase 3 Target 2: Advanced Configuration Themes
 */
int bin_theme(int argc, char **argv) {
    if (!argv) {
        return 1;
    }

    // No arguments - show current theme and available themes
    if (argc == 1) {
        theme_definition_t *active = theme_get_active();
        if (active) {
            printf("Current theme: %s\n", active->name);
            printf("Description: %s\n", active->description);
            printf("Category: %s\n",
                   active->category == THEME_CATEGORY_PROFESSIONAL
                       ? "Professional"
                   : active->category == THEME_CATEGORY_CREATIVE  ? "Creative"
                   : active->category == THEME_CATEGORY_MINIMAL   ? "Minimal"
                   : active->category == THEME_CATEGORY_CLASSIC   ? "Classic"
                   : active->category == THEME_CATEGORY_DEVELOPER ? "Developer"
                                                                  : "Custom");
        } else {
            printf("No theme active\n");
        }

        printf("\nAvailable themes:\n");
        char **themes = theme_list_available(-1);
        if (themes) {
            for (int i = 0; themes[i]; i++) {
                theme_definition_t *theme = theme_load(themes[i]);
                if (theme) {
                    printf("  %-12s - %s\n", theme->name, theme->description);
                }
                free(themes[i]);
            }
            free(themes);
        }
        return 0;
    }

    // Handle subcommands
    if (argc >= 2) {
        if (strcmp(argv[1], "list") == 0) {
            // List themes by category
            printf("Available themes:\n\n");

            printf("Professional:\n");
            char **prof_themes =
                theme_list_available(THEME_CATEGORY_PROFESSIONAL);
            if (prof_themes) {
                for (int i = 0; prof_themes[i]; i++) {
                    theme_definition_t *theme = theme_load(prof_themes[i]);
                    if (theme) {
                        printf("  %-12s - %s\n", theme->name,
                               theme->description);
                    }
                    free(prof_themes[i]);
                }
                free(prof_themes);
            }

            printf("\nDeveloper:\n");
            char **dev_themes = theme_list_available(THEME_CATEGORY_DEVELOPER);
            if (dev_themes) {
                for (int i = 0; dev_themes[i]; i++) {
                    theme_definition_t *theme = theme_load(dev_themes[i]);
                    if (theme) {
                        printf("  %-12s - %s\n", theme->name,
                               theme->description);
                    }
                    free(dev_themes[i]);
                }
                free(dev_themes);
            }

            printf("\nMinimal:\n");
            char **min_themes = theme_list_available(THEME_CATEGORY_MINIMAL);
            if (min_themes) {
                for (int i = 0; min_themes[i]; i++) {
                    theme_definition_t *theme = theme_load(min_themes[i]);
                    if (theme) {
                        printf("  %-12s - %s\n", theme->name,
                               theme->description);
                    }
                    free(min_themes[i]);
                }
                free(min_themes);
            }

            printf("\nCreative:\n");
            char **creative_themes =
                theme_list_available(THEME_CATEGORY_CREATIVE);
            if (creative_themes) {
                for (int i = 0; creative_themes[i]; i++) {
                    theme_definition_t *theme = theme_load(creative_themes[i]);
                    if (theme) {
                        printf("  %-12s - %s\n", theme->name,
                               theme->description);
                    }
                    free(creative_themes[i]);
                }
                free(creative_themes);
            }

            printf("\nClassic:\n");
            char **classic_themes =
                theme_list_available(THEME_CATEGORY_CLASSIC);
            if (classic_themes) {
                for (int i = 0; classic_themes[i]; i++) {
                    theme_definition_t *theme = theme_load(classic_themes[i]);
                    if (theme) {
                        printf("  %-12s - %s\n", theme->name,
                               theme->description);
                    }
                    free(classic_themes[i]);
                }
                free(classic_themes);
            }

            return 0;
        }

        if (strcmp(argv[1], "set") == 0) {
            if (argc < 3) {
                error_message("theme set: theme name required");
                return 1;
            }

            const char *theme_name = argv[2];
            if (theme_set_active(theme_name)) {
                printf("Theme set to: %s\n", theme_name);

                // Update configuration
                if (config.theme_name) {
                    free(config.theme_name);
                }
                config.theme_name = strdup(theme_name);

                // Rebuild prompt with new theme
                rebuild_prompt();

                return 0;
            } else {
                error_message("theme set: theme '%s' not found", theme_name);
                return 1;
            }
        }

        if (strcmp(argv[1], "info") == 0) {
            const char *theme_name = argc >= 3 ? argv[2] : NULL;
            theme_definition_t *theme =
                theme_name ? theme_load(theme_name) : theme_get_active();

            if (!theme) {
                error_message("theme info: %s", theme_name ? "theme not found"
                                                           : "no active theme");
                return 1;
            }

            printf("Theme: %s\n", theme->name);
            printf("Description: %s\n", theme->description);
            printf("Author: %s\n", theme->author);
            printf("Version: %s\n", theme->version);
            printf("Category: %s\n",
                   theme->category == THEME_CATEGORY_PROFESSIONAL
                       ? "Professional"
                   : theme->category == THEME_CATEGORY_CREATIVE  ? "Creative"
                   : theme->category == THEME_CATEGORY_MINIMAL   ? "Minimal"
                   : theme->category == THEME_CATEGORY_CLASSIC   ? "Classic"
                   : theme->category == THEME_CATEGORY_DEVELOPER ? "Developer"
                                                                 : "Custom");
            printf("Built-in: %s\n", theme->is_built_in ? "Yes" : "No");
            printf("256-color support: %s\n",
                   theme->supports_256_color ? "Yes" : "No");
            printf("True color support: %s\n",
                   theme->supports_true_color ? "Yes" : "No");
            printf("Requires Powerline fonts: %s\n",
                   theme->requires_powerline_fonts ? "Yes" : "No");

            printf("\nFeatures:\n");
            printf("  Right prompt: %s\n",
                   theme->templates.enable_right_prompt ? "Yes" : "No");
            printf("  Timestamp: %s\n",
                   theme->templates.enable_timestamp ? "Yes" : "No");
            printf("  Git status: %s\n",
                   theme->templates.enable_git_status ? "Yes" : "No");
            printf("  Exit code: %s\n",
                   theme->templates.enable_exit_code ? "Yes" : "No");
            printf("  Icons: %s\n", theme->effects.enable_icons ? "Yes" : "No");

            return 0;
        }

        if (strcmp(argv[1], "colors") == 0) {
            theme_definition_t *theme = theme_get_active();
            if (!theme) {
                error_message("theme colors: no active theme");
                return 1;
            }

            printf("Color scheme for theme: %s\n\n", theme->name);

            // Display color palette with examples using termcap
            if (termcap_supports_colors()) {
                termcap_print_colored(TERMCAP_BLUE, TERMCAP_COLOR_DEFAULT, "Primary:    ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_CYAN, TERMCAP_COLOR_DEFAULT, "Secondary:  ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_GREEN, TERMCAP_COLOR_DEFAULT, "Success:    ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_YELLOW, TERMCAP_COLOR_DEFAULT, "Warning:    ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_RED, TERMCAP_COLOR_DEFAULT, "Error:      ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_CYAN, TERMCAP_COLOR_DEFAULT, "Info:       ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_WHITE, TERMCAP_COLOR_DEFAULT, "Text:       ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_COLOR_BRIGHT_BLACK, TERMCAP_COLOR_DEFAULT, "Text dim:   ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_COLOR_BRIGHT_CYAN, TERMCAP_COLOR_DEFAULT, "Highlight:  ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_GREEN, TERMCAP_COLOR_DEFAULT, "Git clean:  ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_YELLOW, TERMCAP_COLOR_DEFAULT, "Git dirty:  ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_COLOR_BRIGHT_GREEN, TERMCAP_COLOR_DEFAULT, "Git staged: ");
                printf("Example text\n");
                termcap_print_colored(TERMCAP_MAGENTA, TERMCAP_COLOR_DEFAULT, "Git branch: ");
                printf("Example text\n");
            } else {
                printf("Primary:    Example text\n");
                printf("Secondary:  Example text\n");
                printf("Success:    Example text\n");
                printf("Warning:    Example text\n");
                printf("Error:      Example text\n");
                printf("Info:       Example text\n");
                printf("Text:       Example text\n");
                printf("Text dim:   Example text\n");
                printf("Highlight:  Example text\n");
                printf("Git clean:  Example text\n");
                printf("Git dirty:  Example text\n");
                printf("Git staged: Example text\n");
                printf("Git branch: Example text\n");
            }

            return 0;
        }

        if (strcmp(argv[1], "preview") == 0) {
            const char *theme_name = argc >= 3 ? argv[2] : NULL;
            theme_definition_t *theme =
                theme_name ? theme_load(theme_name) : theme_get_active();

            if (!theme) {
                error_message("theme preview: %s", theme_name
                                                       ? "theme not found"
                                                       : "no active theme");
                return 1;
            }

            printf("Preview of theme: %s\n\n", theme->name);

            // Temporarily set the theme and generate sample prompts
            theme_definition_t *original = theme_get_active();
            if (theme_name) {
                theme_set_active(theme_name);
            }

            char sample_prompt[1024];
            if (theme_generate_primary_prompt(sample_prompt,
                                              sizeof(sample_prompt))) {
                printf("Primary prompt: %s\n", sample_prompt);
            }

            char sample_ps2[256];
            if (theme_generate_secondary_prompt(sample_ps2,
                                                sizeof(sample_ps2))) {
                printf("Secondary prompt: %s\n", sample_ps2);
            }

            // Restore original theme if we changed it
            if (theme_name && original) {
                theme_set_active(original->name);
            }

            return 0;
        }

        if (strcmp(argv[1], "stats") == 0) {
            size_t total, builtin, custom;
            theme_get_statistics(&total, &builtin, &custom);

            printf("Theme system statistics:\n");
            printf("  Total themes: %zu\n", total);
            printf("  Built-in themes: %zu\n", builtin);
            printf("  Custom themes: %zu\n", custom);
            printf("  Color support: %d\n", theme_detect_color_support());
            printf("  Theme system version: %s\n", theme_get_version());

            return 0;
        }

        if (strcmp(argv[1], "symbols") == 0) {
            // Symbol compatibility controls
            if (argc == 2) {
                // Show current symbol mode
                symbol_compatibility_t mode = symbol_get_compatibility_mode();
                printf("Symbol compatibility mode: %s\n",
                       mode == SYMBOL_MODE_UNICODE ? "unicode" :
                       mode == SYMBOL_MODE_ASCII ? "ascii" : "auto");
                
                // Show terminal detection
                symbol_compatibility_t detected = symbol_detect_terminal_capability();
                printf("Terminal detected capability: %s\n",
                       detected == SYMBOL_MODE_UNICODE ? "unicode" : "ascii");
                
                // Show symbol mappings
                printf("\nSymbol mappings:\n");
                const symbol_mapping_t* mappings = symbol_get_mapping_table();
                for (int i = 0; mappings[i].unicode_symbol != NULL; i++) {
                    printf("  %s -> %s  (%s)\n", 
                           mappings[i].unicode_symbol, 
                           mappings[i].ascii_fallback,
                           mappings[i].description);
                }
                return 0;
            } else if (argc == 3) {
                // Set symbol mode
                const char* mode_str = argv[2];
                symbol_compatibility_t new_mode;
                
                if (strcmp(mode_str, "unicode") == 0) {
                    new_mode = SYMBOL_MODE_UNICODE;
                } else if (strcmp(mode_str, "ascii") == 0) {
                    new_mode = SYMBOL_MODE_ASCII;
                } else if (strcmp(mode_str, "auto") == 0) {
                    new_mode = SYMBOL_MODE_AUTO;
                } else {
                    error_message("theme symbols: invalid mode '%s' (use unicode|ascii|auto)", mode_str);
                    return 1;
                }
                
                if (symbol_set_compatibility_mode(new_mode)) {
                    printf("Symbol compatibility mode set to: %s\n", mode_str);
                    // Rebuild prompt to apply new symbol settings
                    rebuild_prompt();
                    return 0;
                } else {
                    error_message("theme symbols: failed to set mode");
                    return 1;
                }
            } else {
                error_message("theme symbols: usage: theme symbols [unicode|ascii|auto]");
                return 1;
            }
        }

        if (strcmp(argv[1], "help") == 0) {
            printf("Theme command usage:\n");
            printf("  theme              - Show current theme and list "
                   "available themes\n");
            printf("  theme list         - List all themes by category\n");
            printf("  theme set <name>   - Set active theme\n");
            printf("  theme info [name]  - Show detailed theme information\n");
            printf(
                "  theme colors       - Show color palette of active theme\n");
            printf("  theme preview [name] - Preview theme prompts\n");
            printf("  theme stats        - Show theme system statistics\n");
            printf("  theme symbols [mode] - Show/set symbol compatibility (unicode|ascii|auto)\n");
            printf("  theme help         - Show this help message\n");
            printf("\nAvailable built-in themes:\n");
            printf("  corporate  - Professional theme for business "
                   "environments\n");
            printf("  dark       - Modern dark theme with bright accents\n");
            printf("  light      - Clean light theme with good contrast\n");
            printf("  colorful   - Vibrant theme for creative workflows\n");
            printf("  minimal    - Ultra-minimal theme for focused work\n");
            printf("  classic    - Traditional shell appearance\n");

            return 0;
        }

        // Unknown subcommand
        error_message("theme: unknown subcommand '%s'", argv[1]);
        printf("Use 'theme help' for usage information\n");
        return 1;
    }

    return 0;
}

/**
 * bin_network:
 *      Manage network features and SSH host completion
 *      Phase 3 Target 3: Network Integration
 */
int bin_network(int argc, char **argv) {
    if (!argv) {
        return 1;
    }

    // No arguments - show network status and SSH host count
    if (argc == 1) {
        ssh_host_cache_t *cache = get_ssh_host_cache();
        remote_context_t *context = &g_remote_context;

        printf("Network Integration Status:\n");
        printf("SSH host completion: %s\n",
               g_network_config.ssh_completion_enabled ? "Enabled"
                                                       : "Disabled");
        printf("SSH hosts cached: %zu\n", cache ? cache->count : 0);
        printf("Remote session: %s\n",
               context->is_remote_session ? "Yes" : "No");
        printf("Cloud instance: %s\n",
               context->is_cloud_instance ? "Yes" : "No");

        if (context->remote_host[0]) {
            printf("Remote host: %s\n", context->remote_host);
        }
        if (context->cloud_provider[0]) {
            printf("Cloud provider: %s\n", context->cloud_provider);
        }

        return 0;
    }

    // Handle subcommands
    if (argc >= 2) {
        if (strcmp(argv[1], "hosts") == 0) {
            // List SSH hosts
            ssh_host_cache_t *cache = get_ssh_host_cache();
            if (!cache || cache->count == 0) {
                printf("No SSH hosts found in cache\n");
                printf("Check ~/.ssh/config and ~/.ssh/known_hosts\n");
                return 0;
            }

            printf("SSH Hosts (%zu total):\n", cache->count);
            printf("%-25s %-15s %-8s %s\n", "Host/Alias", "Hostname", "Port",
                   "Source");
            printf("%-25s %-15s %-8s %s\n", "----------", "--------", "----",
                   "------");

            for (size_t i = 0; i < cache->count && i < 50; i++) {
                ssh_host_t *host = &cache->hosts[i];
                const char *alias =
                    host->alias[0] ? host->alias : host->hostname;
                const char *hostname =
                    host->hostname[0] ? host->hostname : "N/A";
                const char *port = host->port[0] ? host->port : "22";
                const char *source =
                    host->from_config ? "config" : "known_hosts";

                printf("%-25s %-15s %-8s %s\n", alias, hostname, port, source);
            }

            if (cache->count > 50) {
                printf("... and %zu more hosts\n", cache->count - 50);
            }

            return 0;
        }

        if (strcmp(argv[1], "refresh") == 0) {
            // Refresh SSH host cache
            printf("Refreshing SSH host cache...\n");
            refresh_ssh_host_cache();
            ssh_host_cache_t *cache = get_ssh_host_cache();
            printf("Loaded %zu SSH hosts\n", cache ? cache->count : 0);
            return 0;
        }

        if (strcmp(argv[1], "test") == 0) {
            // Test network connectivity
            if (argc >= 3) {
                const char *hostname = argv[2];
                int port = (argc >= 4) ? atoi(argv[3]) : 22;

                printf("Testing connectivity to %s:%d...\n", hostname, port);
                bool result = test_host_connectivity(hostname, port, 5000);
                printf("Result: %s\n", result ? "Connected" : "Failed");
                return result ? 0 : 1;
            } else {
                printf("Usage: network test <hostname> [port]\n");
                return 1;
            }
        }

        if (strcmp(argv[1], "info") == 0) {
            // Show detailed network information
            print_remote_context_info(&g_remote_context);
            printf("\n");
            print_network_config(&g_network_config);
            printf("\n");
            print_ssh_host_cache_stats(get_ssh_host_cache());
            return 0;
        }

        if (strcmp(argv[1], "diagnostics") == 0) {
            // Run full network diagnostics
            return run_network_diagnostics();
        }

        if (strcmp(argv[1], "config") == 0) {
            // Show or modify network configuration
            if (argc >= 4 && strcmp(argv[2], "set") == 0) {
                const char *setting = argv[3];
                const char *value = (argc >= 5) ? argv[4] : "true";

                if (strcmp(setting, "ssh_completion") == 0) {
                    g_network_config.ssh_completion_enabled =
                        (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
                    printf("SSH completion: %s\n",
                           g_network_config.ssh_completion_enabled
                               ? "Enabled"
                               : "Disabled");
                } else if (strcmp(setting, "cache_timeout") == 0) {
                    int timeout = atoi(value);
                    if (timeout > 0 && timeout <= 60) {
                        g_network_config.cache_timeout_minutes = timeout;
                        printf("Cache timeout set to %d minutes\n", timeout);
                    } else {
                        printf("Invalid timeout value (1-60 minutes)\n");
                        return 1;
                    }
                } else {
                    printf("Unknown setting: %s\n", setting);
                    printf(
                        "Available settings: ssh_completion, cache_timeout\n");
                    return 1;
                }
                return 0;
            } else {
                print_network_config(&g_network_config);
                return 0;
            }
        }

        if (strcmp(argv[1], "help") == 0) {
            printf("Network command usage:\n");
            printf("  network                    - Show network status\n");
            printf("  network hosts              - List SSH hosts from config "
                   "and known_hosts\n");
            printf("  network refresh            - Refresh SSH host cache\n");
            printf(
                "  network test <host> [port] - Test connectivity to host\n");
            printf("  network info               - Show detailed network "
                   "information\n");
            printf("  network diagnostics        - Run comprehensive network "
                   "diagnostics\n");
            printf(
                "  network config             - Show network configuration\n");
            printf("  network config set <setting> <value> - Modify network "
                   "settings\n");
            printf("  network help               - Show this help message\n");
            printf("\nFeatures:\n");
            printf("  - SSH host completion for ssh, scp, rsync commands\n");
            printf("  - Remote session detection and context awareness\n");
            printf("  - Cloud provider detection (AWS, GCP, Azure)\n");
            printf("  - Network connectivity testing and VPN detection\n");
            printf("  - SSH config and known_hosts parsing\n");
            printf("\nPhase 3 Target 3: Network Integration - COMPLETE\n");
            return 0;
        }

        // Unknown subcommand
        printf("network: unknown subcommand '%s'\n", argv[1]);
        printf("Use 'network help' for usage information\n");
        return 1;
    }

    return 0;
}

// Debug builtin command - wrapper for debug system
int bin_debug(int argc __attribute__((unused)), char **argv) {
    // Initialize debug context if not already done
    if (!g_debug_context) {
        g_debug_context = debug_init();
        if (!g_debug_context) {
            fprintf(stderr, "debug: Failed to initialize debug context\n");
            return 1;
        }
    }

    debug_context_t *ctx = g_debug_context;

    // Count arguments
    int argc_real = 0;
    while (argv[argc_real]) {
        argc_real++;
    }

    // No arguments - show current debug status
    if (argc_real == 1) {
        printf("Debug Status:\n");
        printf("  Enabled: %s\n", ctx->enabled ? "yes" : "no");
        printf("  Level: %d ", ctx->level);
        switch (ctx->level) {
        case DEBUG_NONE:
            printf("(none)\n");
            break;
        case DEBUG_BASIC:
            printf("(basic)\n");
            break;
        case DEBUG_VERBOSE:
            printf("(verbose)\n");
            break;
        case DEBUG_TRACE:
            printf("(trace)\n");
            break;
        case DEBUG_PROFILE:
            printf("(profile)\n");
            break;
        default:
            printf("(unknown)\n");
            break;
        }
        printf("  Stack Depth: %d\n", ctx->stack_depth);
        printf("  Total Commands: %ld\n", ctx->total_commands);
        return 0;
    }

    // Process subcommands - comprehensive implementation
    const char *subcmd = argv[1];

    if (strcmp(subcmd, "on") == 0 || strcmp(subcmd, "enable") == 0) {
        debug_enable(ctx, true);
        if (argc_real > 2) {
            // Set level if provided
            int level = atoi(argv[2]);
            if (level >= DEBUG_NONE && level <= DEBUG_PROFILE) {
                debug_set_level(ctx, (debug_level_t)level);
            }
        }
        printf("Debug mode enabled\n");
        return 0;
    }

    if (strcmp(subcmd, "off") == 0 || strcmp(subcmd, "disable") == 0) {
        debug_enable(ctx, false);
        printf("Debug mode disabled\n");
        return 0;
    }

    if (strcmp(subcmd, "level") == 0) {
        if (argc_real < 3) {
            printf("Current debug level: %d\n", ctx->level);
            return 0;
        }

        int level = atoi(argv[2]);
        if (level < DEBUG_NONE || level > DEBUG_PROFILE) {
            fprintf(stderr, "debug: Invalid level %d (must be 0-4)\n", level);
            return 1;
        }

        debug_set_level(ctx, (debug_level_t)level);
        printf("Debug level set to %d\n", level);
        return 0;
    }

    if (strcmp(subcmd, "trace") == 0) {
        if (argc_real < 3) {
            printf("Trace execution: %s\n",
                   ctx->trace_execution ? "enabled" : "disabled");
            return 0;
        }

        if (strcmp(argv[2], "on") == 0) {
            ctx->trace_execution = true;
            printf("Trace execution enabled\n");
        } else if (strcmp(argv[2], "off") == 0) {
            ctx->trace_execution = false;
            printf("Trace execution disabled\n");
        } else {
            fprintf(stderr,
                    "debug: Invalid trace option '%s' (use 'on' or 'off')\n",
                    argv[2]);
            return 1;
        }
        return 0;
    }

    if (strcmp(subcmd, "break") == 0 || strcmp(subcmd, "breakpoint") == 0) {
        if (argc_real < 3) {
            debug_list_breakpoints(ctx);
            return 0;
        }

        if (strcmp(argv[2], "add") == 0) {
            if (argc_real < 5) {
                fprintf(stderr, "debug: Usage: debug break add <file> <line> "
                                "[condition]\n");
                return 1;
            }

            const char *file = argv[3];
            int line = atoi(argv[4]);
            const char *condition = argc_real > 5 ? argv[5] : NULL;

            int id = debug_add_breakpoint(ctx, file, line, condition);
            if (id > 0) {
                printf("Breakpoint %d added at %s:%d\n", id, file, line);
            } else {
                fprintf(stderr, "debug: Failed to add breakpoint\n");
                return 1;
            }
            return 0;
        }

        if (strcmp(argv[2], "remove") == 0 || strcmp(argv[2], "delete") == 0) {
            if (argc_real < 4) {
                fprintf(stderr, "debug: Usage: debug break remove <id>\n");
                return 1;
            }

            int id = atoi(argv[3]);
            if (debug_remove_breakpoint(ctx, id)) {
                printf("Breakpoint %d removed\n", id);
            } else {
                fprintf(stderr, "debug: Breakpoint %d not found\n", id);
                return 1;
            }
            return 0;
        }

        if (strcmp(argv[2], "list") == 0) {
            debug_list_breakpoints(ctx);
            return 0;
        }

        if (strcmp(argv[2], "clear") == 0) {
            debug_clear_breakpoints(ctx);
            printf("All breakpoints cleared\n");
            return 0;
        }

        fprintf(stderr, "debug: Unknown breakpoint command '%s'\n", argv[2]);
        return 1;
    }

    if (strcmp(subcmd, "step") == 0) {
        debug_step_into(ctx);
        return 0;
    }

    if (strcmp(subcmd, "next") == 0) {
        debug_step_over(ctx);
        return 0;
    }

    if (strcmp(subcmd, "continue") == 0) {
        debug_continue(ctx);
        return 0;
    }

    if (strcmp(subcmd, "stack") == 0) {
        debug_show_stack(ctx);
        return 0;
    }

    if (strcmp(subcmd, "vars") == 0) {
        debug_inspect_all_variables(ctx);
        return 0;
    }

    if (strcmp(subcmd, "print") == 0) {
        if (argc_real < 3) {
            fprintf(stderr, "debug: Usage: debug print <variable>\n");
            return 1;
        }

        debug_inspect_variable(ctx, argv[2]);
        return 0;
    }

    if (strcmp(subcmd, "profile") == 0) {
        if (argc_real < 3) {
            printf("Performance profiling: %s\n",
                   ctx->profile_enabled ? "enabled" : "disabled");
            return 0;
        }

        if (strcmp(argv[2], "on") == 0) {
            debug_profile_start(ctx);
            printf("Performance profiling enabled\n");
        } else if (strcmp(argv[2], "off") == 0) {
            debug_profile_stop(ctx);
            printf("Performance profiling disabled\n");
        } else if (strcmp(argv[2], "report") == 0) {
            debug_profile_report(ctx);
        } else if (strcmp(argv[2], "reset") == 0) {
            debug_profile_reset(ctx);
            printf("Profile data reset\n");
        } else {
            fprintf(stderr, "debug: Invalid profile option '%s'\n", argv[2]);
            return 1;
        }
        return 0;
    }

    if (strcmp(subcmd, "analyze") == 0) {
        if (argc_real < 3) {
            fprintf(stderr, "debug: Usage: debug analyze <script>\n");
            return 1;
        }

        debug_analyze_script(ctx, argv[2]);
        return 0;
    }

    if (strcmp(subcmd, "functions") == 0) {
        debug_list_functions(ctx);
        return 0;
    }

    if (strcmp(subcmd, "function") == 0) {
        if (argc_real < 3) {
            fprintf(stderr, "debug: Usage: debug function <name>\n");
            return 1;
        }

        debug_show_function(ctx, argv[2]);
        return 0;
    }

    if (strcmp(subcmd, "help") == 0) {
        printf("Debug command usage:\n");
        printf("  debug                    - Show debug status\n");
        printf("  debug on [level]         - Enable debug mode with optional "
               "level\n");
        printf("  debug off                - Disable debug mode\n");
        printf("  debug level [0-4]        - Set debug level\n");
        printf(
            "  debug trace on|off       - Enable/disable execution tracing\n");
        printf(
            "  debug break add <file> <line> [condition] - Add breakpoint\n");
        printf("  debug break remove <id>  - Remove breakpoint\n");
        printf("  debug break list         - List all breakpoints\n");
        printf("  debug break clear        - Clear all breakpoints\n");
        printf("  debug step               - Step into next statement\n");
        printf("  debug next               - Step over next statement\n");
        printf("  debug continue           - Continue execution\n");
        printf("  debug stack              - Show call stack\n");
        printf("  debug vars               - Show all variables\n");
        printf("  debug print <var>        - Print variable value\n");
        printf("  debug profile on|off|report|reset - Control profiling\n");
        printf("  debug analyze <script>   - Analyze script for issues\n");
        printf("  debug functions          - List all defined functions\n");
        printf("  debug function <name>    - Show function definition\n");
        printf("  debug help               - Show this help\n");
        printf("\nDebug levels:\n");
        printf("  0 - None (disabled)\n");
        printf("  1 - Basic debugging\n");
        printf("  2 - Verbose debugging\n");
        printf("  3 - Trace execution\n");
        printf("  4 - Full profiling\n");
        printf("\nPhase 3 Target 4: Shell Scripting Enhancement - ADVANCED "
               "FEATURES READY\n");
        return 0;
    }

    fprintf(stderr, "debug: Unknown command '%s'\n", subcmd);
    fprintf(stderr, "debug: Use 'debug help' for usage information\n");
    return 1;
}

/**
 * bin_display:
 *      Manage the layered display system (Week 8 - Display Integration)
 *      Usage: display status|enable|disable|config|stats|diagnostics|help
 */
int bin_display(int argc, char **argv) {
    if (argc < 2) {
        printf("Display Integration System\n");
        printf("Usage: display <command> [options]\n");
        printf("\nCommands:\n");
        printf("  status      - Show display integration status\n");
        printf("  config      - Show current configuration\n");
        printf("  stats       - Show performance statistics\n");
        printf("  diagnostics - Show detailed diagnostic information\n");
        printf("  lle         - LLE (Lusush Line Editor) control commands\n");
        printf("  help        - Show this help message\n");
        printf("\nEnvironment Variables:\n");
        printf("  LUSUSH_DISPLAY_DEBUG=1|0        - Enable/disable debug output\n");
        printf("  LUSUSH_DISPLAY_OPTIMIZATION=0-4 - Set optimization level\n");
        return 0;
    }
    
    if (strcmp(argv[1], "testsuggestion") == 0) {
        printf("Testing autosuggestion system...\n");
        
        // Force initialize autosuggestions for testing
        printf("Force initializing autosuggestions...\n");
        if (!lusush_autosuggestions_init()) {
            printf("ERROR: Failed to initialize autosuggestions\n");
            return 1;
        }
        
        // Add some test history entries
        printf("Adding test history entries...\n");
        extern void lusush_history_add(const char *line);
        lusush_history_add("echo hello world");
        lusush_history_add("echo test command");
        lusush_history_add("ls -la");
        
        // Test with some sample input
        const char *test_input = "echo";
        printf("Testing input: '%s'\n", test_input);
        
        // Call the suggestion system directly
        lusush_autosuggestion_t *suggestion = lusush_get_suggestion(test_input, strlen(test_input));
        
        if (suggestion) {
            printf("SUCCESS: Got suggestion: '%s'\n", suggestion->display_text ? suggestion->display_text : "NULL");
            
            // Clean up
            lusush_free_autosuggestion(suggestion);
        } else {
            printf("No suggestion generated\n");
        }
        
        return 0;
    }
    


    const char *subcmd = argv[1];

    if (strcmp(subcmd, "status") == 0) {
        // Show display integration status
        printf("Display Integration: ACTIVE (Layered display exclusive)\n");
        display_integration_health_t health = display_integration_get_health();
        printf("Health Status: %s\n", display_integration_health_string(health));
        
        display_integration_config_t config;
        if (display_integration_get_config(&config)) {
            printf("Configuration:\n");
            printf("  Layered display: enabled (exclusive system)\n");
            printf("  Caching: %s\n", config.enable_caching ? "enabled" : "disabled");
            printf("  Performance monitoring: %s\n", config.enable_performance_monitoring ? "enabled" : "disabled");
            printf("  Optimization level: %d\n", config.optimization_level);
            printf("  Debug mode: %s\n", config.debug_mode ? "enabled" : "disabled");
        }
        return 0;

    } else if (strcmp(subcmd, "config") == 0) {
        // Show detailed configuration
        display_integration_config_t config;
        if (!display_integration_get_config(&config)) {
            fprintf(stderr, "display: Failed to get configuration\n");
            return 1;
        }
        
        printf("=== Display Integration Configuration ===\n");
        printf("Core Features:\n");
        printf("  Layered display: enabled (exclusive system)\n");
        printf("  Caching: %s\n", config.enable_caching ? "enabled" : "disabled");
        printf("  Performance monitoring: %s\n", config.enable_performance_monitoring ? "enabled" : "disabled");
        printf("\nOptimization:\n");
        printf("  Optimization level: %d ", config.optimization_level);
        switch (config.optimization_level) {
            case 0: printf("(Disabled)\n"); break;
            case 1: printf("(Basic)\n"); break;
            case 2: printf("(Standard)\n"); break;
            case 3: printf("(Aggressive)\n"); break;
            case 4: printf("(Maximum)\n"); break;
            default: printf("(Unknown)\n"); break;
        }
        printf("  Performance threshold: %u ms\n", config.performance_threshold_ms);
        printf("  Cache hit rate threshold: %.1f%%\n", config.cache_hit_rate_threshold * 100.0);
        printf("\nBehavior:\n");
        printf("  Fallback on error: %s\n", config.fallback_on_error ? "enabled" : "disabled");
        printf("  Debug mode: %s\n", config.debug_mode ? "enabled" : "disabled");
        printf("  Max output size: %zu bytes\n", config.max_output_size);
        printf("========================================\n");
        return 0;

    } else if (strcmp(subcmd, "stats") == 0) {
        // Show performance statistics
        display_integration_stats_t stats;
        if (!display_integration_get_stats(&stats)) {
            fprintf(stderr, "display: Failed to get statistics\n");
            return 1;
        }
        
        printf("=== Display Integration Statistics ===\n");
        printf("Usage:\n");
        printf("  Total display calls: %llu\n", (unsigned long long)stats.total_display_calls);
        printf("  Layered display calls: %llu\n", (unsigned long long)stats.layered_display_calls);
        printf("  Fallback calls: %llu\n", (unsigned long long)stats.fallback_calls);
        
        if (stats.total_display_calls > 0) {
            double layered_rate = (double)stats.layered_display_calls / stats.total_display_calls * 100.0;
            double fallback_rate = (double)stats.fallback_calls / stats.total_display_calls * 100.0;
            printf("  Layered display rate: %.1f%%\n", layered_rate);
            printf("  Fallback rate: %.1f%%\n", fallback_rate);
        }
        
        if (display_integration_is_layered_active()) {
            printf("\nPerformance:\n");
            printf("  Average display time: %.2f ms\n", stats.avg_layered_display_time_ns / 1000000.0);
            printf("  Cache hit rate: %.1f%%\n", stats.cache_hit_rate * 100.0);
            printf("  Memory usage: %zu bytes\n", stats.memory_usage_bytes);
            
            printf("\nHealth:\n");
            printf("  Performance within threshold: %s\n", stats.performance_within_threshold ? "yes" : "no");
            printf("  Cache efficiency good: %s\n", stats.cache_efficiency_good ? "yes" : "no");
            printf("  Memory usage acceptable: %s\n", stats.memory_usage_acceptable ? "yes" : "no");
        }
        printf("=====================================\n");
        return 0;

    } else if (strcmp(subcmd, "diagnostics") == 0) {
        // Show detailed diagnostics
        display_integration_print_diagnostics();
        return 0;

    } else if (strcmp(subcmd, "test") == 0) {
        // Test layered display with actual content
        printf("Testing layered display system with actual content...\n");
        
        if (!display_integration_is_layered_active()) {
            printf("Error: Layered display system is not active. Run 'display enable' first.\n");
            return 1;
        }
        
        // Force a redisplay with current content
        printf("Triggering display_integration_redisplay()...\n");
        display_integration_redisplay();
        printf("Display test completed.\n");
        
        return 0;
        
    } else if (strcmp(subcmd, "performance") == 0) {
        // Performance monitoring commands
        if (argc < 3) {
            printf("Performance Monitoring Commands:\n");
            printf("  display performance init          - Initialize performance monitoring\n");
            printf("  display performance report        - Show performance report\n");
            printf("  display performance report detail - Show detailed performance report\n");
            printf("  display performance layers        - Show layer-specific cache performance\n");
            printf("  display performance memory        - Show memory pool fallback analysis\n");
            printf("  display performance baseline      - Establish performance baseline\n");
            printf("  display performance reset         - Reset performance metrics\n");
            printf("  display performance targets       - Check if targets are being met\n");
            printf("  display performance monitoring on - Enable real-time monitoring\n");
            printf("  display performance monitoring off - Disable real-time monitoring\n");
            printf("  display performance debug         - Show debug information\n");
            return 0;
        }
        
        const char *perf_cmd = argv[2];
        
        if (strcmp(perf_cmd, "init") == 0) {
            if (display_integration_init_phase_2b_monitoring()) {
                printf("Performance monitoring initialized\n");
                printf("Targets: Cache hit rate >75%%, Display timing <50ms\n");
                return 0;
            } else {
                fprintf(stderr, "display: Failed to initialize performance monitoring\n");
                return 1;
            }
            
        } else if (strcmp(perf_cmd, "report") == 0) {
            bool detailed = (argc > 3 && strcmp(argv[3], "detail") == 0);
            if (display_integration_generate_phase_2b_report(detailed)) {
                return 0;
            } else {
                fprintf(stderr, "display: Failed to generate performance report\n");
                return 1;
            }
            
        } else if (strcmp(perf_cmd, "layers") == 0) {
            display_integration_print_layer_cache_report();
            return 0;
            
        } else if (strcmp(perf_cmd, "memory") == 0) {
            lusush_pool_analyze_fallback_patterns();
            return 0;
            
        } else if (strcmp(perf_cmd, "baseline") == 0) {
            if (display_integration_establish_baseline()) {
                printf("Performance baseline established\n");
                return 0;
            } else {
                fprintf(stderr, "display: Failed to establish baseline (need more measurements)\n");
                return 1;
            }
            
        } else if (strcmp(perf_cmd, "reset") == 0) {
            if (display_integration_reset_phase_2b_metrics()) {
                printf("Performance metrics reset\n");
                return 0;
            } else {
                fprintf(stderr, "display: Failed to reset performance metrics\n");
                return 1;
            }
            
        } else if (strcmp(perf_cmd, "targets") == 0) {
            bool cache_met, timing_met;
            if (display_integration_check_phase_2b_targets(&cache_met, &timing_met)) {
                printf("Performance Target Status:\n");
                printf("  Cache Hit Rate: %s\n", cache_met ? "OK MET" : "X NOT MET");
                printf("  Display Timing: %s\n", timing_met ? "OK MET" : "X NOT MET");
                printf("  Overall: %s\n", (cache_met && timing_met) ? "OK ALL TARGETS MET" : "! NEEDS OPTIMIZATION");
                return 0;
            } else {
                fprintf(stderr, "display: Failed to check performance targets\n");
                return 1;
            }
            
        } else if (strcmp(perf_cmd, "monitoring") == 0) {
            if (argc < 4) {
                fprintf(stderr, "display: 'monitoring' requires 'on' or 'off'\n");
                return 1;
            }
            
            const char *state = argv[3];
            if (strcmp(state, "on") == 0) {
                if (display_integration_set_phase_2b_monitoring(true, 10)) {
                    printf("Real-time performance monitoring enabled (10Hz)\n");
                    return 0;
                } else {
                    fprintf(stderr, "display: Failed to enable performance monitoring\n");
                    return 1;
                }
            } else if (strcmp(state, "off") == 0) {
                if (display_integration_set_phase_2b_monitoring(false, 0)) {
                    printf("Real-time performance monitoring disabled\n");
                    return 0;
                } else {
                    fprintf(stderr, "display: Failed to disable performance monitoring\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "display: Invalid monitoring state '%s' (use 'on' or 'off')\n", state);
                return 1;
            }
            
        } else if (strcmp(perf_cmd, "debug") == 0) {
            // Debug command to troubleshoot data collection
            printf("Performance Monitoring Debug Information:\n");
            
            // Check initialization status
            phase_2b_performance_metrics_t metrics;
            if (display_integration_get_phase_2b_metrics(&metrics)) {
                printf("  Monitoring initialized: YES\n");
                printf("  Cache operations recorded: %" PRIu64 "\n", metrics.cache_operations_total);
                printf("  Display operations recorded: %" PRIu64 "\n", metrics.display_operations_measured);
                printf("  Monitoring active: %s\n", metrics.monitoring_active ? "YES" : "NO");
                printf("  Last measurement time: %ld\n", metrics.last_measurement_time);
            } else {
                printf("  Monitoring initialized: NO\n");
            }
            
            // Check integration stats
            display_integration_stats_t stats;
            if (display_integration_get_stats(&stats)) {
                printf("  Total display calls: %" PRIu64 "\n", stats.total_display_calls);
                printf("  Layered display calls: %" PRIu64 "\n", stats.layered_display_calls);
                printf("  Fallback calls: %" PRIu64 "\n", stats.fallback_calls);
                printf("  Integration active: %s\n", display_integration_is_layered_active() ? "YES" : "NO");
            }
            
            // Force a measurement test
            printf("Triggering test measurements...\n");
            display_integration_record_display_timing(5000000); // 5ms test
            display_integration_record_cache_operation(true);   // Test cache hit
            display_integration_record_cache_operation(false);  // Test cache miss
            printf("Test measurements recorded.\n");
            
            return 0;
            
        } else {
            fprintf(stderr, "display: Unknown performance command '%s'\n", perf_cmd);
            fprintf(stderr, "display: Use 'display performance' for available commands\n");
            return 1;
        }
        
    } else if (strcmp(subcmd, "lle") == 0) {
        // LLE (Lusush Line Editor) control commands
        if (argc < 3) {
            printf("LLE (Lusush Line Editor) Commands\n");
            printf("Usage: display lle <command> [options]\n");
            printf("\nCore Commands:\n");
            printf("  enable           - Enable LLE for this session\n");
            printf("  disable          - Disable LLE for this session\n");
            printf("  status           - Show LLE status and configuration\n");
            printf("\nFeature Control:\n");
            printf("  autosuggestions on|off  - Control Fish-style autosuggestions\n");
            printf("  syntax on|off           - Control syntax highlighting\n");
            printf("  multiline on|off        - Control multiline editing\n");
            printf("\nInformation:\n");
            printf("  keybindings      - Show active keybindings\n");
            printf("  diagnostics      - Show LLE diagnostics and health\n");
            printf("  history-import   - Import GNU Readline history into LLE\n");
            printf("\nNote: Changes apply immediately. Use 'config save' to persist.\n");
            return 0;
        }
        
        const char *lle_cmd = argv[2];
        
        if (strcmp(lle_cmd, "enable") == 0) {
            extern config_values_t config;
            config.use_lle = true;
            printf("✓ LLE enabled for this session (takes effect immediately)\n");
            printf("  Next prompt will use LLE line editor\n");
            printf("  To persist: config set editor.use_lle true && config save\n");
            return 0;
            
        } else if (strcmp(lle_cmd, "disable") == 0) {
            extern config_values_t config;
            config.use_lle = false;
            printf("✓ LLE disabled for this session (takes effect immediately)\n");
            printf("  Next prompt will use GNU Readline\n");
            printf("  To persist: config set editor.use_lle false && config save\n");
            return 0;
            
        } else if (strcmp(lle_cmd, "status") == 0) {
            extern config_values_t config;
            printf("LLE Status:\n");
            printf("  Mode: %s\n", config.use_lle ? "LLE (enabled)" : "GNU Readline (default)");
            printf("  History file: %s\n", config.use_lle ? "~/.lusush_history_lle" : "~/.lusush_history");
            
            if (config.use_lle) {
                printf("\nLLE Features:\n");
                printf("  Multi-line editing: %s\n", config.lle_enable_multiline_editing ? "enabled" : "disabled");
                printf("  History deduplication: %s\n", config.lle_enable_deduplication ? "enabled" : "disabled");
                printf("  Forensic tracking: %s\n", config.lle_enable_forensic_tracking ? "enabled" : "disabled");
            }
            return 0;
            
        } else if (strcmp(lle_cmd, "history-import") == 0) {
            extern config_values_t config;
            
            if (!config.use_lle) {
                fprintf(stderr, "Error: LLE must be enabled to import history\n");
                fprintf(stderr, "Run: display lle enable\n");
                return 1;
            }
            
            /* Get the global LLE editor */
            extern lle_editor_t *lle_get_global_editor(void);
            lle_editor_t *editor = lle_get_global_editor();
            
            if (!editor || !editor->history_system) {
                fprintf(stderr, "Error: LLE history system not initialized\n");
                fprintf(stderr, "This shouldn't happen - please report this bug\n");
                return 1;
            }
            
            /* Import from GNU Readline history using bridge */
            printf("Importing GNU Readline history into LLE...\n");
            lle_result_t result = lle_history_bridge_import_from_readline();
            
            if (result == LLE_SUCCESS) {
                /* Get entry count */
                size_t count = 0;
                lle_history_get_entry_count(editor->history_system, &count);
                
                printf("✓ Successfully imported history from ~/.lusush_history\n");
                printf("  Total entries in LLE history: %zu\n", count);
                
                /* Save to LLE history file */
                const char *home = getenv("HOME");
                if (home) {
                    char history_path[1024];
                    snprintf(history_path, sizeof(history_path), "%s/.lusush_history_lle", home);
                    lle_history_save_to_file(editor->history_system, history_path);
                    printf("  Saved to: %s\n", history_path);
                }
                
                return 0;
            } else {
                fprintf(stderr, "Error: Failed to import history (error code: %d)\n", result);
                return 1;
            }

        } else if (strcmp(lle_cmd, "keybindings") == 0) {
            /* Show active keybindings */
            extern lle_editor_t *lle_get_global_editor(void);
            lle_editor_t *editor = lle_get_global_editor();
            
            printf("LLE Active Keybindings (Emacs mode)\n");
            printf("====================================\n");
            
            if (editor && editor->keybinding_manager) {
                lle_keybinding_info_t *bindings = NULL;
                size_t count = 0;
                
                if (lle_keybinding_manager_list_bindings(editor->keybinding_manager, 
                                                         &bindings, &count) == LLE_SUCCESS) {
                    printf("\nNavigation:\n");
                    for (size_t i = 0; i < count; i++) {
                        const char *name = bindings[i].function_name ? bindings[i].function_name : "unknown";
                        if (strstr(name, "beginning") || strstr(name, "end") || 
                            strstr(name, "forward") || strstr(name, "backward") ||
                            strstr(name, "left") || strstr(name, "right") ||
                            strstr(name, "up") || strstr(name, "down")) {
                            printf("  %-12s  %s\n", bindings[i].key_sequence, name);
                        }
                    }
                    
                    printf("\nEditing:\n");
                    for (size_t i = 0; i < count; i++) {
                        const char *name = bindings[i].function_name ? bindings[i].function_name : "unknown";
                        if (strstr(name, "delete") || strstr(name, "kill") || 
                            strstr(name, "yank") || strstr(name, "undo") ||
                            strstr(name, "redo") || strstr(name, "transpose")) {
                            printf("  %-12s  %s\n", bindings[i].key_sequence, name);
                        }
                    }
                    
                    printf("\nHistory:\n");
                    for (size_t i = 0; i < count; i++) {
                        const char *name = bindings[i].function_name ? bindings[i].function_name : "unknown";
                        if (strstr(name, "history") || strstr(name, "search") ||
                            strstr(name, "previous") || strstr(name, "next")) {
                            printf("  %-12s  %s\n", bindings[i].key_sequence, name);
                        }
                    }
                    
                    printf("\nOther:\n");
                    for (size_t i = 0; i < count; i++) {
                        const char *name = bindings[i].function_name ? bindings[i].function_name : "unknown";
                        if (strstr(name, "accept") || strstr(name, "abort") ||
                            strstr(name, "clear") || strstr(name, "complete")) {
                            printf("  %-12s  %s\n", bindings[i].key_sequence, name);
                        }
                    }
                    
                    printf("\nTotal: %zu keybindings\n", count);
                } else {
                    printf("  (Unable to retrieve keybindings)\n");
                }
            } else {
                /* Show default keybindings when LLE not active */
                printf("\nNavigation:\n");
                printf("  C-a          beginning-of-line\n");
                printf("  C-e          end-of-line\n");
                printf("  C-f / RIGHT  forward-char\n");
                printf("  C-b / LEFT   backward-char\n");
                printf("  M-f          forward-word\n");
                printf("  M-b          backward-word\n");
                printf("\nEditing:\n");
                printf("  C-d / DEL    delete-char\n");
                printf("  BACKSPACE    backward-delete-char\n");
                printf("  C-k          kill-line\n");
                printf("  C-u          unix-line-discard\n");
                printf("  C-w          backward-kill-word\n");
                printf("  C-y          yank\n");
                printf("  C-_          undo\n");
                printf("  C-^          redo\n");
                printf("\nHistory:\n");
                printf("  C-p / UP     previous-history\n");
                printf("  C-n / DOWN   next-history\n");
                printf("  C-r          reverse-search-history\n");
                printf("\nOther:\n");
                printf("  RET          accept-line\n");
                printf("  C-c          abort\n");
                printf("  C-l          clear-screen\n");
                printf("  TAB          complete\n");
            }
            return 0;
            
        } else if (strcmp(lle_cmd, "autosuggestions") == 0) {
            /* Control autosuggestions */
            extern config_values_t config;
            
            if (argc < 4) {
                printf("Autosuggestions: %s\n", config.display_autosuggestions ? "enabled" : "disabled");
                printf("Usage: display lle autosuggestions on|off\n");
                return 0;
            }
            
            const char *state = argv[3];
            if (strcmp(state, "on") == 0) {
                config.display_autosuggestions = true;
                printf("✓ Autosuggestions enabled\n");
                return 0;
            } else if (strcmp(state, "off") == 0) {
                config.display_autosuggestions = false;
                printf("✓ Autosuggestions disabled\n");
                return 0;
            } else {
                fprintf(stderr, "display lle autosuggestions: Invalid option '%s' (use 'on' or 'off')\n", state);
                return 1;
            }
            
        } else if (strcmp(lle_cmd, "syntax") == 0) {
            /* Control syntax highlighting */
            extern config_values_t config;
            
            if (argc < 4) {
                printf("Syntax highlighting: %s\n", config.display_syntax_highlighting ? "enabled" : "disabled");
                printf("Usage: display lle syntax on|off\n");
                return 0;
            }
            
            const char *state = argv[3];
            if (strcmp(state, "on") == 0) {
                config.display_syntax_highlighting = true;
                printf("✓ Syntax highlighting enabled\n");
                return 0;
            } else if (strcmp(state, "off") == 0) {
                config.display_syntax_highlighting = false;
                printf("✓ Syntax highlighting disabled\n");
                return 0;
            } else {
                fprintf(stderr, "display lle syntax: Invalid option '%s' (use 'on' or 'off')\n", state);
                return 1;
            }
            
        } else if (strcmp(lle_cmd, "multiline") == 0) {
            /* Control multiline editing */
            extern config_values_t config;
            
            if (argc < 4) {
                printf("Multiline editing: %s\n", config.lle_enable_multiline_editing ? "enabled" : "disabled");
                printf("Usage: display lle multiline on|off\n");
                return 0;
            }
            
            const char *state = argv[3];
            if (strcmp(state, "on") == 0) {
                config.lle_enable_multiline_editing = true;
                printf("✓ Multiline editing enabled\n");
                return 0;
            } else if (strcmp(state, "off") == 0) {
                config.lle_enable_multiline_editing = false;
                printf("✓ Multiline editing disabled\n");
                return 0;
            } else {
                fprintf(stderr, "display lle multiline: Invalid option '%s' (use 'on' or 'off')\n", state);
                return 1;
            }
            
        } else if (strcmp(lle_cmd, "diagnostics") == 0) {
            /* Show LLE diagnostics */
            extern config_values_t config;
            extern lle_editor_t *lle_get_global_editor(void);
            lle_editor_t *editor = lle_get_global_editor();
            
            printf("LLE Diagnostics\n");
            printf("===============\n");
            
            printf("\nSystem Status:\n");
            printf("  LLE mode: %s\n", config.use_lle ? "active" : "inactive (using GNU Readline)");
            printf("  Global editor: %s\n", editor ? "initialized" : "not initialized");
            
            if (editor) {
                printf("\nSubsystems:\n");
                printf("  Buffer: %s\n", editor->buffer ? "OK" : "MISSING");
                printf("  History: %s\n", editor->history_system ? "OK" : "MISSING");
                printf("  Keybindings: %s\n", editor->keybinding_manager ? "OK" : "MISSING");
                printf("  Kill ring: %s\n", editor->kill_ring ? "OK" : "MISSING");
                printf("  Change tracker: %s\n", editor->change_tracker ? "OK" : "MISSING");
                printf("  Cursor manager: %s\n", editor->cursor_manager ? "OK" : "MISSING");
                
                if (editor->history_system) {
                    size_t count = 0;
                    lle_history_get_entry_count(editor->history_system, &count);
                    printf("\nHistory:\n");
                    printf("  Entries loaded: %zu\n", count);
                }
                
                if (editor->keybinding_manager) {
                    size_t kb_count = 0;
                    lle_keybinding_manager_get_count(editor->keybinding_manager, &kb_count);
                    printf("\nKeybindings:\n");
                    printf("  Bindings registered: %zu\n", kb_count);
                    
                    uint64_t avg_us = 0, max_us = 0;
                    if (lle_keybinding_manager_get_stats(editor->keybinding_manager, &avg_us, &max_us) == LLE_SUCCESS) {
                        printf("  Avg lookup time: %lu µs\n", (unsigned long)avg_us);
                        printf("  Max lookup time: %lu µs\n", (unsigned long)max_us);
                        printf("  Performance: %s\n", max_us < 50 ? "OK (<50µs)" : "SLOW (>50µs)");
                    }
                }
            }
            
            printf("\nFeature Configuration:\n");
            printf("  Autosuggestions: %s\n", config.display_autosuggestions ? "enabled" : "disabled");
            printf("  Syntax highlighting: %s\n", config.display_syntax_highlighting ? "enabled" : "disabled");
            printf("  Multiline editing: %s\n", config.lle_enable_multiline_editing ? "enabled" : "disabled");
            printf("  History deduplication: %s\n", config.lle_enable_deduplication ? "enabled" : "disabled");
            printf("  Interactive search: %s\n", config.lle_enable_interactive_search ? "enabled" : "disabled");
            
            printf("\nHealth: ");
            if (!config.use_lle) {
                printf("N/A (LLE not active)\n");
            } else if (!editor) {
                printf("ERROR (editor not initialized)\n");
            } else if (!editor->buffer || !editor->history_system || !editor->keybinding_manager) {
                printf("DEGRADED (missing subsystems)\n");
            } else {
                printf("OK\n");
            }
            
            return 0;
            
        } else {
            fprintf(stderr, "display lle: Unknown command '%s'\n", lle_cmd);
            fprintf(stderr, "display lle: Use 'display lle' for usage information\n");
            return 1;
        }
        
    } else if (strcmp(subcmd, "help") == 0) {
        // Show help
        printf("Display Integration System\n");
        printf("\nThe display integration system provides coordinated display\n");
        printf("management using the revolutionary layered display architecture.\n");
        printf("It enables universal prompt compatibility, real-time syntax\n");
        printf("highlighting, and intelligent layer combination with enterprise-\n");
        printf("grade performance optimization.\n");
        printf("\nCommands:\n");
        printf("  display status           - Show system status and health\n");

        printf("  display config           - Show detailed configuration\n");
        printf("  display stats            - Show usage statistics\n");
        printf("  display diagnostics      - Show system diagnostics\n");
        printf("  display performance      - Performance monitoring commands\n");
        printf("  display test             - Test layered display with actual content\n");
        printf("  display help             - Show this help message\n");
        printf("\nConfiguration:\n");
        printf("  Environment variables can be used to control behavior:\n");
        printf("  - LUSUSH_LAYERED_DISPLAY=1|0     Enable/disable at startup\n");
        printf("  - LUSUSH_DISPLAY_DEBUG=1|0       Enable debug output\n");
        printf("  - LUSUSH_DISPLAY_OPTIMIZATION=0-4 Set optimization level\n");
        printf("\nOptimization Levels:\n");
        printf("  0 - Disabled (no optimization)\n");
        printf("  1 - Basic (basic caching only)\n");
        printf("  2 - Standard (default optimization)\n");
        printf("  3 - Aggressive (aggressive optimization)\n");
        printf("  4 - Maximum (maximum performance mode)\n");
        printf("\nFor more information, see the Week 8 implementation documentation.\n");
        return 0;

    } else {
        fprintf(stderr, "display: Unknown command '%s'\n", subcmd);
        fprintf(stderr, "display: Use 'display help' for usage information\n");
        return 1;
    }
}
