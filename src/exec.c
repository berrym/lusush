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

////////////////////////// EXECUTE COMMAND FUNCTIONS /////////////////////////

int exec_cmd(CMD *cmd, int cnt)
{
    register int i;
    int ret, status;
    char *cmdpath;
    int pids[cnt];
    CMD *psave1, *psave2;

    psave1 = cmd;               // save current position in command history

    for (i = 0; i < cnt; i++) {
        pids[i] = 0;
    }

    for (i = 0; i < cnt; i++) {
        if ((cmdpath = (char *)calloc(MAXLINE, sizeof(char))) == (char *)NULL) {
            perror("lusush: calloc");
            global_cleanup();
            exit(EXIT_FAILURE);
        }

        if ((ret = is_builtin_cmd(cmd->argv[0])) != -1) {
            if (cmd->pipe) {
                printf("lusush: pipes are not supported for builtins\n");
                return i;
            }
            pids[i] = 0;
            exec_builtin_cmd(ret, cmd);
        }
        else {
            cmdpath = path_to_cmd(cmd->argv[0]);
            if (cmdpath && strcmp(cmdpath, "S_ISDIR") == 0) {
                print_debug("lusush: %s is a directory.\n",
                        cmd->argv[0]);
                cd(cmd->argv[0]);
            }
            else if (cmdpath) {
                strcpy(cmd->argv[0], cmdpath);
                if ((pids[i] = exec_external_cmd(cmd, (char **)NULL)) == -1) {
                    return -1;
                }
            }
            else {
                printf("lusush: command not found.\n");
                return i;
            }
            if (cmdpath)
                free(cmdpath);
            cmdpath = (char *)NULL;
        }
        if (cmd->next)
            cmd = cmd->next;
        else
            break;
    }

    psave2 = cmd;                       // save last place in command history
    cmd = psave1;                       // restore to inital offset

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

int exec_external_cmd(CMD *cmd, char **envp)
{
    int status,j;
    pid_t pid;
    CMD *tmp;

    // Check for invalid strings at the end of vector,
    // give back to free pool, they will mess up redirections
    for (j = 0; cmd->argv[j]; j++) {
        if (!*cmd->argv[j]) {
            cmd->argc--;
            free(cmd->argv[j]);
            cmd->argv[j] = (char *)NULL;
        }
    }

    /////////////////////////////////////////////////
    // Spawn a new process
    /////////////////////////////////////////////////

    pid = fork();
    switch (pid) {
        case -1:                    // fork error
            perror("lusush: fork");
            return;
        case 0:                     // child process
            if (cmd->pipe) {

                /////////////////////////////////////////////////
                // Configure a pipe
                /////////////////////////////////////////////////

                if (cmd->prev && cmd->prev->pipe) {
                    print_debug("*** Setting up pipe\n");
                    tmp = cmd->prev;
                    close(tmp->fd[1]);
                    if (tmp->fd[0] != STDIN_FILENO) {
                        if (dup2(tmp->fd[0], STDIN_FILENO) != STDIN_FILENO) {
                            fprintf(stderr, "lusush: dup2\n");
                            return -1;
                        }
                    }
                    close(tmp->fd[0]);
                }

                /////////////////////////////////////////////////
                // Create a pipe
                /////////////////////////////////////////////////

                if (cmd->next && cmd->next->pipe) {
                    tmp = cmd->next;
                    print_debug("**** Creating pipe\n");
                    pipe(cmd->fd);
                    if (cmd->fd[1] != STDOUT_FILENO) {
                        if (dup2(cmd->fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
                            fprintf(stderr, "lusus: dup2\n");
                            return -1;
                        }
                    }
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
                freopen(cmd->out_filename, "w", stdout);
            }

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
            if (envp) {
                print_debug("calling execve\n");
                execve(cmd->argv[0], cmd->argv, envp);
            }
            else {
                print_debug("calling execv\n");
                execv(cmd->argv[0], cmd->argv);
            }

            fprintf(stderr, "Could not execute: %s\n", cmd->argv[0]);
            exit(127);                  // exec shouldn't return ever
            break;
        default:                        // parent process
            return pid;
    }
}

int is_builtin_cmd(const char *cmdname)
{
    register int i;

    for (i = 0; i < BUILTIN_CMD_CNT; i += 2) {
        if (strcmp(cmdname, builtins[i]) == 0)
            return i;
    }

    return -1;
}

void exec_builtin_cmd(int cmdno, CMD *cmd)
{
    switch (cmdno) {
        case BUILTIN_CMD_EXIT:
            printf("Goodbye!\n");
            global_cleanup();
            exit(EXIT_SUCCESS);
            break;
        case BUILTIN_CMD_HELP:
            if (cmd->argv[1] && *cmd->argv[1]) {
                help(cmd->argv[1]);
            }
            else {
                help((char *)NULL);
            }
            break;
        case BUILTIN_CMD_CD:
            if (cmd->argv[1])
                cd(cmd->argv[1]);
            break;
        case BUILTIN_CMD_PWD:
            pwd();
            break;
        case BUILTIN_CMD_HISTORY:
#if defined( USING_READLINE )
            history();
#else
            history(cmd);
#endif
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
    }
}

/**
 * path_to_cmd:
 *      path_to_cmd appropriately enough will return a pointer with the absolute
 *      path to a command if it is found in path.  The value NULL is returned
 *      in the case that cmd is not found.  "S_ISDIR" is returned if the cmd is
 *      found but is a directory and not a regular file.  If the string given is
 *      already the absolute path name then it is returned.
 */
char *path_to_cmd(char *cmd)
{
    char *tok = (char *)NULL, *full_cmd = (char *)NULL, *ptr = (char *)NULL;
    struct stat cmd_st;
    char *isdir;
    if ((isdir = calloc(strlen("S_ISDIR")+1, sizeof(char))) == NULL) {
        perror("lusush: calloc");
        return (char *)NULL;
    }
    strcpy(isdir, "S_ISDIR");
    isdir[strlen(isdir)] = '\0';

    // First check if cmd is already the absolute path.
    if (stat(cmd, &cmd_st) == -1) {
        print_debug("\t%s: stat: %s\n", full_cmd, strerror(errno));
    }
    else {
        if (S_ISREG(cmd_st.st_mode))
            return cmd;
        else if (S_ISDIR(cmd_st.st_mode))
            return isdir;
    }

    ptr = strdup(ENV_PATH);
    tok = strtok(ptr, ": ");    // seperate tokens by ':' or ' '
    print_debug("Searching path for command\n");

    while (tok) {
        if ((full_cmd=(char *)calloc(MAXLINE, sizeof(char))) == (char *)NULL) {
            perror("calloc");
            return (char *)NULL;
        }
        full_cmd[0] = '\0';
        strcpy(full_cmd, tok);
        strcat(full_cmd, "/");
        strcat(full_cmd, cmd);

        // call stat
        if (stat(full_cmd, &cmd_st) < 0) {
            print_debug("\tlusush%s: stat: %s\n", full_cmd, strerror(errno));
        }
        else {
            if (S_ISREG(cmd_st.st_mode)) {     // is a regular file
                print_debug("\nCommand Found:\n\t%s\n", full_cmd);
                return full_cmd;
            }
        }

        free(full_cmd);
        full_cmd = (char *)NULL;

        tok = strtok((char *)NULL, ": ");
    }

    return (char *)NULL;
}
