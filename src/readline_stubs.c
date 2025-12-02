/*
 * Lusush - Readline Stubs for LLE-only builds
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This file provides stub implementations of readline_integration functions
 * when GNU Readline support is disabled (-Dreadline_support=false).
 * 
 * In LLE-only builds, these functions either:
 * - Return appropriate default values
 * - Do nothing (for configuration functions)
 * - Return false/NULL to indicate unavailability
 *
 * This allows the rest of the codebase to compile without #ifdef guards
 * everywhere, while LLE handles all actual line editing.
 */

#include "../include/readline_integration.h"
#include <stdlib.h>
#include <string.h>

#if !HAVE_READLINE

/* ============================================================================
 * MAIN READLINE INTERFACE - Stubs
 * ============================================================================ */

bool lusush_readline_init(void) {
    /* Readline not available - LLE will be used instead */
    return false;
}

char *lusush_readline(void) {
    /* Should not be called when readline is disabled */
    return NULL;
}

char *lusush_readline_with_prompt(const char *prompt) {
    (void)prompt;
    return NULL;
}

void lusush_readline_cleanup(void) {
    /* Nothing to clean up */
}

bool lusush_readline_available(void) {
    return false;  /* Readline is not available */
}

const char *lusush_readline_version(void) {
    return "GNU Readline: disabled (LLE-only build)";
}

/* ============================================================================
 * HISTORY MANAGEMENT - Stubs
 * ============================================================================ */

void lusush_history_add(const char *line) {
    (void)line;
    /* LLE handles its own history */
}

void lusush_history_save(void) {
    /* LLE handles its own history */
}

bool lusush_history_load(void) {
    return false;  /* LLE handles its own history */
}

void lusush_history_clear(void) {
    /* LLE handles its own history */
}

const char *lusush_history_get(int index) {
    (void)index;
    return NULL;
}

int lusush_history_length(void) {
    return 0;
}

int lusush_history_remove_duplicates(void) {
    return 0;
}

/* ============================================================================
 * TAB COMPLETION - Stubs
 * ============================================================================ */

void lusush_completion_setup(void) {
    /* LLE handles its own completion */
}

char *lusush_completion_generator(const char *text, int state) {
    (void)text;
    (void)state;
    return NULL;
}

char **lusush_completion_matches(const char *text, int start, int end) {
    (void)text;
    (void)start;
    (void)end;
    return NULL;
}

/* ============================================================================
 * SYNTAX HIGHLIGHTING - Stubs
 * ============================================================================ */

void lusush_syntax_highlighting_set_enabled(bool enabled) {
    (void)enabled;
}

bool lusush_syntax_highlighting_is_enabled(void) {
    return false;
}

int lusush_syntax_highlight_line(void) {
    return 0;
}

void lusush_show_command_syntax_preview(const char *command) {
    (void)command;
}

void lusush_syntax_highlighting_configure(const char *commands_color,
                                         const char *strings_color,
                                         const char *comments_color,
                                         const char *keywords_color) {
    (void)commands_color;
    (void)strings_color;
    (void)comments_color;
    (void)keywords_color;
}

/* ============================================================================
 * PROMPT INTEGRATION - Stubs
 * ============================================================================ */

char *lusush_generate_prompt(void) {
    return NULL;  /* LLE handles prompts */
}

void lusush_prompt_update(void) {
    /* LLE handles prompts */
}

void lusush_prompt_set_callback(lusush_prompt_callback_t callback) {
    (void)callback;
}

/* ============================================================================
 * KEY BINDINGS - Stubs
 * ============================================================================ */

void lusush_keybindings_setup(void) {
    /* LLE handles its own keybindings */
}

void lusush_update_editing_mode(void) {
    /* LLE handles editing modes */
}

int lusush_keybinding_add(int key, rl_command_func_t *function) {
    (void)key;
    (void)function;
    return -1;  /* Not supported */
}

int lusush_keybinding_remove(int key) {
    (void)key;
    return -1;  /* Not supported */
}

/* ============================================================================
 * CONFIGURATION - Stubs
 * ============================================================================ */

void lusush_readline_configure(bool show_completions_immediately,
                              bool case_insensitive_completion,
                              bool colored_completion_prefix) {
    (void)show_completions_immediately;
    (void)case_insensitive_completion;
    (void)colored_completion_prefix;
}

void lusush_history_set_max_length(int max_length) {
    (void)max_length;
}

void lusush_multiline_set_enabled(bool enabled) {
    (void)enabled;
}

bool lusush_history_set_file(const char *filepath) {
    (void)filepath;
    return false;
}

/* ============================================================================
 * UTILITY FUNCTIONS - Stubs
 * ============================================================================ */

void lusush_clear_screen(void) {
    /* Can be implemented with ANSI escapes if needed */
}

void lusush_refresh_line(void) {
    /* LLE handles display refresh */
}

int lusush_get_cursor_position(void) {
    return 0;
}

bool lusush_set_cursor_position(int position) {
    (void)position;
    return false;
}

const char *lusush_get_line_buffer(void) {
    return NULL;
}

void lusush_set_line_buffer(const char *new_content) {
    (void)new_content;
}

/* ============================================================================
 * ERROR HANDLING - Stubs
 * ============================================================================ */

const char *lusush_readline_get_error(void) {
    return "GNU Readline support is disabled";
}

void lusush_readline_set_debug(bool enabled) {
    (void)enabled;
}

bool lusush_readline_is_debug_enabled(void) {
    return false;
}

/* ============================================================================
 * COMPATIBILITY LAYER - Stubs
 * ============================================================================ */

void lusush_add_completion(lusush_completions_t *lc, const char *completion) {
    if (!lc || !completion) return;
    
    char **new_cvec = realloc(lc->cvec, (lc->len + 1) * sizeof(char *));
    if (new_cvec) {
        lc->cvec = new_cvec;
        lc->cvec[lc->len] = strdup(completion);
        if (lc->cvec[lc->len]) {
            lc->len++;
        }
    }
}

void lusush_free_completions(lusush_completions_t *lc) {
    if (!lc) return;
    
    for (size_t i = 0; i < lc->len; i++) {
        free(lc->cvec[i]);
    }
    free(lc->cvec);
    lc->cvec = NULL;
    lc->len = 0;
}

/* ============================================================================
 * INTEGRATION HOOKS - Stubs
 * ============================================================================ */

void lusush_set_pre_input_hook(lusush_pre_input_hook_t hook) {
    (void)hook;
}

void lusush_set_post_input_hook(lusush_post_input_hook_t hook) {
    (void)hook;
}

void lusush_show_highlight_performance(void) {
    /* No performance data when readline is disabled */
}

void lusush_set_debug_enabled(bool enabled) {
    (void)enabled;
}

#endif /* !HAVE_READLINE */
