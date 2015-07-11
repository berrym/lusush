/**
 * exec.c - execute commands
 *
 * Copyright (c) 2009-2015 Michael Berry <trismegustis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "lusush.h"
#include "exec.h"
#include "misc.h"
#include "builtins.h"
#include "alias.h"
#include "opts.h"
#include "prompt.h"

#define WAITFLAGS(command) command->background ? WNOHANG : 0

/**
 * exec_cmd:
 *      wrapper function for exec_builtin_command and exec_external_cmd
 */
int exec_cmd(struct command *cmd, int n)
{
    int i;                      // loop variable
    int ret, status;            // return value, waitpid status
    int pids[n];            // array of pids to wait on
    struct command *psave1;     // place holders in command history

    psave1 = cmd;               // save current position in command history

    if (!*cmd->argv[0])
        return 0;

    for (i = 0; i < n; i++)
        pids[i] = 0;

    /////////////////////////////////////////////////
    //  Execute (n) number of chained commands
    /////////////////////////////////////////////////

    for (i = 0; i < n; i++) {

        /////////////////////////////////////////////////
        // Execute a builtin command
        /////////////////////////////////////////////////

        if ((ret = is_builtin_cmd(cmd->argv[0])) != -1) {
            if (cmd->pipe) {
                printf("lusush: pipes are not supported for builtins\n");
                return i;
            }
            pids[i] = 0;
            exec_builtin_cmd(ret, cmd);
        }

        /////////////////////////////////////////////////
        // Execute an external command
        /////////////////////////////////////////////////

        else {
            if ((pids[i] = exec_external_cmd(cmd)) == -1)
                return -1;
        }

        // Move to next command in chain
        if (cmd->next)
            cmd = cmd->next;
        else
            continue;
    }

    cmd = psave1;               // restore to inital offset

    /////////////////////////////////////////////////
    // Wait for processes to finish
    /////////////////////////////////////////////////

    for (i = 0; i < n; i++) {
        if (pids[i]) {
            // If executing the command in the background, call waitpid with
            // the WNOHANG option, otherwise pass 0 to block.
            if ((pids[i] = waitpid(pids[i], &status, WAITFLAGS(cmd))) == -1) {
                perror("lusush: exec.c: exec_cmd: waitpid");
                return -1;
            }
        }

        if (cmd->next != NULL)
            cmd = cmd->next;
    }

    return 0;
}

/**
 * exec_external_cmd:
 *      execute an external command setting up pipes or redirection.
 */
int exec_external_cmd(struct command *cmd)
{
    int j;
    pid_t pid;

    // Check for invalid strings at the end of vector,
    // give back to free pool, they will mess up redirections
    for (j = 0; cmd->argv[j]; j++) {
        if (!*cmd->argv[j]) {
            cmd->argc--;
            free(cmd->argv[j]);
            cmd->argv[j] = NULL;
        }
    }

    /////////////////////////////////////////////////
    // Create a pipe
    /////////////////////////////////////////////////

    if (cmd->pipe) {
        if (cmd->next && cmd->next->pipe) {
            vprint("*** Creating pipe\n");
            pipe(cmd->fd);
        }
    }

    /////////////////////////////////////////////////
    // Spawn a new process
    /////////////////////////////////////////////////

    pid = fork();
    switch (pid) {
    case -1:                    // fork error
        perror("lusush: exec.c: exec_external_command: fork");
        return -1;
    case 0:                     // child process

        /////////////////////////////////////////////////
        // Configure pipe plumbing
        /////////////////////////////////////////////////

        if (cmd->pipe) {
            // There was a previous command in pipe chain
            if (cmd->prev && cmd->prev->pipe) {
                vprint("*** Reading from parent pipe\n");
                dup2(cmd->prev->fd[0], STDIN_FILENO);
                close(cmd->prev->fd[0]);
                close(cmd->prev->fd[1]);
            }

            // There is a future command in pipe chain
            if (cmd->next && cmd->next->pipe) {
                vprint("*** Writing to child pipe\n");
                close(cmd->fd[0]);
                dup2(cmd->fd[1], STDOUT_FILENO);
                close(cmd->fd[1]);
            }
        }

        /////////////////////////////////////////////////
        // Input redirection
        /////////////////////////////////////////////////

        if (cmd->iredir)
            if (cmd->prev != NULL && cmd->prev->pipe)
                freopen(cmd->ifname, "r", stdin);

        /////////////////////////////////////////////////
        // Output redirection
        /////////////////////////////////////////////////

        if (cmd->oredir && !cmd->pipe)
            freopen(cmd->ofname, cmd->oredir_append ? "a" : "w", stdout);

        /////////////////////////////////////////////////
        // Background operation
        /////////////////////////////////////////////////

        // Close stdin and stdout if executing in the background
        // and then redirect them to /dev/null
        if (cmd->background && !cmd->oredir && !cmd->pipe) {
            if (!cmd->iredir)
                close(STDIN_FILENO);
            close(STDOUT_FILENO);
            freopen("/dev/null", "r", stdin);
            freopen("/dev/null", "w", stderr);
        }

        /////////////////////////////////////////////////
        // Call execve or one of it's wrappers
        /////////////////////////////////////////////////

        vprint("calling execvp\n");
        if (*cmd->argv != NULL)
            execvp(cmd->argv[0], cmd->argv);

        fprintf(stderr, "Could not execute: %s\n", cmd->argv[0]);
        exit(127);                  // exec shouldn't return ever
        break;
    default:                        // parent process
        // Close old pipe ends
        if (cmd->pipe && !cmd->pipe_head) {
            if (cmd->prev && cmd->prev->pipe) {
                vprint("*** Closing old/unused pipe ends\n");
                close(cmd->prev->fd[0]);
                close(cmd->prev->fd[1]);
            }
        }

        return pid;                 // return the pid to wait for
    }
}

/**
 * exec_builtin_cmd:
 *      execute builtin command number (cmdno) with the data in (cmd)
 */
void exec_builtin_cmd(int cmdno, struct command *cmd)
{
    char tmp[MAXLINE] = { '\0' };
    size_t i = 0;

    switch (cmdno) {
    case BUILTIN_CMD_EXIT:
        printf("Goodbye!\n");
        exit(EXIT_SUCCESS);
        break;
    case BUILTIN_CMD_HELP:
        if (cmd->argv[1] && *cmd->argv[1])
            help(cmd->argv[1]);
        else
            help(NULL);
        break;
    case BUILTIN_CMD_CD:
        if (cmd->argv[1])
            cd(cmd->argv[1]);
        break;
    case BUILTIN_CMD_PWD:
        pwd();
        break;
    case BUILTIN_CMD_HISTORY:
        history();
        break;
    case BUILTIN_CMD_SETENV:
        if (cmd->argc != 3)
            fprintf(stderr, "lusush: setenv: takes two arguments\n");
        else
            if (setenv(cmd->argv[1], cmd->argv[2], 1) < 0)
                perror("lusush: exec.c: exec_builtin_cmd: setenv");
        break;
    case BUILTIN_CMD_UNSETENV:
        if (cmd->argc != 2)
            fprintf(stderr, "usage: unsetenv variable\n");
        else
            if (unsetenv(cmd->argv[1]) < 0)
                perror("lusush: exec.c: exec_builtin_cmd: unsetenv");
        break;
    case BUILTIN_CMD_ALIAS:
        if (cmd->argc == 1) {
            print_alias_list();
        }
        else if (cmd->argc < 3) {
            fprintf(stderr, "usage: alias word replacement text\n");
        }
        else {
            strncpy(tmp, cmd->argv[2], MAXLINE);
            strncat(tmp, " ", 2);
            for (i=3; cmd->argv[i]; i++) {
                strncat(tmp, cmd->argv[i], MAXLINE);
                strncat(tmp, " ", 2);
            }
            set_alias(cmd->argv[1], tmp);
            strncpy(tmp, "\0", MAXLINE);
        }
        break;
    case BUILTIN_CMD_UNALIAS:
        if (cmd->argc != 2)
            fprintf(stderr, "usage: unalias alias\n");
        else
            unset_alias(cmd->argv[1]);
        break;
    case BUILTIN_CMD_SETOPT:
        if (cmd->argc != 2)
            fprintf(stderr, "usage: setopt option\n");
        else
            if (strncmp(cmd->argv[1], "VERBOSE_PRINT", MAXLINE) == 0)
                set_bool_opt(VERBOSE_PRINT, true);
            else if (strncmp(cmd->argv[1], "COLOR_PROMPT", MAXLINE) == 0)
                set_bool_opt(COLOR_PROMPT, true);
        break;
    case BUILTIN_CMD_UNSETOPT:
        if (cmd->argc != 2)
            fprintf(stderr, "usage: unsetopt option\n");
        else
            if (strncmp(cmd->argv[1], "VERBOSE_PRINT", MAXLINE) == 0)
                set_bool_opt(VERBOSE_PRINT, false);
            else if (strncmp(cmd->argv[1], "COLOR_PROMPT", MAXLINE) == 0)
                set_bool_opt(COLOR_PROMPT, false);
        break;
    case BUILTIN_CMD_SETPROMPT:
        set_prompt(cmd->argc, cmd->argv);
    deafult:
        break;
    }
}
