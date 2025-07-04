#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/symtable.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global shell options instance
shell_options_t shell_opts = {0};

// Initialize POSIX shell options with defaults
void init_posix_options(void) {
    // Set default values
    shell_opts.command_mode = false;
    shell_opts.command_string = NULL;
    shell_opts.stdin_mode = false;
    shell_opts.interactive = false;
    shell_opts.login_shell = false;
    shell_opts.exit_on_error = false;
    shell_opts.trace_execution = false;
    shell_opts.syntax_check = false;
    shell_opts.unset_error = false;
    shell_opts.verbose = false;
    shell_opts.no_globbing = false;
    shell_opts.hash_commands = true; // Default enabled for performance
    shell_opts.job_control = false;
}

// Check if a specific POSIX option is set
bool is_posix_option_set(char option) {
    switch (option) {
    case 'c':
        return shell_opts.command_mode;
    case 's':
        return shell_opts.stdin_mode;
    case 'i':
        return shell_opts.interactive;
    case 'l':
        return shell_opts.login_shell;
    case 'e':
        return shell_opts.exit_on_error;
    case 'x':
        return shell_opts.trace_execution;
    case 'n':
        return shell_opts.syntax_check;
    case 'u':
        return shell_opts.unset_error;
    case 'v':
        return shell_opts.verbose;
    case 'f':
        return shell_opts.no_globbing;
    case 'h':
        return shell_opts.hash_commands;
    case 'm':
        return shell_opts.job_control;
    default:
        return false;
    }
}

// Option checking functions
bool should_exit_on_error(void) { return shell_opts.exit_on_error; }

bool should_trace_execution(void) { return shell_opts.trace_execution; }

bool is_syntax_check_mode(void) { return shell_opts.syntax_check; }

bool should_error_unset_vars(void) { return shell_opts.unset_error; }

bool is_verbose_mode(void) { return shell_opts.verbose; }

bool is_globbing_disabled(void) { return shell_opts.no_globbing; }

// Print command trace for -x option
void print_command_trace(const char *command) {
    if (should_trace_execution()) {
        fprintf(stderr, "+ %s\n", command);
        fflush(stderr);
    }
}

// Named option mapping structure
typedef struct option_mapping {
    const char *name;
    bool *flag;
    char short_opt;
} option_mapping_t;

// Map option names to flags
static option_mapping_t option_map[] = {
    {"errexit",   &shell_opts.exit_on_error, 'e'},
    { "xtrace", &shell_opts.trace_execution, 'x'},
    { "noexec",    &shell_opts.syntax_check, 'n'},
    {"nounset",     &shell_opts.unset_error, 'u'},
    {"verbose",         &shell_opts.verbose, 'v'},
    { "noglob",     &shell_opts.no_globbing, 'f'},
    {"hashall",   &shell_opts.hash_commands, 'h'},
    {"monitor",     &shell_opts.job_control, 'm'},
    {     NULL,                        NULL,   0}
};

// Find option mapping by name
static option_mapping_t *find_option_by_name(const char *name) {
    for (int i = 0; option_map[i].name; i++) {
        if (strcmp(option_map[i].name, name) == 0) {
            return &option_map[i];
        }
    }
    return NULL;
}

// Find option mapping by short option
static option_mapping_t *find_option_by_short(char opt) {
    for (int i = 0; option_map[i].name; i++) {
        if (option_map[i].short_opt == opt) {
            return &option_map[i];
        }
    }
    return NULL;
}

// Implementation of the `set` builtin command
int builtin_set(char **args) {
    if (!args[1]) {
        // No arguments - display all variables (traditional behavior)
        // For now, just show that set is implemented
        printf("set: shell options management\n");
        printf("Current shell options:\n");
        printf("  errexit (exit on error): %s\n",
               shell_opts.exit_on_error ? "on" : "off");
        printf("  xtrace (trace execution): %s\n",
               shell_opts.trace_execution ? "on" : "off");
        printf("  noexec (syntax check): %s\n",
               shell_opts.syntax_check ? "on" : "off");
        printf("  nounset (error on unset): %s\n",
               shell_opts.unset_error ? "on" : "off");
        printf("  verbose: %s\n", shell_opts.verbose ? "on" : "off");
        printf("  noglob (disable globbing): %s\n",
               shell_opts.no_globbing ? "on" : "off");
        printf("  hashall (command hashing): %s\n",
               shell_opts.hash_commands ? "on" : "off");
        printf("  monitor (job control): %s\n",
               shell_opts.job_control ? "on" : "off");
        return 0;
    }

    for (int i = 1; args[i]; i++) {
        char *arg = args[i];

        // Handle -o and +o options
        if (strcmp(arg, "-o") == 0) {
            if (args[i + 1]) {
                // Set named option
                i++; // consume the option name
                option_mapping_t *opt = find_option_by_name(args[i]);
                if (opt) {
                    *(opt->flag) = true;
                } else {
                    error_message("set: invalid option name: %s", args[i]);
                    return 1;
                }
            } else {
                // No argument - show all options
                printf("Current shell options:\n");
                for (int j = 0; option_map[j].name; j++) {
                    printf("set %co %s\n", *(option_map[j].flag) ? '-' : '+', option_map[j].name);
                }
                return 0;
            }
        } else if (strcmp(arg, "+o") == 0) {
            if (args[i + 1]) {
                // Unset named option
                i++; // consume the option name
                option_mapping_t *opt = find_option_by_name(args[i]);
                if (opt) {
                    *(opt->flag) = false;
                } else {
                    error_message("set: invalid option name: %s", args[i]);
                    return 1;
                }
            } else {
                // No argument - show all options
                printf("Current shell options:\n");
                for (int j = 0; option_map[j].name; j++) {
                    printf("set %co %s\n", *(option_map[j].flag) ? '-' : '+', option_map[j].name);
                }
                return 0;
            }
        } else if (strcmp(arg, "--") == 0) {
            // Handle -- option: end of options, start of positional parameters
            i++; // Move past the --

            // Clear existing positional parameters $1, $2, etc.
            for (int param_num = 1; param_num <= 99; param_num++) {
                char param_name[4];
                snprintf(param_name, sizeof(param_name), "%d", param_num);
                symtable_unset_global(param_name);
            }

            // Count how many new parameters we have
            int new_argc = 0;
            int temp_i = i;
            while (args[temp_i]) {
                new_argc++;
                temp_i++;
            }

            // Free existing shell_argv if it was dynamically allocated
            if (shell_argv && shell_argv_is_dynamic) {
                for (int j = 0; j < shell_argc; j++) {
                    free(shell_argv[j]);
                }
                free(shell_argv);
            }

            // Allocate new shell_argv (include space for program name)
            shell_argc = new_argc + 1;
            shell_argv = malloc(shell_argc * sizeof(char *));
            if (shell_argv) {
                // Set program name (shell_argv[0])
                shell_argv[0] = strdup("lusush");

                // Set new positional parameters in both symbol table and global
                // arrays
                int param_num = 1;
                while (args[i] && param_num <= 99) {
                    char param_name[4];
                    snprintf(param_name, sizeof(param_name), "%d", param_num);
                    symtable_set_global(param_name, args[i]);

                    // Also update global shell_argv
                    shell_argv[param_num] = strdup(args[i]);

                    i++;
                    param_num++;
                }

                // Mark shell_argv as dynamically allocated
                shell_argv_is_dynamic = true;
            }

            // Update $# (number of positional parameters)
            char argc_str[4];
            snprintf(argc_str, sizeof(argc_str), "%d", new_argc);
            symtable_set_global("#", argc_str);

            break; // Process no more arguments after --
        } else if (arg[0] == '-' && arg[1] != '\0') {
            // Handle short options like -e, -x, etc.
            for (int j = 1; arg[j]; j++) {
                option_mapping_t *opt = find_option_by_short(arg[j]);
                if (opt) {
                    *(opt->flag) = true;
                } else {
                    error_message("set: invalid option: -%c", arg[j]);
                    return 1;
                }
            }
        } else if (arg[0] == '+' && arg[1] != '\0') {
            // Handle disabling short options like +e, +x, etc.
            for (int j = 1; arg[j]; j++) {
                option_mapping_t *opt = find_option_by_short(arg[j]);
                if (opt) {
                    *(opt->flag) = false;
                } else {
                    error_message("set: invalid option: +%c", arg[j]);
                    return 1;
                }
            }
        } else {
            // Regular positional parameters without -- prefix
            error_message("set: invalid option: %s", arg);
            return 1;
        }
    }

    return 0;
}
