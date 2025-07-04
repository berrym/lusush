#include "../include/config.h"
#include "../include/linenoise/linenoise.h"
#include "../include/lusush.h"
#include "../include/symtable.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Legacy shell options - now using config system

void init_shell_opts(void) {
    // Legacy function - now uses config system values
    symtable_set_global_int("HISTORY_NO_DUPS", config.history_no_dups);
    symtable_set_global_int("NO_WORD_EXPAND", config.no_word_expand);
    symtable_set_global_int("ENHANCED_COMPLETION", config.completion_enabled);
}

/* Getter function for enhanced completion option */
bool get_enhanced_completion(void) { return config.completion_enabled; }

/* Getter function for no word expand option */
bool get_no_word_expand(void) { return config.no_word_expand; }
