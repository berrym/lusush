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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "errors.h"
#include "exec.h"
#include "history.h"
#include "builtins.h"
#include "alias.h"

#define WAITFLAGS(command) (command->background ? WNOHANG : WUNTRACED)

static int pfd[2] = { -1 }; // pipe file descriptors for parent/child ipc

/**
 * tell_wait:
 *      Set up pipes for ipc between parent and child processes.
 */
static void tell_wait(void)
{
    // Create a new parent/child ipc pipe
    if (pipe(pfd) < 0) {
        error_return("pipe error");
        pfd[0] = pfd[1] = -1;
    }
}

/**
 * tell_parent:
 *      Write a character 'c' to parent process signaling to quit blocking.
 */
static void tell_parent(void)
{
    if (write(pfd[1], "c", 1) != 1)
        error_return("write error");
}

/**
 * wait_child:
 *      Block until a character 'c' is read from child process.
 */
static void wait_child(void)
{
    char c;

    if (read(pfd[0], &c, 1) != 1) {
        error_return("read error");
        return;
    }

    if (c != 'c')
        error_message("lusush: child process sent incorrect data");
}

/**
 * set_pipes:
 *      Set up pipe file descriptors for command pipe chains.
 */
static int set_pipes(struct command * cmd)
{
    // There was a previous command in pipe chain
    if (cmd->prev && cmd->prev->pipe) {
        if (dup2(cmd->prev->pfd[0], fileno(stdin)) < 0) {
            error_return("dup2");
            return -1;
        }

        if (close(cmd->prev->pfd[0]) < 0 || close(cmd->prev->pfd[1]) < 0) {
            error_return("close");
            return -1;
        }
    }

    // There is a future command in pipe chain
    if (cmd->next && cmd->next->pipe) {
        if (close(cmd->pfd[0]) < 0) {
            error_return("close error");
            return -1;
        }

        if (dup2(cmd->pfd[1], fileno(stdout)) < 0) {
            error_return("dup2 error");
            return -1;
        }

        if (close(cmd->pfd[1]) < 0) {
            error_return("close error");
            return -1;
        }
    }
    return 0;
}

/**
 * close_old_pipes:
 *      Close old unused pipes.
 */
static int close_old_cmd_pipes(struct command *cmd)
{
    // Close pipes from previous command in pipe chain
    if (cmd->prev && cmd->prev->pipe) {
        if (close(cmd->prev->pfd[0]) < 0 || close(cmd->prev->pfd[1]) < 0) {
            error_return("close");
            return -1;
        }
        cmd->prev->pfd[0] = cmd->prev->pfd[1] = -1;
    }
    return 0;
}

/**
 * set_redirections:
 *     Set up input/output redirections.
 */
static int set_redirections(struct command *cmd)
{
    // Set up input redirection
    if (cmd->iredir) {
        if (cmd->ifd >= 0) {
            if ((close(cmd->ifd)) < 0) {
                error_return("close error");
                return -1;
            }
            if ((openat(cmd->ifd, cmd->ifname, O_RDONLY)) < 0) {
                error_return("openat error");
                return -1;
            }
        } else {
            if (freopen(cmd->ifname, "r", stdin) <= 0) {
                error_return("freopen");
                return -1;
            }
        }
    }

    // Set up output redirection
    if (cmd->oredir) {
        if (cmd->ofd >= 0) {
            if ((close(cmd->ofd)) <= 0) {
                error_return("close error");
                return -1;
            }
            if (!cmd->oredir_append) {
                if ((openat(cmd->ofd, cmd->ofname, O_WRONLY)) <= 0) {
                    error_return("openat error");
                    return -1;
                } else {
                    if ((openat(cmd->ofd, cmd->ofname, O_RDWR)) <= 0) {
                        error_return("openat error");
                        return -1;
                    }
                }
            }
        } else {
            if (freopen(cmd->ofname,
                        cmd->oredir_append ? "a" : "w", stdout) <= 0) {
                error_return("freopen");
                return -1;
            }
        }
    }
    return 0;
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
            if (pipe(cmd->pfd) < 0) {
                error_return("pipe error");
                return -1;
            }
        }
    }

    // Spawn a new process by calling fork
    pid = fork();

    switch (pid) {
    case -1:                    // fork error
        error_return("fork");
        return -1;
    case 0:                     // child process
        // Configure pipe plumbing
        if (cmd->pipe)
            if (set_pipes(cmd) < 0)
                return -1;

        // Configure redirections
        if (cmd->iredir || cmd->oredir)
            if (set_redirections(cmd) < 0)
                return -1;

        // Signal parent to quit blocking
        tell_parent();

        // Call execvp
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
            if (close(pfd[0]) < 0) {
                error_return("close error");
                return -1;
            }
            pfd[0] = -1;
        }

        if (pfd[1] >= 0) {
            if (close(pfd[1]) < 0) {
                error_return("close error");
                return -1;
            }
            pfd[1] = -1;
        }

        if (cmd->background)
            kill(pid, SIGSTOP);

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
    char ret[128] = { '\0' };   // buffer for return status

    // Execute each command in the list
    for (cmd = cmdp; *cmd->argv[0]; cmd = cmd->next) {
        if ((bin = find_builtin(cmd->argv[0]))) { // execute a builtin
            if (cmd->pipe) {
                error_message("lusush: cannot pipe with builtins\n");
                free(bin);
                bin = NULL;
                // Save the process return value
                snprintf(ret, 128, "%d", 1);
                setenv("?", ret, 1);
                break;
            }

            // Special case for the exit command
            if (strcmp(bin->name, "exit") == 0) {
                free(bin);
                bin = NULL;
                printf("Goodbye!\n");
                exit(EXIT_SUCCESS);
            }

            // Call the builtin function
            err = bin->func(cmd);

            // Free memory used by bin
            free(bin);
            bin = NULL;

            // Save the process return value
            snprintf(ret, 128, "%d", err);
            setenv("?", ret, 1);
        } else {                // execute an external command
            if (!(pid = exec_external_cmd(cmd)))
                continue;

            do {
                if ((pid = waitpid(pid, &status, WAITFLAGS(cmd))) == -1)
                    error_return("waitpid");

                if (WIFEXITED(status)) {
                    printf("child exited with status %d\n",
                           WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("child killed by signal %d\n",
                           WTERMSIG(status));
                } else if (WIFSTOPPED(status)) {
                    printf("child stopped by signal %d\n",
                           WSTOPSIG(status));
                } else if (WIFCONTINUED(status)) {
                    printf("child continued");
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            // Save the process return value
            snprintf(ret, 128, "%d", status);
            setenv("?", ret, 1);
        }
    }
}
