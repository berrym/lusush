/**
 * init.c - startup and initialization routines
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "lusush.h"
#include "init.h"
#include "errors.h"
#include "alias.h"
#include "opts.h"
#include "history.h"
#include "cmdlist.h"
#include "input.h"

// The type of shell instance
static int SHELL_TYPE;

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
int init(int argc, char **argv, FILE **in)
{
    struct stat st;             // stat buffer
    int optind = 0;             // index of option being parsed

    if (!argv)
        exit(EXIT_FAILURE);

    // Set all locales according to environment
    setlocale(LC_ALL, "");

    // Setup signal handlers
    setup_signal_handlers();

    // Parse command line options
    optind = parse_opts(argc, argv);

    // Determine the shell type
    if (**argv == '-') {
        SHELL_TYPE = LOGIN_SHELL;
    } else if (optind && argv[optind] && *argv[optind]) {
        // Check that argv[optind] is a regular file
        stat(argv[optind], &st);
        if (!S_ISREG(st.st_mode)) {
            error_message("lusush: %s is not a regular file.\n",
                          argv[optind]);
            optind = 0;
            SHELL_TYPE = INTERACTIVE_SHELL;
        } else {
            SHELL_TYPE = NORMAL_SHELL;
            if ((*in = fopen(argv[optind], "r")) == NULL)
                error_syscall("fopen");
        }
    } else {
        optind = 0;
        SHELL_TYPE = INTERACTIVE_SHELL;
    }

    // Get and set shell's pid in environment
    pid_t pid = getpid();
    char spid[MAX_PID_LEN] = { '\0' };
    snprintf(spid, MAX_PID_LEN, "%d", pid);
    setenv("$", spid, 1);

    // Initialize history
    init_history();

    // Set memory cleanup procedures on termination
    atexit(free_alias_list);
    atexit(free_command_list);
    atexit(free_line_read);
    atexit(free_history_list);
    atexit(save_history);

    return optind;
}

/**
 * setup_signal_handlers:
 *      Set signal actions for specific signals.
 */
void setup_signal_handlers(void) {
    setup_sigint_handler();
    setup_sigsegv_handler();
}

/**
 * setup_sigint_handler:
 *      Handle SIGINT, ignore it completely.
 */
void setup_sigint_handler(void) {
    struct sigaction prev_info, handler;

    if (sigaction(SIGINT, NULL, &prev_info) != -1) {
        handler.sa_handler = SIG_IGN;
        sigemptyset(&(handler.sa_mask));
        handler.sa_flags = 0;

        if (sigaction(SIGINT, &handler, &prev_info) == -1) {
            error_syscall("lusush: signal error");
        }
    } else {
        error_syscall("lusush: signal error");
    }
}

/**
 * setup_sigsegv_handler:
 *      Handle SIGSEGV, call sigsegv_handler to abort.
 */
void setup_sigsegv_handler(void) {
    struct sigaction prev_info, handler;

    if (sigaction(SIGSEGV, NULL, &prev_info) != -1) {
        handler.sa_handler = &sigsegv_handler;
        sigemptyset(&(handler.sa_mask));
        handler.sa_flags = 0;

        if (sigaction(SIGSEGV, &handler, &prev_info) == -1) {
            error_syscall("lusush: signal error");
        }
    } else {
        error_syscall("lusush: signal error");
    }
}
