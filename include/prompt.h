#ifndef PROMPT_H
#define PROMPT_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Intelligent Prompt Caching System
typedef struct {
    char cached_prompt[512];
    char working_directory[256];
    char git_branch[64];
    char theme_name[32];
    time_t git_status_time;
    time_t cache_time;
    bool is_valid;
    bool git_dirty;
    int git_ahead;
    int git_behind;
} lusush_prompt_cache_t;

void build_prompt(void);
void rebuild_prompt(void);
void update_git_info(void);
void format_git_prompt(char *git_prompt, size_t size);

// Cache optimization functions
bool prompt_cache_init(void);
void prompt_cache_cleanup(void);
bool prompt_cache_get(lusush_prompt_cache_t *entry);
void prompt_cache_set(const lusush_prompt_cache_t *entry);
void prompt_cache_invalidate(void);
bool prompt_cache_is_valid_for_context(void);

#endif
