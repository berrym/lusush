/*
 * opts.c - runtime option parsing and configuration
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "ldefs.h"
#include "opts.h"
#include "prompt.h"

#define DBGSTR "lusush: opts.c: "

static bool OPTIONS[OPT_CNT] = { false };

static void usage(int ret) {
    fprintf(stderr, "Usage:\n\t-h Help\n\t");
    fprintf(stderr, "-c Color prompt\n\t");
    fprintf(stderr, "-v Verbose ouput\n");
    exit(ret);
}

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

bool opt_is_set(int index)
{
    return OPTIONS[index];
}

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
