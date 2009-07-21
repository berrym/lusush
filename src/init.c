#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "init.h"
#include "misc.h"
#include "env.h"

/**
 * init:
 *     Performs initial tasks at shell startup.
 */
void init(char **argv)
{
    // Determine the shell type
    if (argv[0][0] == '-') {
        SHELL_TYPE = SHELL_TYPE_LOGIN;
        print_debug("THIS IS A LOGIN SHELL\n");
    }
    else {
        SHELL_TYPE = SHELL_TYPE_INTERACTIVE;
        print_debug("THIS IS AN INTERACTIVE SHELL\n");
    }

    /*
     * Set up signal handlers
     *      sig_int:        SIGINT handler
     *      sig_seg:        SIGSEGV handler
     */
    if (signal(SIGINT, sig_int) == SIG_ERR) {
        fprintf(stderr, "lusush: signal error: %d\n", SIGINT);
        global_cleanup();
        exit(EXIT_FAILURE);
    }

    if (signal(SIGSEGV, sig_seg) == SIG_ERR) {
        fprintf(stderr, "lusush: signal error: %d\n", SIGSEGV);
        global_cleanup();
        exit(EXIT_FAILURE);
    }

    // Set stdout, stdin, and stderr to a non-buffered state
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Set the initial environment
    env_init(argv);
}

/**
 * sig_int:
 *      interupt ^C signal handler, free memory where it can, then exit
 */
void sig_int(int signo)
{
    fprintf(stderr, "lusush: caught signal %d, terminating.\n", signo);
    fflush(stderr);
    global_cleanup();
    exit(EXIT_FAILURE);
}

/**
 * NOTE: NECESSARALY FATAL
 * sig_seg:
 *      segmentation fault handler, free memory where it can, then exit
 */
void sig_seg(int signo)
{
    fprintf(stderr, "lusush: caught signal %d, terminating.\n", signo);
    fprintf(stderr, "\tAnd fix your damn code.\n");
    fflush(stderr);
    global_cleanup();
    exit(EXIT_FAILURE);
}
