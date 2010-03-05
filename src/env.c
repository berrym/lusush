/*
 * env.c - environmentally aware
 */

#include <stdlib.h>
#include <string.h>
#include "ldefs.h"
#include "misc.h"
#include "env.h"
#include "prompt.h"

int SHELL_TYPE = 0;

/*
 * env_init:
 *      env_init will initialize some environment/global variables
 *      and set their values, it should only be called once.
 */
void env_init(char **argv)
{
    /*
     * Default environment variables
     *      HOME        users' home directory
     *      PATH        users' path to programs
     *      PROMPT      users' shell prompt
     *      SHELL       users' login shell
     *      LOGNAME     users' login name
     *      MAIL        users' email address
     */
    char *ENV_HOME = NULL;
    char *ENV_PATH = NULL;
    char *ENV_PROMPT = NULL;
    char *ENV_SHELL = NULL;
    char *ENV_LOGNAME = NULL;
    char *ENV_MAIL = NULL;

    // get the user's home directory
    ENV_HOME = getenv("HOME");
    print_debug("*** HOME IS SET\n%s\n\n", ENV_HOME);

    // get the user's PATH string
    ENV_PATH = getenv("PATH");
    print_debug("*** PATH IS SET\n%s\n\n", ENV_PATH);

    // get the user's shell, if it is null then set it.
    ENV_SHELL = getenv("SHELL");
    if (ENV_SHELL == NULL) {
        setenv("SHELL", argv[0], 1);
        ENV_SHELL = getenv("SHELL");
    }
    print_debug("*** SHELL IS SET\n%s\n\n", ENV_SHELL);

    // get the users login name
    ENV_LOGNAME = getenv("LOGNAME");
    print_debug("*** LOGNAME IS SET\n%s\n\n", ENV_LOGNAME);

    // get the users mail directory
    ENV_MAIL = getenv("MAIL");
    print_debug("*** MAIL IS SET\n%s\n\n", ENV_MAIL);

    // set the users prompt
    build_prompt();
    // get the users prompt
    ENV_PROMPT = getenv("PROMPT");
    print_debug("*** PROMPT IS SET\n%s\n\n", ENV_PROMPT);

    // Set everything back to NULL
    ENV_LOGNAME = NULL;
    ENV_HOME = NULL;
    ENV_PATH = NULL;
    ENV_PROMPT = NULL;
    ENV_SHELL = NULL;
    ENV_MAIL = NULL;
}

