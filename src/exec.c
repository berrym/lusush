#define _POSIX_C_SOURCE 200809L

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
    char *PATH = getenv("PATH");
    char *p = PATH;
    char *p2;

    while (p && *p) {
        p2 = p;
        while (*p2 && *p2 != ':')
            p2++;

        int plen = p2 - p;

        if (!plen)
            plen = 1;

        int alen = strnlen(fn, MAXLINE);
        char path[plen + 1 + alen + 1];

        strncpy(path, p, p2 - p);
        path[p2 - p] = '\0';

        if (p2[-1] != '/')
            strncat(path, "/", 2);

        strncat(path, fn, MAXLINE);

        struct stat st;

        if (stat(path, &st) == 0) {
            if (!S_ISREG(st.st_mode)) {
                errno = ENOENT;
                p = p2;
                if (*p2 == ':')
                    p++;
                continue;
            }

            if ((p = alloc_string(strlen(path) + 1, false)) == NULL)
                return NULL;

            strncpy(p, path, strnlen(path, MAXLINE));
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
        free(path);
    }
    return 0;
}

static inline void free_argv(int argc, char **argv)
{
    if (!argc)
        return;

    while (argc--)
        free(argv[argc]);
}

int do_command(struct node *node)
{
    if (!node)
        return 0;

    struct node *child = node->first_child;

    if (!child)
        return 0;

    int argc = 0;
    long max_args = 255;
    char *argv[max_args + 1]; /* keep 1 for the terminating NULL arg */
    char *str;

    while (child) {
        str = child->val.str;
        argv[argc] = alloc_string(strnlen(str, MAXLINE) + 1, false);

        if(!argv[argc]) {
            free_argv(argc, argv);
            return 0;
        }

        strncpy(argv[argc], str, strnlen(str, MAXLINE));

        if (++argc >= max_args)
            break;

        child = child->next_sibling;
    }

    argv[argc] = NULL;

    char *alias = expand_alias(*argv);
    if (alias) {
        // Create a source structure from input
        struct source src;
        src.buf = alias;
        src.bufsize = strnlen(alias, MAX_ALIAS_LEN);
        src.pos = INIT_SRC_POS;
        // Parse then execute a command
        parse_and_execute(&src);
        free_argv(argc, argv);
        return 1;
    }

    for (size_t i = 0; i < builtins_count; i++) {
        if (strncmp(*argv, builtins[i].name, MAXLINE) == 0) {
            builtins[i].func(argc, argv);
            free_argv(argc, argv);
            return 1;
        }
    }

    pid_t child_pid = 0;

    if ((child_pid = fork()) == 0) {
        do_exec_cmd(argc, argv);
        error_message("error: failed to execute command: %s\n", strerror(errno));
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
        error_return("error: failed to fork command");
        return 0;
    }

    int status = 0;

    waitpid(child_pid, &status, 0);

    free_argv(argc, argv);

    return 1;
}
