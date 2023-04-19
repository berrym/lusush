#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "alias.h"
#include "builtins.h"
#include "errors.h"
#include "exec.h"
#include "lusush.h"
#include "node.h"
#include "parser.h"
#include "scanner.h"
#include "strings.h"

char *search_path(char *fn)
{
    char *PATH = getenv("PATH"), *p = PATH, *p2 = NULL;

    while (p && *p) {
        p2 = p;
        while (*p2 && *p2 != ':')
            p2++;

        size_t plen = p2 - p;

        if (!plen)
            plen = 1;

        size_t alen = strlen(fn);
        char path[plen + 1 + alen + 1];

        strncpy(path, p, p2 - p);
        path[p2 - p] = '\0';

        if (p2[-1] != '/')
            strcat(path, "/");

        strcat(path, fn);

        struct stat st;

        if (stat(path, &st) == 0) {
            if (!S_ISREG(st.st_mode)) {
                errno = ENOENT;
                p = p2;
                if (*p2 == ':')
                    p++;
                continue;
            }

            if (!(p = alloc_str(strlen(path) + 1, false)))
                return NULL;

            strcpy(p, path);
            return p;
        } else {
            p = p2;
            if (*p2 == ':')
                p++;
        }
    }

    errno = ENOENT;
    return NULL;
}

int do_exec_cmd(int argc, char **argv)
{
    if (strchr(*argv, '/')) {
        execv(*argv, argv);
    } else {
        char *path = search_path(*argv);

        if (!path)
            return 0;

        execv(path, argv);
        error_return("do_exec_cmd");
    }

    return 0;
}

inline void free_argv(int argc, char **argv)
{
    if (!argc)
        return;

    while (argc--)
        free(argv[argc]);
}

int do_command(node_s *n)
{
    size_t argc = 0, max_args = 255;
    char *argv[max_args + 1], *str = NULL;

    if (!n)
        return 0;

    node_s *child = n->first_child;

    if (!child)
        return 0;

    while (child) {
        str = child->val.str;
        argv[argc] = alloc_str(strlen(str) + 1, false);

        if (!argv[argc]) {
            free_argv(argc, argv);
            return 0;
        }

        strcpy(argv[argc], str);
        null_terminate_str(argv[argc]);

        if (++argc >= max_args)
            break;

        child = child->next_sibling;
    }

    argv[argc] = NULL;

    // Execute an aliased command
    char *alias = lookup_alias(*argv);
    if (alias) {
        // Create a source structure from input
        source_s src;
        src.buf = alias;
        src.bufsize = strlen(alias);
        src.pos = INIT_SRC_POS;

        // Parse then execute a command
        parse_and_execute(&src);
        free_argv(argc, argv);
        return 1;
    }

    // Execute a builtin command
    for (size_t i = 0; i < builtins_count; i++) {
        if (strcmp(*argv, builtins[i].name) == 0) {
            builtins[i].func(argc, argv);
            free_argv(argc, argv);
            return 1;
        }
    }

    pid_t child_pid = 0;

    if ((child_pid = fork()) == 0) {
        do_exec_cmd(argc, argv);
        error_return("do_command");

        switch (errno) {
        case ENOEXEC:
            exit(126);
            break;
        case ENOENT:
            exit(127);
            break;
        default:
            exit(EXIT_FAILURE);
            break;
        }
    } else if (child_pid < 0) {
        error_return("do_command");
        return 0;
    }

    int status = 0;

    waitpid(child_pid, &status, 0);

    free_argv(argc, argv);

    return 1;
}
