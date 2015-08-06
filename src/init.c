/**
 * init.c - startup and initialization routines
 *
 * Copyright (c) 2009-2015 Michael Berry <trismegustis@gmail.com>
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

#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "lusush.h"
#include "init.h"
#include "misc.h"
#include "history.h"
#include "opts.h"
#include "alias.h"

// The type of shell instance
static int SHELL_TYPE;

/**
 * sig_int:
 *      Interrupt ^C signal handler, ignore it for now.
 */
static void sig_int(int signo)
{
    vprint("\nlusush: caught signal %d.\n", signo);
}

/**
 * NOTE: NECESSARALY FATAL
 * sig_seg:
 *      Segmentation fault handler, insult programmer then abort.
 */
static void sig_seg(int signo)
{
    fprintf(stderr, "lusush: caught signal %d, terminating.\n", signo);
    fprintf(stderr, "\tAnd fix your damn code.\n");
    exit(EXIT_FAILURE);
}

/**
 * shell_type:
 *      Return the value that represents the type of the shell instance.
 */
int shell_type(void)
{
    return SHELL_TYPE;
}

/**
 * init:
 *      Performs initial tasks at shell startup.
 */
int init(int argc, char **argv)
{
    struct stat st;             // stat  buffer
    int optind = 0;             // index of option being parsed

    if (!argv)
        exit(EXIT_FAILURE);

    // Set all locales according to environment
    setlocale(LC_ALL, "");

    /*
     * Set up signal handlers
     *  sig_int: SIGINT handler
     *  sig_seg: SIGSEGV handler
     */
    if (signal(SIGINT, sig_int) == SIG_ERR) {
        fprintf(stderr, "lusush: signal error: %d\n", SIGINT);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGSEGV, sig_seg) == SIG_ERR) {
        fprintf(stderr, "lusush: signal error: %d\n", SIGSEGV);
        exit(EXIT_FAILURE);
    }

    // Set up aliases
    if (init_alias_list() < 0) {
        fprintf(stderr, "lusush: init_alias_list_failed\n");
        exit(EXIT_FAILURE);
    }

    // Process command options
    optind = parse_opts(argc, argv);

    // Determine the shell type
    if (**argv == '-') {
        SHELL_TYPE = LOGIN_SHELL;
        vprint("THIS IS A LOGIN SHELL\n");
    }
    else if (optind && argv[optind] && *argv[optind]) {
        // check that argv[optind] is a regular file
        stat(argv[optind], &st);
        if (!S_ISREG(st.st_mode)) {
            fprintf(stderr,
                    "lusush: %s is not a regular file.\n",
                    argv[optind]);
            optind = 0;
            SHELL_TYPE = INTERACTIVE_SHELL;
            vprint("THIS IS AN INTERACTIVE SHELL\n");
        }
        else {
            SHELL_TYPE = NORMAL_SHELL;
            vprint("THIS IS A NORMAL SHELL\n");
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
