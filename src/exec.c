#include "../include/exec.h"
#include "../include/alias.h"
#include "../include/builtins.h"
#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/node.h"
#include "../include/strings.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

char *search_path(char *fn) {
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

int do_exec_cmd(int argc, char **argv) {
    if (strchr(*argv, '/')) {
        execv(*argv, argv);
    } else {
        char *path = search_path(*argv);

        if (path == NULL)
            return 1;

        execv(path, argv);
        error_return("do_exec_cmd");
    }

    return 0;
}

inline void free_argv(int argc, char **argv) {
    if (!argc)
        return;

    while (argc--)
        free(argv[argc]);

    free(argv);
}

int do_basic_command(node_s *n) {
    size_t argc = 0, targc = 0;
    char **argv = NULL, *str = NULL;

    if (n == NULL)
        return 0;

    node_s *child = n->first_child;

    if (child == NULL)
        return 0;

    // Don't perform expansions on certain builtins like alias
    if (is_builtin(child->val.str)) {
        if (strcmp(child->val.str, "alias") == 0) {
            no_word_expand = true;
        }
    }

    // Perform alias expansion now
    char *alias = lookup_alias(child->val.str);
    if (alias) {
        set_node_val_str(child, alias);
    }

    while (child) {
        str = child->val.str;

        struct word_s *w = word_expand(str);

        if (w == NULL) {
            child = child->next_sibling;
            continue;
        }

        const struct word_s *w2 = w;
        while (w2) {
            if (check_buffer_bounds(&argc, &targc, &argv)) {
                str = calloc(strlen(w2->data) + 1, sizeof(char));
                if (str) {
                    strcpy(str, w2->data);
                    argv[argc++] = str;
                }
            }
            w2 = w2->next;
        }

        free_all_words(w);

        child = child->next_sibling;
    }

    // Make sure expansions are turned back on
    if (no_word_expand) {
        no_word_expand = false;
    }

    if (check_buffer_bounds(&argc, &targc, &argv)) {
        argv[argc] = NULL;
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
        error_return("do_basic_command");

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
    }

    if (child_pid < 0) {
        error_return("do_basic_command");
        free_argv(argc, argv);
        return 0;
    }

    int status = 0;

    waitpid(child_pid, &status, 0);
    free_argv(argc, argv);

    return 1;
}
