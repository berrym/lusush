/*
 * init.c - startup and initialization routines
 */

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "ldefs.h"
#include "init.h"
#include "misc.h"
#include "env.h"
#include "history.h"

/*
 * init:
 *     Performs initial tasks at shell startup.
 */
void init(int argc, char **argv)
{
    // Determine the shell type
    if (argv[0][0] == '-') {
        SHELL_TYPE = LOGIN_SHELL;
        print_debug("THIS IS A LOGIN SHELL\n");
    }
    else if(argc > 1) {
        SHELL_TYPE = NORMAL_SHELL;
        print_debug("THIS IS A NORMAL SHELL\n");
    }
    else {
        SHELL_TYPE = INTERACTIVE_SHELL;
        print_debug("THIS IS AN INTERACTIVE SHELL\n");
    }

    /*
     * Set up signal handlers
     *      sig_int:        SIGINT handler
     *      sig_seg:        SIGSEGV handler
     */
    if (signal(SIGINT, sig_int) == SIG_ERR) {
        fprintf(stderr, "lusush: signal error: %d\n", SIGINT);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGSEGV, sig_seg) == SIG_ERR) {
        fprintf(stderr, "lusush: signal error: %d\n", SIGSEGV);
        exit(EXIT_FAILURE);
    }

    // Set the initial environment
    env_init(argv);

    // Initialize history
    init_history();
}

/*
 * sig_int:
 *      interupt ^C signal handler
 */
void sig_int(int signo)
{
    print_debug("\nlusush: caught signal %d.\n", signo);
}

/*
 * NOTE: NECESSARALY FATAL
 * sig_seg:
 *      segmentation fault handler, free memory where it can, then exit
 */
void sig_seg(int signo)
{
    fprintf(stderr, "lusush: caught signal %d, terminating.\n", signo);
    fprintf(stderr, "\tAnd fix your damn code.\n");
    exit(EXIT_FAILURE);
}

