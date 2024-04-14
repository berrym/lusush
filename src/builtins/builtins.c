#include "../../include/builtins.h"

#include "../../include/alias.h"
#include "../../include/errors.h"
#include "../../include/history.h"
#include "../../include/lusush.h"
#include "../../include/prompt.h"
#include "../../include/scanner.h"
#include "../../include/strings.h"
#include "../../include/symtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    {     "dump",       "dump symbol table",      bin_dump},
};

const size_t builtins_count = sizeof(builtins) / sizeof(builtin);

/**
 * bin_exit:
 *      Exit the shell.
 */
int bin_exit(int argc __attribute__((unused)),
             char **argv __attribute__((unused))) {
    exit_flag = true;
    exit(EXIT_SUCCESS);
    return 0;
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

        // Create a source struct from history entry
        source_t src;
        src.buf = line;
        src.bufsize = strlen(line);
        src.pos = INIT_SRC_POS;

        // Execute the source struct
        parse_and_execute(&src);
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
 * bin_setprompt:
 *      Set prompt attributes.
 */
int bin_setprompt(int argc __attribute__((unused)),
                  char **argv __attribute__((unused))) {
    set_prompt(argc, argv);
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
