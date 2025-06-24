#include "../../include/builtins.h"

#include "../../include/alias.h"
#include "../../include/errors.h"
#include "../../include/history.h"
#include "../../include/linenoise/linenoise.h"
#include "../../include/lusush.h"
#include "../../include/prompt.h"

#include "../../include/strings.h"
#include "../../include/symtable.h"
#include "../../include/executor_modern.h"
#include "../../include/signals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for job control builtins
int bin_jobs(int argc, char **argv);
int bin_fg(int argc, char **argv);
int bin_bg(int argc, char **argv);
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

// Table of builtin commands
builtin builtins[] = {
    {     "exit",              "exit shell",      bin_exit},
    {     "help",            "builtin help",      bin_help},
    {       "cd",        "change directory",        bin_cd},
    {      "pwd", "print working directory",       bin_pwd},
    {  "history",   "print command history",   bin_history},
    {    "alias",            "set an alias",     bin_alias},
    {  "unalias",          "unset an alias",   bin_unalias},
    {"setprompt",   "set prompt attributes", bin_setprompt},
    {    "clear",        "clear the screen",     bin_clear},
    {   "setopt",      "set a shell option",    bin_setopt},
    {    "unset",  "unset a shell variable",     bin_unset},
    {     "dump",       "dump symbol table",      bin_dump},
    {     "echo",      "echo text to stdout",      bin_echo},
    {   "export",     "export shell variables",   bin_export},
    {   "source",            "source a script",   bin_source},
    {        ".",            "source a script",   bin_source},
    {     "test",         "test expressions",      bin_test},
    {        "[",         "test expressions",      bin_test},
    {     "read",        "read user input",      bin_read},
    {     "eval",      "evaluate arguments",      bin_eval},
    {     "true",    "return success status",      bin_true},
    {    "false",    "return failure status",     bin_false},
    {      "set",       "set shell options",       bin_set},
    {     "jobs",         "list active jobs",      bin_jobs},
    {       "fg",    "bring job to foreground",        bin_fg},
    {       "bg",    "send job to background",         bin_bg},
    {    "shift",     "shift positional parameters",  bin_shift},
    {    "break",     "break out of loops",           bin_break},
    { "continue",     "continue to next loop iteration", bin_continue},
    {   "return",     "return from functions",        bin_return},
    {     "trap",     "set signal handlers",          bin_trap},
};

const size_t builtins_count = sizeof(builtins) / sizeof(builtin);

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
    
    // Exit immediately with the specified code
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
    if (argc == 1) {
        if (chdir(getenv("HOME")) != 0) {
            error_return("cd");
            return 1;
        }

        return 0;
    }

    if (argc != 2) {
        error_message("usage: cd pathname");
        return 1;
    }

    if (chdir(argv[1]) < 0) {
        error_return("cd");
        return 1;
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
    unset_shell_var(argv[1]);
    return 0;
}

/**
 * bin_dump:
 *      Print a local symbol table.
 */
int bin_dump(int argc __attribute__((unused)),
             char **argv __attribute__((unused))) {
    dump_local_symtable();
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
            src++;  // Skip the backslash
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
    bool interpret_escapes = true;   // Enable by default for POSIX compliance
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
 * is_valid_identifier:
 *      Check if a string is a valid shell variable identifier.
 */
static int is_valid_identifier(const char *name) {
    if (!name || !*name) return 0;
    
    // First character must be letter or underscore
    if (!isalpha(*name) && *name != '_') return 0;
    
    // Subsequent characters must be alphanumeric or underscore
    for (const char *p = name + 1; *p; p++) {
        if (!isalnum(*p) && *p != '_') return 0;
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
            
            // Set variable value using legacy API
            set_shell_varp(name, (char*)value);
            
            // Export the variable using legacy API
            export_shell_var(name);
            
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
            
            // Set variable value using legacy API
            set_shell_varp((char*)name, (char*)value);
            
            // Export the variable using legacy API
            export_shell_var((char*)name);
            
            // Skip the = and value tokens
            i += 2;
        } else {
            // Just export existing variable
            if (!is_valid_identifier(argv[i])) {
                error_message("export: '%s' not a valid identifier", argv[i]);
                return 1;
            }
            
            // Check if variable exists and get its value
            char *current_value = get_shell_varp(argv[i], NULL);
            if (current_value) {
                // Variable exists - just export it
                export_shell_var(argv[i]);
            } else {
                // Variable doesn't exist - create with empty value and export
                set_shell_varp(argv[i], "");
                export_shell_var(argv[i]);
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
        return 1;  // False if no arguments
    }
    
    // Handle closing bracket for '[' command
    if (strcmp(argv[0], "[") == 0) {
        if (argc < 2 || strcmp(argv[argc - 1], "]") != 0) {
            error_message("test: '[' command missing closing ']'");
            return 2;
        }
        argc--;  // Remove the closing bracket
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
            error_message("read: end of file reached");
        } else {
            error_message("read: input error");
        }
        free(line);
        return 1;
    }
    
    // Remove newline
    if (line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }
    
    // Set the variable using legacy API
    set_shell_varp(argv[1], line);
    
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
        total_len += strlen(argv[i]) + 1;  // +1 for space
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
    (void)argc; (void)argv;
    return 0;
}

/**
 * bin_false:
 *      Always return failure (exit status 1)
 */
int bin_false(int argc, char **argv) {
    (void)argc; (void)argv;
    return 1;
}

/**
 * bin_set:
 *      Manage shell options and behavior flags
 */
int bin_set(int argc, char **argv) {
    return builtin_set(argv);
}

// Global executor pointer for job control builtins
extern executor_modern_t *current_executor;

/**
 * bin_jobs:
 *      List active jobs
 */
int bin_jobs(int argc, char **argv) {
    if (current_executor) {
        return executor_modern_builtin_jobs(current_executor, argv);
    }
    return 1;
}

/**
 * bin_fg:
 *      Bring job to foreground
 */
int bin_fg(int argc, char **argv) {
    if (current_executor) {
        return executor_modern_builtin_fg(current_executor, argv);
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
        return executor_modern_builtin_bg(current_executor, argv);
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
    
    // Calculate available parameters to shift (excluding script name at argv[0])
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
    extern executor_modern_t *current_executor;
    
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
            fprintf(stderr, "break: %d: cannot break %d levels (only %d nested)\n", 
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
    extern executor_modern_t *current_executor;
    
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
            fprintf(stderr, "continue: %s: numeric argument required\n", argv[1]);
            return 1;
        }
        
        if (continue_level > current_executor->loop_depth) {
            fprintf(stderr, "continue: %d: cannot continue %d levels (only %d nested)\n", 
                    continue_level, continue_level, current_executor->loop_depth);
            return 1;
        }
    }
    
    // Set loop control state to continue
    current_executor->loop_control = LOOP_CONTINUE;
    
    return 0;
}/**
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
    
    // Return a special exit code that the executor can recognize as "function return"
    // We'll use a specific value that doesn't conflict with normal exit codes
    return 200 + (return_code & 0xFF); // 200-255 range for function returns
}/**
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
            fprintf(stderr, "trap: %s: invalid signal specification\n", argv[i]);
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
                fprintf(stderr, "trap: failed to set trap for signal %s\n", argv[i]);
                return 1;
            }
        }
    }
    
    return 0;
}