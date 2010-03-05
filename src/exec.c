/*
 * exec.c - execute commands
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ldefs.h"
#include "ltypes.h"
#include "exec.h"
#include "misc.h"
#include "builtins.h"
#include "env.h"
#include "alias.h"
#include "opts.h"
#include "prompt.h"

/*
 * exec_cmd:
 *      wrapper function for exec_builtin_command and exec_external_cmd
 */
int exec_cmd(CMD *cmd, int cnt)
{
    int i;                      // loop variable
    int ret, status;            // return value, waitpid status
    int pids[cnt];              // array of pids to wait on
    CMD *psave1, *psave2;       // place holders in command history

    psave1 = cmd;               // save current position in command history

    for (i = 0; i < cnt; i++) {
        pids[i] = 0;
    }

    /////////////////////////////////////////////////
    //  Execute (cnt) number of chained commands
    /////////////////////////////////////////////////

    for (i = 0; i < cnt; i++) {

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
            if ((pids[i] = exec_external_cmd(cmd)) == -1) {
                return -1;
            }
        }

        // Move to next command in chain
        if (cmd->next)
            cmd = cmd->next;
        else
            break;
    }

    psave2 = cmd;                       // save last place in command history
    cmd = psave1;                       // restore to inital offset

    /////////////////////////////////////////////////
    // Wait for processes to finish
    /////////////////////////////////////////////////

    for (i = 0; i < cnt; i++) {
        if (pids[i]) {
            // If executing the command in the background, call waitpid with
            // the WNOHANG option, otherwise pass 0 to block.
            if (cmd->background) {
                if ((pids[i]= waitpid(pids[i], &status, WNOHANG)) == -1) {
                    perror("lusush: waitpid");
                    return -1;
                }
            }
            else {
                if ((pids[i] = waitpid(pids[i], &status, 0)) == -1) {
                    perror("lusush: waitpid");
                    return -1;
                }
            } 
        }
        cmd = cmd->next;
    }

    cmd = psave2;                   // restore to final postion

    return 0;
}

/*
 * exec_external_cmd:
 *      execute an external command setting up pipes or redirection.
 */
int exec_external_cmd(CMD *cmd)
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
            print_debug("*** Creating pipe\n");
            pipe(cmd->fd);
        }
    }

    /////////////////////////////////////////////////
    // Spawn a new process
    /////////////////////////////////////////////////

    pid = fork();
    switch (pid) {
    case -1:                    // fork error
        perror("lusush: fork");
        return -1;
    case 0:                     // child process

        /////////////////////////////////////////////////
        // Configure pipe plumbing
        /////////////////////////////////////////////////

        if (cmd->pipe) {
            // There was a previous command in pipe chain
            if (cmd->prev && cmd->prev->pipe) {
                print_debug("*** Reading from parent pipe\n");
                dup2(cmd->prev->fd[0], STDIN_FILENO);
                close(cmd->prev->fd[0]);
                close(cmd->prev->fd[1]);
            }

            // There is a future command in pipe chain
            if (cmd->next && cmd->next->pipe) {
                print_debug("*** Writing to child pipe\n");
                close(cmd->fd[0]);
                dup2(cmd->fd[1], STDOUT_FILENO);
                close(cmd->fd[1]);
            }
        }

        /////////////////////////////////////////////////
        // Input redirection
        /////////////////////////////////////////////////

        if (cmd->in_redirect && !cmd->prev->pipe) {
            close(STDIN_FILENO);
            freopen(cmd->in_filename, "r", stdin);
        }

        /////////////////////////////////////////////////
        // Output redirection
        /////////////////////////////////////////////////

        if (cmd->out_redirect && !cmd->pipe) {
            close(STDOUT_FILENO);
            freopen(cmd->out_filename,
                    cmd->oredir_append ? "a" : "w", stdout);
        }

        /////////////////////////////////////////////////
        // Background operation
        /////////////////////////////////////////////////

        // Close stdin and stdout if executing in the background
        // and then redirect them to /dev/null
        if (cmd->background && !cmd->out_redirect && !cmd->pipe) {
            if (!cmd->in_redirect)
                close(STDIN_FILENO);
            close(STDOUT_FILENO);
            freopen("/dev/null", "r", stdin);
            freopen("/dev/null", "w", stderr);
        }

        /////////////////////////////////////////////////
        // Call execve or one of it's wrappers
        /////////////////////////////////////////////////

        print_debug("calling execvp\n");
        execvp(cmd->argv[0], cmd->argv);

        fprintf(stderr, "Could not execute: %s\n", cmd->argv[0]);
        exit(127);                  // exec shouldn't return ever
        break;
    default:                        // parent process
        // Close old pipe ends
        if (cmd->pipe && !cmd->pchain_master) {
            if (cmd->prev && cmd->prev->pipe) {
                print_debug("*** Closing old/unused pipe ends\n");
                close(cmd->prev->fd[0]);
                close(cmd->prev->fd[1]);
            }
        }
        return pid;                 // return the pid of to wait for
    }
}

/*
 * exec_builtin_cmd:
 *      execute builtin command number (cmdno) with the data in (cmd)
 */
void exec_builtin_cmd(int cmdno, CMD *cmd)
{
    char tmp[MAXLINE] = { '\0' };
    size_t i = 0;

    switch (cmdno) {
    case BUILTIN_CMD_EXIT:
        printf("Goodbye!\n");
        exit(EXIT_SUCCESS);
        break;
    case BUILTIN_CMD_HELP:
        if (cmd->argv[1] && *cmd->argv[1]) {
            help(cmd->argv[1]);
        }
        else {
            help(NULL);
        }
        break;
    case BUILTIN_CMD_CD:
        if (cmd->argv[1]) {
            cd(cmd->argv[1]);
        }
        break;
    case BUILTIN_CMD_PWD:
        pwd();
        break;
    case BUILTIN_CMD_HISTORY:
        history();
        break;
    case BUILTIN_CMD_SETENV:
        if (cmd->argc != 3) {
            fprintf(stderr, "lusush: setenv: takes two arguments\n");
        }
        else {
            if (setenv(cmd->argv[1], cmd->argv[2], 1) < 0) {
                perror("lusush: setenv");
            }
        }
        break;
    case BUILTIN_CMD_UNSETENV:
        if (cmd->argc != 2) {
            fprintf(stderr, "lusush: unsetenv: takes one argument\n");
        }
        else {
            if (unsetenv(cmd->argv[1]) < 0) {
                perror("lusush: unsetenv");
            }
        }
        break;
    case BUILTIN_CMD_ALIAS:
        if (cmd->argc == 1) {
            print_alias();
        }
        else if (cmd->argc < 3) {
            fprintf(stderr, "lusush: alias: alias word replacement text\n");
        }
        else {
            strncpy(tmp, cmd->argv[2], MAXLINE);
            strncat(tmp, " ", 2);
            for (i=3; cmd->argv[i]; i++) {
                strncat(tmp, cmd->argv[i], MAXLINE);
                strncat(tmp, " ", 2);
            }
            set_alias(cmd->argv[1], &tmp);
            strncpy(tmp, "\0", 1);
        }
        break;
    case BUILTIN_CMD_UNALIAS:
        if (cmd->argc != 2) {
            fprintf(stderr, "lusush: unalias: unalias alias\n");
        }
        else {
            unset_alias(cmd->argv[1]);
        }
        break;
    case BUILTIN_CMD_SETOPT:
        if (cmd->argc != 2) {
            fprintf(stderr, "lusush: setopt: setopt option\n");
        }
        else {
            if (strncmp(cmd->argv[1], "VERBOSE_PRINT", MAXLINE) == 0) {
                set_bool_opt(VERBOSE_PRINT, true);
            }
            else if (strncmp(cmd->argv[1], "COLOR_PROMPT", MAXLINE) == 0) {
                set_bool_opt(COLOR_PROMPT, true);
            }
        }
        break;
    case BUILTIN_CMD_UNSETOPT:
        if (cmd->argc != 2) {
            fprintf(stderr, "lusush: unsetopt: unsetopt option\n");
        }
        else {
            if (strncmp(cmd->argv[1], "VERBOSE_PRINT", MAXLINE) == 0) {
                set_bool_opt(VERBOSE_PRINT, false);
            }
            else if (strncmp(cmd->argv[1], "COLOR_PROMPT", MAXLINE) == 0) {
                set_bool_opt(COLOR_PROMPT, false);
            }
        }
        break;
    case BUILTIN_CMD_SETPROMPT:
            set_prompt(cmd->argc, cmd->argv);
    }
}

