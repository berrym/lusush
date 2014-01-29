/**
 * opts.c - runtime option parsing and configuration
 *
 * Copyright (c) 2009-2014 Michael Berry <trismegustis@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "lusush.h"
#include "opts.h"
#include "prompt.h"

#define DBGSTR "lusush: opts.c: "

static bool OPTIONS[OPT_CNT] = { false };

/**
 * usage:
 *      command line switches
 */
static void usage(int ret) {
    fprintf(stderr, "Usage:\n\t-h Help\n\t");
    fprintf(stderr, "-c Color prompt\n\t");
    fprintf(stderr, "-v Verbose ouput\n");
    exit(ret);
}

/**
 * set_bool_opt:
 *      set boolean option to moddify runtime behaviour
 */
bool set_bool_opt(unsigned int opt, bool val)
{
    switch (opt) {
    case VERBOSE_PRINT:
        OPTIONS[VERBOSE_PRINT] = val;
        break;
    case COLOR_PROMPT:
        OPTIONS[COLOR_PROMPT] = val;
        build_prompt();
        break;
    default:
        fprintf(stderr, "%sset_bool_opt: unknown option %u\n", DBGSTR, opt);
        return false;
    }

    return true;
}

/**
 * opt_is_set:
 *     returns boolean value for runtime options
 */
bool opt_is_set(int index)
{
    return OPTIONS[index];
}

/**
 * parse_opts:
 *      parse command line switches using getopt framework
 */
int parse_opts(int argc, char **argv)
{
    // next option
    int nopt = 0;
    // string of valid short options
    const char *sopts = "hcv";
    // array describing valid long options
    const struct option lopts[] = {
        { "help", 0, NULL, 'h' },
        { "color-prompt", 0, NULL, 'c' },
        { "verbose", 0, NULL, 'v' },
        { NULL, 0, NULL, 0 }
    };

    do {
        nopt = getopt_long(argc, argv, sopts, lopts, NULL);

        switch (nopt) {
        case 'h':
            usage(EXIT_SUCCESS);
            break;
        case 'c':
            set_bool_opt(COLOR_PROMPT, true);
            break;
        case 'v':
            set_bool_opt(VERBOSE_PRINT, true);
            break;
        case '?':
            usage(EXIT_FAILURE);
        case -1:
            break;
        default:
            abort();
        }
    } while (nopt != -1);

    return optind;
}
