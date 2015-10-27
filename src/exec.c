/**
 * exec.c - execute commands
 *
 * Copyright (c) 2015 Michael Berry <trismegustis@gmail.com>
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

#include "exec.h"
#include "history.h"
#include "builtins.h"
#include "alias.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define WAITFLAGS(command) (command->background ? WNOHANG : 0)

static int pfd[2] = { -1 }; // pipe file descriptors for parent/child ipc

/**
 * tell_wait:
 *      Set up pipes for ipc between parent and child processes.
 */
static void tell_wait(void)
{
    // Create a new parent/child ipc pipe
    if (pipe(pfd) < 0) {
        error_return("lusush: exec.c: tell_wait: pipe");
        pfd[0] = pfd[1] = -1;
    }
}

/**
 * tell_parent:
 *      Write a character 'c' to parent process signaling to quit blocking.
 */
static void tell_parent(pid_t pid)
{
    if (write(pfd[1], "c", 1) != 1)
        error_return("lusush: exec.c: tell_parent: write");
}

/**
 * wait_child:
 *      Block until a character 'c' is read from child process.
 */
static void wait_child(void)
{
    char c;

    if (read(pfd[0], &c, 1) != 1)
        error_return("lusush: exec.c: wait_child: read");

    if (c != 'c')
        error_message("lusush: exec.c: wait_child: incorrect data");
}

/**
 * set_pipes:
 *      Set up pipe file descriptors for command pipe chains.
 */
static void set_pipes(struct command * cmd)
{
    // There was a previous command in pipe chain
    if (cmd->prev && cmd->prev->pipe) {
        vputs("*** Reading from parent pipe\n");
        if (dup2(cmd->prev->pfd[0], fileno(stdin)) < 0)
            error_return("lusush: exec.c: set_pipes: dup2");

        if (close(cmd->prev->pfd[0]) < 0 || close(cmd->prev->pfd[1]) < 0)
            error_return("lusush: exec.c: set_pipes: close");
    }

    // There is a future command in pipe chain
    if (cmd->next && cmd->next->pipe) {
        vputs("*** Writing to child pipe\n");
        if (close(cmd->pfd[0]) < 0)
            error_return("lusush: exec.c: set_pipes: close");

        if (dup2(cmd->pfd[1], fileno(stdout)) < 0)
            error_return("lusush: exec.c: set_pipes: dup2");

        if (close(cmd->pfd[1]) < 0)
            error_return("lusush: exec.c: set_pipes: close");
    }
}

/**
 * close_old_pipes:
 *      Close old unused pipes.
 */
static void close_old_cmd_pipes(struct command *cmd)
{
    // Close pipes from previous command in pipe chain
    if (cmd->prev && cmd->prev->pipe) {
        vputs("*** Closing old/unused pipe ends\n");
        if (close(cmd->prev->pfd[0]) < 0 || close(cmd->prev->pfd[1]) < 0)
            error_return("lusush: exec.c: close_old_pipes: close");
        cmd->prev->pfd[0] = cmd->prev->pfd[1] = -1;
    }
}

/**
 * set_redirections:
 *     Set up input/output redirections.
 */
static void set_redirections(struct command *cmd)
{
    // Set up input redirection
    if (cmd->iredir)
        if (freopen(cmd->ifname, "r", stdin) == NULL)
            error_return("lusush: exec.c: set_redirections: freopen");

    // Execute in the backgroud
    if (cmd->background)
        if (freopen("/dev/null", "w", stdout) == NULL ||
            freopen("/dev/null", "w", stderr) == NULL)
            error_return("lusush: exec.c: set_redirections: freopen");

    // Set up output redirection
    if (cmd->oredir)
        if (freopen(cmd->ofname,
                    cmd->oredir_append ? "a" : "w", stdout) == NULL)
            error_return("lusush: exec.c: set_redirections: freopen");
}

/**
 * exec_external_cmd:
 *      Execute an external command after setting up pipes or redirections.
 */
static int exec_external_cmd(struct command *cmd)
{
    pid_t pid;                  // pid return by execvp

    // Setup pipes for parent/child ipc
    tell_wait();

    // Create a pipe if command is in a pipe chain
    if (cmd->pipe) {
        if (cmd->next && cmd->next->pipe) {
            vputs("*** Creating pipe\n");
            if (pipe(cmd->pfd) < 0)
                error_return("lusush: exec.c: exec_external_cmd: pipe");
        }
    }

    // Spawn a new process by calling fork
    pid = fork();

    switch (pid) {
    case -1:                    // fork error
        error_return("lusush: exec.c: exec_external_command: fork");
    case 0:                     // child process
        // Configure pipe plumbing
        if (cmd->pipe)
            set_pipes(cmd);

        // Configure redirections
        if (cmd->iredir || cmd->oredir || cmd->background)
            set_redirections(cmd);

        // Signal parent to quit blocking
        tell_parent(getppid());

        // Call execvp
        vputs("calling execvp\n");
        execvp(cmd->argv[0], cmd->argv);
        error_return("lusush: %s", cmd->argv[0]);
        exit(127);
        break;
    default:                    // parent process
        // Block until signaled by child
        wait_child();

        // Close old pipe ends
        if (cmd->pipe && !cmd->pipe_head)
            close_old_cmd_pipes(cmd);

        if (pfd[0] >= 0) {
            if (close(pfd[0]) < 0)
                error_return("lusush: exec.c: tell_wait: close");
            pfd[0] = -1;
        }

        if (pfd[1] >= 0) {
            if (close(pfd[1]) < 0)
                error_return("lusush: exec.c: tell_wait: close");
            pfd[1] = -1;
        }
        break;
    }

    return pid;                 // return the pid to wait for
}

/**
 * exec_cmd:
 *      Execute built in and external commands.
 */
void exec_cmd(struct command *cmdp)
{
    int err = 0, status = 0;    // error status, waitpid status
    int pid = 0;                // process id returned by fork
    struct command *cmd = NULL; // command pointer to iterate over list
    struct builtin *bin = NULL; // built in command

    // Execute each command in the list
    for (cmd = cmdp; *cmd->argv[0]; cmd = cmd->next) {
        if ((bin = find_builtin(cmd->argv[0]))) { // execute a builtin
            if (cmd->pipe) {
                error_message("lusush: cannot pipe with builtins\n");
                free(bin);
                bin = NULL;
                break;
            }

            // Special case for the exit command
            if (strcmp(bin->name, "exit") ==0) {
                free(bin);
                bin = NULL;
                printf("Goodbye!\n");
                exit(EXIT_SUCCESS);
            }

            // Call the builtin function
            err = bin->func(cmd);
            vputs("BUILTIN (%s) returned a status of %d\n", bin->name, err);

            // Free memory used by bin
            free(bin);
            bin = NULL;
        }
        else {                  // execute an external command
            if (!(pid = exec_external_cmd(cmd)))
                continue;

            // If executing the command in the background, call waitpid with
            // the WNOHANG option, otherwise pass 0 to block
            if ((pid = waitpid(pid, &status, WAITFLAGS(cmd))) == -1)
                error_return("lusush: exec.c: exec_cmd: waitpid");
        }
    }
}
