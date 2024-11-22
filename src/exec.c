#include "../include/alias.h"
#include "../include/builtins.h"
#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/node.h"
#include "../include/strings.h"

#include <errno.h>
#include <stddef.h>
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
        while (*p2 && *p2 != ':') {
            p2++;
        }

        size_t plen = p2 - p;

        if (!plen) {
            plen = 1;
        }

        size_t alen = strlen(fn);
        char path[plen + 1 + alen + 1];

        strncpy(path, p, p2 - p);
        path[p2 - p] = '\0';

        if (p2[-1] != '/') {
            strcat(path, "/");
        }

        strcat(path, fn);

        struct stat st;

        if (stat(path, &st) == 0) {
            if (!S_ISREG(st.st_mode)) {
                errno = ENOENT;
                p = p2;
                if (*p2 == ':') {
                    p++;
                }
                continue;
            }

            if (!(p = alloc_str(strlen(path) + 1, false))) {
                return NULL;
            }

            strcpy(p, path);
            return p;
        } else {
            p = p2;
            if (*p2 == ':') {
                p++;
            }
        }
    }

    errno = ENOENT;
    return NULL;
}

int do_exec_cmd(int argc __attribute__((unused)), char **argv) {
    if (strchr(*argv, '/')) {
        execv(*argv, argv);
    } else {
        char *path = search_path(*argv);

        if (path == NULL) {
            return 1;
        }

        execv(path, argv);
        error_return("error: `do_exec_cmd`");
    }

    return 0;
}

int do_basic_pipe_list(node_t *n) {
    node_t **commands = NULL;
    size_t len = 32;
    size_t cnt = 0;

    // sanity check
    if (n == NULL || n->first_child == NULL) {
        return 0;
    }

    // create command buffer
    commands = calloc(len, sizeof(node_t *));
    commands[cnt] = new_node(NODE_COMMAND);
    for (node_t *p = n->first_child; p; p = p->next_sibling) {
        if (p->type == NODE_COMMAND) {
            cnt++;
            if (cnt >= len - 1) {
                len *= 2;
                node_t **tmp = realloc(commands, len * sizeof(node_t *));
                if (tmp == NULL) {
                    return 1;
                }
                commands = tmp;
            }
            commands[cnt] = new_node(NODE_COMMAND);
        } else if (p->type == NODE_PIPE) {
            ;
        } else {
            if (p->type != NODE_VAR) {
                continue;
            }
            node_t *c = new_node(NODE_VAR);
            set_node_val_str(c, p->val.str);
            add_child_node(commands[cnt], c);
        }
    }
    cnt++;

    int pipes[cnt - 1][2];

    for (size_t i = 0; i < cnt; i++) {
        size_t argc = 0, targc = 0;
        char **argv = NULL, *str = NULL;

        if (n == NULL) {
            return 0;
        }

        node_t *child = n->first_child;

        if (child == NULL) {
            return 0;
        }

        while (child) {
            str = child->val.str;
            word_t *w = word_expand(str);
            if (w == NULL) {
                child = child->next_sibling;
                continue;
            }
            const word_t *w2 = w;
            while (w2) {
                if (check_buffer_bounds(&argc, &targc, &argv)) {
                    str = alloc_str(strlen(w2->data) + 1, false);
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

        if (check_buffer_bounds(&argc, &targc, &argv)) {
            argv[argc] = NULL;
        }

        if (i < cnt - 1) {
            if (pipe(pipes[i]) == -1) {
                error_return("error: `do_pipe_list`");
            }
        }

        pid_t pid = fork();
        int status = 0;

        if (pid == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
            if (i < cnt - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            do_exec_cmd(argc, argv);
            error_return("error: `do_pipe_list`");
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
        } else if (pid < 0) {
            perror("fork");
            exit(1);
        } else {
            if (i > 0) {
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
            waitpid(pid, &status, 0);
        }
        free_argv(argc, argv);
    }

    for (size_t i = 0; i < cnt; i++) {
        free_node_tree(commands[i]);
    }
    free(commands);

    return 1;
}

int do_basic_command(node_t *n) {
    size_t argc = 0, targc = 0;
    char **argv = NULL, *str = NULL;

    if (n == NULL) {
        return 0;
    }

    node_t *child = n->first_child;

    if (child == NULL) {
        return 0;
    }

    // Set a flag if performing a variable assignment word expansion first
    bool var_assignment = false;
    if (child->val.str[0] == '$' && strchr(child->val.str, '=')) {
        var_assignment = true;
    }

    // Parsing aliases allowing for expandable commands
    // needs a modified expansion process, so set a flag
    if (strcmp(child->val.str, "alias") == 0) {
        parsing_alias = true;
    }

    while (child) {
        str = child->val.str;

        word_t *w = word_expand(str);

        if (w == NULL) {
            child = child->next_sibling;
            continue;
        }

        const word_t *w2 = w;
        while (w2) {
            if (check_buffer_bounds(&argc, &targc, &argv)) {
                str = alloc_str(strlen(w2->data) + 1, false);
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

    // Turn back on regular expansion process
    if (parsing_alias) {
        parsing_alias = false;
    }

    if (check_buffer_bounds(&argc, &targc, &argv)) {
        argv[argc] = NULL;
    }

    // If var_assignment flag is set don't execute the command
    if (var_assignment) {
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

    pid_t child_pid = fork();
    int status = 0;

    if (child_pid == -1) {
        error_return("error: `do_basic_command`");
        free_argv(argc, argv);
        return 0;
    } else if (child_pid == 0) {
        do_exec_cmd(argc, argv);
        error_return("error: `do_basic_command`");

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
    } else {
        waitpid(child_pid, &status, 0);
    }

    free_argv(argc, argv);

    return 1;
}
