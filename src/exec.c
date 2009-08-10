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

/**
 * TODO: Figure out what is wrong with this function.  It causes some
 * serious (aka FATAL) issues when called.
 *
 * exec_cmdl:
 *      a wrapper to exec_builtin_cmd and exec_external_cmd.
 */
void exec_cmdl(CMDLIST *cmdl, char **envp)
{
    register int i;
    CMD *cmd;

    // Check that parameters are not NULL
    if (cmdl == NULL)
        return;

    cmd = cmdl->head;       // Point cmd to beginning of command list

    // Execute commands in list     
    for (i = 0; i < cmdl->size; i++) {
        if (is_builtin_cmd(cmd->argv[0]) == true)
            exec_builtin_cmd(cmd);
        else
            exec_external_cmd(cmd, envp);

        cmd = cmd->next;    // Point cmd to next command
    }
}

void exec_external_cmd(CMD *cmd, char **envp)
{
    int status,j;
    pid_t pid;

    // Check for invalid strings at the end of vector,
    // give back to free pool, they will mess up redirections
    for (j = 0; cmd->argv[j]; j++) {
        if (!*cmd->argv[j]) {
            cmd->argc--;
            free(cmd->argv[j]);
            cmd->argv[j] = (char *)NULL;
        }
    }

    // Spawn a new process
    if ((pid = fork()) < 0) {
        perror("lusush: fork");
        return;
    }
    else if (pid == 0) {                      // child process
        // Input redirection
        if (cmd->in_redirect) {
            close(STDIN_FILENO);
            freopen(cmd->in_filename, "r", stdin);
        }
        // Output redirection
        if (cmd->out_redirect) {
            close(STDOUT_FILENO);
            freopen(cmd->out_filename, "w", stdout);
        }
        // Close stdin and stdout if executing in the background
        // and then redirect them to /dev/null
        if (cmd->background && !cmd->out_redirect) {
            if (!cmd->in_redirect)
                close(STDIN_FILENO);
            close(STDOUT_FILENO);
            freopen("/dev/null", "r", stdin);
            freopen("/dev/null", "w", stderr);
        }
        if (envp != NULL) {
            print_debug("calling execve\n");
            execve(cmd->argv[0], cmd->argv, envp);
        }
        else {
            print_debug("calling execv\n");
            execv(cmd->argv[0], cmd->argv);
        }
        fprintf(stderr, "Could not execute: %s\n", cmd->argv[0]);
        exit(127);
    }

    /*
     * If executing the command in the background, call waitpid with
     * the WNOHANG option, otherwise pass 0 to block.
     */
    if (cmd->background) {
        if ((pid = waitpid(pid, &status, WNOHANG)) == -1) // parent
            perror("lusush: waitpid");
    }
    else {
        if ((pid = waitpid(pid, &status, 0)) == -1) // parent
            perror("lusush: waitpid");
    } 
}

bool is_builtin_cmd(const char *cmdname)
{
    register int i;

    for (i = 0; i < BUILTIN_CMD_CNT; i++) {
        if (strcmp(cmdname, builtin_cmds[i]) == 0)
            return true;
    }

    return false;
}

void exec_builtin_cmd(CMD *cmd)
{
    if (strcmp(cmd->argv[0], builtin_cmds[0]) == 0) {
        printf("Goodbye!\n");
        global_cleanup();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(cmd->argv[0], builtin_cmds[BUILTIN_CMD_CD]) == 0) {
        if (cmd->argv[1] != NULL)
            cd(cmd->argv[1]);
    }
    else if (strcmp(cmd->argv[0], builtin_cmds[BUILTIN_CMD_PWD]) == 0) {
        pwd();
    }
    else if (strcmp(cmd->argv[0], builtin_cmds[BUILTIN_CMD_HISTORY]) == 0) {
#if defined( USING_READLINE )
        history();
#else
        history(cmd);
#endif
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
    char *tok = NULL, *full_cmd = NULL, *ptr = NULL;
    struct stat cmd_st;
    char *isdir;
    if ((isdir = calloc(strlen("S_ISDIR")+1, sizeof(char))) == NULL) {
        perror("lusush: calloc");
        return NULL;
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

    while (tok != NULL) {
        full_cmd = (char *)calloc(MAXLINE, sizeof(char));
        if (full_cmd == NULL) {
            perror("calloc");
            return NULL;
        }
        full_cmd[0] = '\0';
        strcpy(full_cmd, tok);
        strcat(full_cmd, "/");
        strcat(full_cmd, cmd);

        // call stat
        if (stat(full_cmd, &cmd_st) == -1) {
            print_debug("\t%s: stat: %s\n", full_cmd, strerror(errno));
        }
        else {
            if (S_ISREG(cmd_st.st_mode)) {     // is a regular file
                print_debug("\nCommand Found:\n\t%s\n", full_cmd);
                return full_cmd;
            }
        }

        free(full_cmd);
        full_cmd = NULL;

        tok = strtok(NULL, ": ");
    }

    return NULL;
}
