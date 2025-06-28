#include "../../include/builtins.h"

#include "../../include/alias.h"
#include "../../include/config.h"
#include "../../include/errors.h"
#include "../../include/executor.h"
#include "../../include/history.h"
#include "../../include/libhashtable/ht.h"
#include "../../include/linenoise/linenoise.h"
#include "../../include/lusush.h"
#include "../../include/prompt.h"
#include "../../include/signals.h"
#include "../../include/strings.h"
#include "../../include/symtable.h"
#include "../../include/themes.h"

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
#include <ctype.h>
#include <errno.h>
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
    {     "exit",                      "exit shell",      bin_exit},
    {     "help",                    "builtin help",      bin_help},
    {       "cd",                "change directory",        bin_cd},
    {      "pwd",         "print working directory",       bin_pwd},
    {  "history",           "print command history",   bin_history},
    {    "alias",                    "set an alias",     bin_alias},
    {  "unalias",                  "unset an alias",   bin_unalias},
    {"setprompt",           "set prompt attributes", bin_setprompt},
    {    "clear",                "clear the screen",     bin_clear},
    {   "setopt",              "set a shell option",    bin_setopt},
    {     "type",            "display command type",      bin_type},
    {    "unset",          "unset a shell variable",     bin_unset},
    {     "dump",               "dump symbol table",      bin_dump},
    {     "echo",             "echo text to stdout",      bin_echo},
    {   "printf",                "formatted output",    bin_printf},
    {   "export",          "export shell variables",    bin_export},
    {   "source",                 "source a script",    bin_source},
    {        ".",                 "source a script",    bin_source},
    {     "test",                "test expressions",      bin_test},
    {        "[",                "test expressions",      bin_test},
    {     "read",                 "read user input",      bin_read},
    {     "eval",              "evaluate arguments",      bin_eval},
    {     "true",           "return success status",      bin_true},
    {    "false",           "return failure status",     bin_false},
    {      "set",               "set shell options",       bin_set},
    {     "jobs",                "list active jobs",      bin_jobs},
    {       "fg",         "bring job to foreground",        bin_fg},
    {       "bg",          "send job to background",        bin_bg},
    {    "shift",     "shift positional parameters",     bin_shift},
    {    "break",              "break out of loops",     bin_break},
    { "continue", "continue to next loop iteration",  bin_continue},
    {   "return",           "return from functions",    bin_return},
    {     "trap",             "set signal handlers",      bin_trap},
    {     "exec",      "replace shell with command",      bin_exec},
    {     "wait",        "wait for background jobs",      bin_wait},
    {    "umask",  "set/display file creation mask",     bin_umask},
    {   "ulimit",     "set/display resource limits",    bin_ulimit},
    {    "times",           "display process times",     bin_times},
    {  "getopts",           "parse command options",   bin_getopts},
    {    "local",         "declare local variables",     bin_local},
    {        ":",            "null command (no-op)",     bin_colon},
    { "readonly",      "create read-only variables",  bin_readonly},
    {   "config",      "manage shell configuration",    bin_config},
    {     "hash",      "remember utility locations",      bin_hash},
    {    "theme",             "manage shell themes",     bin_theme},
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
 * bin_cd:
 *      Change working directory.
 */
int bin_cd(int argc __attribute__((unused)),
           char **argv __attribute__((unused))) {
    static char *previous_dir = NULL;
    char *current_dir = NULL;
    char *target_dir = NULL;

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

    // Set OLDPWD environment variable for compatibility
    if (previous_dir) {
        setenv("OLDPWD", previous_dir, 1);
    }

    // Set PWD environment variable
    char *new_dir = getcwd(NULL, 0);
    if (new_dir) {
        setenv("PWD", new_dir, 1);
        free(new_dir);
    }

    return 0;
}

/**
 * bin_pwd:
 *      Print working directory.
 */
int bin_pwd(int argc __attribute__((unused)),
            char **argv __attribute__((unused))) {
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
 * bin_alias:
 *      Create aliased commands, or print alias values.
 */
int bin_alias(int argc __attribute__((unused)),
              char **argv __attribute__((unused))) {
    char *src = NULL, *name = NULL, *val = NULL, *s = NULL;

    if (argc == 1) {     // No arguments given to alias
        print_aliases(); // Print a list of set aliases
        return 0;
    }

    // Print an alias entry
    if (argc == 2 && strchr(argv[1], '=') == NULL) {
        s = lookup_alias(argv[1]); // Look up an alias given it's key
        if (s == NULL) {           // If alias not found
            error_message("error: `alias`: %s is not an alias", argv[1]);
            return 1; // Return
        }
        printf("%s='%s'\n", argv[1], s); // Print the alias entry found
        return 0;
    }

    // Create a new alias
    // Reconstruct a source string from argument vector
    src = src_str_from_argv(argc, argv, " ");

    if (src == NULL) {
        return 1;
    }

    // Parse the alias name, the part before =
    name = parse_alias_var_name(src);
    if (name == NULL) {
        error_message("error: `alias`: failed to parse alias name");
        alias_usage();
        return 1;
    }

    // Parse the alias value
    val = parse_alias_var_value(src, find_opening_quote_type(src));
    if (val == NULL) {
        error_message("error: `alias`: failed to parse alias value");
        alias_usage();
        return 1;
    }

    // Check alias is a valid name
    if (!valid_alias_name(name)) {
        error_message(
            "error: `alias`: name cannot contains illegal characters");
        return 1;
    }

    // Can't alias builtin commands or keywords
    if (is_builtin(name)) {
        error_message("error: `alias`: cannot alias shell keyword: %s", name);
        return 1;
    }

    // Set a new alias
    if (!set_alias(name, val)) {
        error_message("error: `alias`: failed to create alias");
        return 1;
    }

    // Free created strings
    free_str(src);
    free_str(name);
    free_str(val);

    return 0;
}

/**
 * bin_unalias:
 *      Remove an aliased command.
 */
int bin_unalias(int argc __attribute__((unused)),
                char **argv __attribute__((unused))) {
    switch (argc) {
    case 2:
        unset_alias(argv[1]);
        break;
    default:
        unalias_usage();
        return 1;
    }

    return 0;
}

/**
 * bin_setopt:
 *      Set a shell option.
 */
int bin_setopt(int argc __attribute__((unused)),
               char **argv __attribute__((unused))) {
    setopt(argc, argv);
    return 0;
}

/**
 * bin_setprompt:
 *      Set prompt attributes.
 */
int bin_setprompt(int argc __attribute__((unused)),
                  char **argv __attribute__((unused))) {
    set_prompt(argc, argv);
    return 0;
}

/**
 * bin_clear:
 *      Clear the screen.
 */
int bin_clear(int argc __attribute__((unused)),
              char **argv __attribute__((unused))) {
    linenoiseClearScreen();
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
    if (argc < 2) {
        error_message("usage: type name [name ...]");
        return 1;
    }

    int result = 0;
    for (int i = 1; i < argc; i++) {
        const char *name = argv[i];

        // Check if it's a builtin command
        if (is_builtin(name)) {
            printf("%s is a shell builtin\n", name);
            continue;
        }

        // Check if it's an alias
        char *alias_value = symtable_get_global("alias");
        if (alias_value) {
            // Simple alias check - in a full implementation this would parse
            // the aliases
            printf("%s is aliased\n", name);
            continue;
        }

        // Check if it's a function (stored in symbol table)
        char *func_value = symtable_get_global(name);
        if (func_value && strstr(func_value, "function")) {
            printf("%s is a function\n", name);
            continue;
        }

        // Check if it's an executable file in PATH
        char *path_env = getenv("PATH");
        if (path_env) {
            char *path_copy = strdup(path_env);
            char *dir = strtok(path_copy, ":");
            bool found = false;

            while (dir != NULL) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, name);

                if (access(full_path, X_OK) == 0) {
                    printf("%s is %s\n", name, full_path);
                    found = true;
                    break;
                }
                dir = strtok(NULL, ":");
            }

            free(path_copy);
            if (found) {
                continue;
            }
        }

        // Not found
        printf("%s: not found\n", name);
        result = 1;
    }

    return result;
}

/**
 * bin_dump:
 *      Print a local symbol table.
 */
int bin_dump(int argc __attribute__((unused)),
             char **argv __attribute__((unused))) {
    symtable_debug_dump_all_scopes();
    return 0;
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
    bool interpret_escapes = true; // Enable by default for POSIX compliance
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
            if (isdigit(format[i])) {
                while (isdigit(format[i])) {
                    width = width * 10 + (format[i] - '0');
                    i++;
                }
            }

            // Parse precision
            if (format[i] == '.') {
                i++;
                precision = 0;
                while (isdigit(format[i])) {
                    precision = precision * 10 + (format[i] - '0');
                    i++;
                }
            }

            // Handle conversion specifier
            char specifier = format[i];
            const char *arg = (arg_index < argc) ? argv[arg_index] : "";

            switch (specifier) {
            case 's': {
                // String format
                int len = strlen(arg);
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
                        putchar(arg[j]);
                    }
                } else {
                    fputs(arg, stdout);
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
                int value = (arg_index < argc) ? atoi(arg) : 0;
                printf("%*d", width, value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'c': {
                // Character format
                int value = (arg_index < argc && strlen(arg) > 0) ? arg[0] : 0;
                putchar(value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'x':
            case 'X': {
                // Hexadecimal format
                unsigned int value = (arg_index < argc)
                                         ? (unsigned int)strtoul(arg, NULL, 10)
                                         : 0;
                printf(specifier == 'x' ? "%*x" : "%*X", width, value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'o': {
                // Octal format
                unsigned int value = (arg_index < argc)
                                         ? (unsigned int)strtoul(arg, NULL, 10)
                                         : 0;
                printf("%*o", width, value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'u': {
                // Unsigned integer format
                unsigned int value = (arg_index < argc)
                                         ? (unsigned int)strtoul(arg, NULL, 10)
                                         : 0;
                printf("%*u", width, value);
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'f':
            case 'F': {
                // Float format
                double value = (arg_index < argc) ? strtod(arg, NULL) : 0.0;
                if (precision >= 0) {
                    printf("%*.*f", width, precision, value);
                } else {
                    printf("%*f", width, value);
                }
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'g':
            case 'G': {
                // General float format
                double value = (arg_index < argc) ? strtod(arg, NULL) : 0.0;
                if (precision >= 0) {
                    printf(specifier == 'g' ? "%*.*g" : "%*.*G", width,
                           precision, value);
                } else {
                    printf(specifier == 'g' ? "%*g" : "%*G", width, value);
                }
                if (arg_index < argc) {
                    arg_index++;
                }
                break;
            }
            case 'e':
            case 'E': {
                // Scientific notation
                double value = (arg_index < argc) ? strtod(arg, NULL) : 0.0;
                if (precision >= 0) {
                    printf(specifier == 'e' ? "%*.*e" : "%*.*E", width,
                           precision, value);
                } else {
                    printf(specifier == 'e' ? "%*e" : "%*E", width, value);
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

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        // Remove newline
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        // Parse and execute the line
        parse_and_execute(line);
    }

    free(line);
    fclose(file);
    return 0;
}

/**
 * bin_test:
 *      Test expressions (basic implementation).
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

    // Simple test implementations
    if (argc == 2) {
        // test STRING - true if string is non-empty
        return (strlen(argv[1]) > 0) ? 0 : 1;
    }

    if (argc == 3) {
        if (strcmp(argv[1], "-z") == 0) {
            // test -z STRING - true if string is empty
            return (strlen(argv[2]) == 0) ? 0 : 1;
        } else if (strcmp(argv[1], "-n") == 0) {
            // test -n STRING - true if string is non-empty
            return (strlen(argv[2]) > 0) ? 0 : 1;
        } else if (strcmp(argv[1], "-f") == 0) {
            // test -f FILE - true if file exists and is regular
            struct stat st;
            return (stat(argv[2], &st) == 0 && S_ISREG(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[1], "-d") == 0) {
            // test -d DIR - true if directory exists
            struct stat st;
            return (stat(argv[2], &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[1], "-e") == 0) {
            // test -e PATH - true if path exists
            struct stat st;
            return (stat(argv[2], &st) == 0) ? 0 : 1;
        } else if (strcmp(argv[1], "-c") == 0) {
            // test -c FILE - true if file is character device
            struct stat st;
            return (stat(argv[2], &st) == 0 && S_ISCHR(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[1], "-b") == 0) {
            // test -b FILE - true if file is block device
            struct stat st;
            return (stat(argv[2], &st) == 0 && S_ISBLK(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[1], "-L") == 0 || strcmp(argv[1], "-h") == 0) {
            // test -L FILE or -h FILE - true if file is symbolic link
            struct stat st;
            return (lstat(argv[2], &st) == 0 && S_ISLNK(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[1], "-p") == 0) {
            // test -p FILE - true if file is named pipe (FIFO)
            struct stat st;
            return (stat(argv[2], &st) == 0 && S_ISFIFO(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[1], "-S") == 0) {
            // test -S FILE - true if file is socket
            struct stat st;
            return (stat(argv[2], &st) == 0 && S_ISSOCK(st.st_mode)) ? 0 : 1;
        } else if (strcmp(argv[1], "-r") == 0) {
            // test -r FILE - true if file is readable
            return (access(argv[2], R_OK) == 0) ? 0 : 1;
        } else if (strcmp(argv[1], "-w") == 0) {
            // test -w FILE - true if file is writable
            return (access(argv[2], W_OK) == 0) ? 0 : 1;
        } else if (strcmp(argv[1], "-x") == 0) {
            // test -x FILE - true if file is executable
            return (access(argv[2], X_OK) == 0) ? 0 : 1;
        } else if (strcmp(argv[1], "-s") == 0) {
            // test -s FILE - true if file exists and is not empty
            struct stat st;
            return (stat(argv[2], &st) == 0 && st.st_size > 0) ? 0 : 1;
        }
    }

    if (argc == 4) {
        if (strcmp(argv[2], "=") == 0) {
            // test STRING1 = STRING2
            return (strcmp(argv[1], argv[3]) == 0) ? 0 : 1;
        } else if (strcmp(argv[2], "!=") == 0) {
            // test STRING1 != STRING2
            return (strcmp(argv[1], argv[3]) != 0) ? 0 : 1;
        } else if (strcmp(argv[2], "-eq") == 0) {
            // test NUM1 -eq NUM2
            int n1 = atoi(argv[1]);
            int n2 = atoi(argv[3]);
            return (n1 == n2) ? 0 : 1;
        } else if (strcmp(argv[2], "-ne") == 0) {
            // test NUM1 -ne NUM2
            int n1 = atoi(argv[1]);
            int n2 = atoi(argv[3]);
            return (n1 != n2) ? 0 : 1;
        } else if (strcmp(argv[2], "-lt") == 0) {
            // test NUM1 -lt NUM2
            int n1 = atoi(argv[1]);
            int n2 = atoi(argv[3]);
            return (n1 < n2) ? 0 : 1;
        } else if (strcmp(argv[2], "-le") == 0) {
            // test NUM1 -le NUM2
            int n1 = atoi(argv[1]);
            int n2 = atoi(argv[3]);
            return (n1 <= n2) ? 0 : 1;
        } else if (strcmp(argv[2], "-gt") == 0) {
            // test NUM1 -gt NUM2
            int n1 = atoi(argv[1]);
            int n2 = atoi(argv[3]);
            return (n1 > n2) ? 0 : 1;
        } else if (strcmp(argv[2], "-ge") == 0) {
            // test NUM1 -ge NUM2
            int n1 = atoi(argv[1]);
            int n2 = atoi(argv[3]);
            return (n1 >= n2) ? 0 : 1;
        }
    }

    error_message("test: unknown test condition or invalid arguments");
    return 2;
}

/**
 * bin_read:
 *      Read user input into variables.
 */
int bin_read(int argc, char **argv) {
    if (argc < 2) {
        error_message("read: usage: read variable_name");
        return 1;
    }

    // Validate variable name
    if (!is_valid_identifier(argv[1])) {
        error_message("read: '%s' not a valid identifier", argv[1]);
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);

    if (read == -1) {
        if (feof(stdin)) {
            // POSIX compliance: read should return non-zero on EOF without
            // output
            free(line);
            return 1;
        } else {
            error_message("read: input error");
            free(line);
            return 1;
        }
    }

    // Remove newline
    if (line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }

    // Set the variable using modern API
    symtable_set_global(argv[1], line);

    free(line);
    return 0;
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
} /**
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

    // For now, we'll implement return by setting a special exit code
    // The function execution mechanism will need to be updated to handle this
    // TODO: Add proper function return mechanism to executor

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
        // No more arguments
        symtable_set_global("OPTIND", "1"); // Reset for next getopts call
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
            // Not an option or single dash
            symtable_set_global("OPTIND", "1");
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
                current_optind++;
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
    } else {
        // Option doesn't take an argument
        char opt_val[2] = {opt_char, '\0'};
        symtable_set_global(varname, opt_val);
        symtable_set_global("OPTARG", "");
        option_pos++;
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
                build_prompt();

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
            printf("  Multiline: %s\n",
                   theme->templates.enable_multiline ? "Yes" : "No");
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

            // Display color palette with examples
            printf("Primary:    %sExample text%s\n", theme->colors.primary,
                   "\033[0m");
            printf("Secondary:  %sExample text%s\n", theme->colors.secondary,
                   "\033[0m");
            printf("Success:    %sExample text%s\n", theme->colors.success,
                   "\033[0m");
            printf("Warning:    %sExample text%s\n", theme->colors.warning,
                   "\033[0m");
            printf("Error:      %sExample text%s\n", theme->colors.error,
                   "\033[0m");
            printf("Info:       %sExample text%s\n", theme->colors.info,
                   "\033[0m");
            printf("Text:       %sExample text%s\n", theme->colors.text,
                   "\033[0m");
            printf("Text dim:   %sExample text%s\n", theme->colors.text_dim,
                   "\033[0m");
            printf("Highlight:  %sExample text%s\n", theme->colors.highlight,
                   "\033[0m");
            printf("Git clean:  %sExample text%s\n", theme->colors.git_clean,
                   "\033[0m");
            printf("Git dirty:  %sExample text%s\n", theme->colors.git_dirty,
                   "\033[0m");
            printf("Git staged: %sExample text%s\n", theme->colors.git_staged,
                   "\033[0m");
            printf("Git branch: %sExample text%s\n", theme->colors.git_branch,
                   "\033[0m");

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
