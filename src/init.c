/**
 * init.c - startup and initialization routines
 */

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "lusush.h"
#include "init.h"
#include "misc.h"
#include "history.h"
#include "opts.h"
#include "prompt.h"

int SHELL_TYPE = 0;

/**
 * init:
 *      Performs initial tasks at shell startup.
 */
int init(int argc, char **argv)
{
    struct stat st;
    int optind = 0;

    if (!argv) {
        exit(EXIT_FAILURE);
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

    // Process command options
    optind = parse_opts(argc, argv);

    // Determine the shell type
    if (argv[0][0] == '-') {
        SHELL_TYPE = LOGIN_SHELL;
        vprint("THIS IS A LOGIN SHELL\n");
    }
    else if (optind) {
        // check that argv[1] is a regular file
        if (argv[optind] && *argv[optind]) {
            if (stat(argv[optind], &st)) {
                if (!S_ISREG(st.st_mode)) {
                    fprintf(stderr, 
                            "Lusush: %s is not a regular file.\n",
                            argv[1]);
                    optind = 0;
                    SHELL_TYPE = INTERACTIVE_SHELL;
                    vprint("THIS IS AN INTERACTIVE SHELL\n");
                }
                else {
                    SHELL_TYPE = NORMAL_SHELL;
                    vprint("THIS IS A NORMAL SHELL\n");
                }
            }
        }
        else {
            optind = 0;
            SHELL_TYPE = INTERACTIVE_SHELL;
            vprint("THIS IS AN INTERACTIVE SHELL\n");
        }
    }
    else {
        optind = 0;
        SHELL_TYPE = INTERACTIVE_SHELL;
        vprint("THIS IS AN INTERACTIVE SHELL\n");
    }


    // Initialize history
    init_history();

    return optind;
}

/**
 * sig_int:
 *      interupt ^C signal handler
 */
void sig_int(int signo)
{
    vprint("\nlusush: caught signal %d.\n", signo);
}

/**
 * NOTE: NECESSARALY FATAL
 * sig_seg:
 *      segmentation fault handler, insult programmer then abort
 */
void sig_seg(int signo)
{
    fprintf(stderr, "lusush: caught signal %d, terminating.\n", signo);
    fprintf(stderr, "\tAnd fix your damn code.\n");
    exit(EXIT_FAILURE);
}
