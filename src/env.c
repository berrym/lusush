#include <stdlib.h>
#include <string.h>
#include "ldefs.h"
#include "misc.h"
#include "env.h"

/**
 * Default environment variables
 *      ENV_HOME        users' home directory
 *      ENV_PATH        users' path to programs
 *      ENV_PROMPT      users' shell prompt
 *      ENV_SHELL       users' login shell
 *      ENV_LOGNAME     users' login name
 *      ENV_MAIL        users' email address
 */
char *ENV_HOME = (char *)NULL;
char *ENV_PATH = (char *)NULL;
char *ENV_PROMPT = (char *)NULL;
char *ENV_SHELL = (char *)NULL;
char *ENV_LOGNAME = (char *)NULL;
char *ENV_MAIL = (char *)NULL;

int SHELL_TYPE = 0;

/**
 * env_init:
 *      env_init will initialize some environment/global variables
 *      and set their values, it should only be called once.
 */
void env_init(char **argv)
{
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

    // set the user's prompt
    build_prompt();
}
