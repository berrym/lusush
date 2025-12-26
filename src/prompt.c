#include "prompt.h"

#include "config.h"
#include "display_integration.h"
#include "lle/async_worker.h"
#include "lusush.h"
#include "symtable.h"
#include "themes.h"

#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Prompt Caching System
static char cached_prompt[512] = {0};
static char cached_working_dir[256] = {0};
static char cached_theme_name[32] = {0};
static int cached_symbol_mode = -1;
static time_t cache_time = 0;
static bool cache_valid = false;
static const int CACHE_VALIDITY_SECONDS = 5; // Cache valid for 5 seconds
static const int GIT_CACHE_SECONDS = 10;     // Git status cached for 10 seconds

// Git status information
typedef struct {
    char branch[256];
    int has_changes;
    int has_staged;
    int has_untracked;
    int ahead;
    int behind;
} git_info_t;

static git_info_t git_info = {0};
static time_t last_git_check = 0;

// Async git status system
static lle_async_worker_t *git_async_worker = NULL;
static pthread_mutex_t git_async_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool git_async_pending = false;
static char git_async_cwd[PATH_MAX] = {0};

/**
 * run_command:
 *      Execute a command and capture its output.
 *      Returns 0 on success, non-zero on failure.
 */
static int run_command(const char *cmd, char *output, size_t output_size) {
    FILE *fp;
    int status;
    char drain_buffer[256];

    if (output) {
        output[0] = '\0';
    }

    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }

    if (output && output_size > 0) {
        if (fgets(output, output_size, fp) != NULL) {
            // Remove trailing newline
            size_t len = strlen(output);
            if (len > 0 && output[len - 1] == '\n') {
                output[len - 1] = '\0';
            }
        }
    }

    // Drain any remaining output to prevent SIGPIPE to child process
    while (fgets(drain_buffer, sizeof(drain_buffer), fp) != NULL) {
        // Discard
    }

    status = pclose(fp);
    return WEXITSTATUS(status);
}

/**
 * get_git_branch:
 *      Get the current git branch name.
 */
static int get_git_branch(char *branch, size_t branch_size) {
    return run_command("git branch --show-current 2>/dev/null", branch,
                       branch_size);
}

/**
 * get_git_status:
 *      Get git repository status information.
 */
static void get_git_status(git_info_t *info) {
    char output[256];

    // Reset info
    memset(info, 0, sizeof(git_info_t));

    // Check if we're in a git repository (suppress stderr to avoid "fatal:"
    // messages)
    if (run_command("git rev-parse --git-dir 2>/dev/null", NULL, 0) != 0) {
        return; // Not in a git repository
    }

    // Get branch name
    if (get_git_branch(info->branch, sizeof(info->branch)) != 0) {
        strcpy(info->branch, "unknown");
    }

    // Check for staged changes
    if (run_command("git diff --cached --quiet", NULL, 0) != 0) {
        info->has_staged = 1;
    }

    // Check for unstaged changes
    if (run_command("git diff --quiet", NULL, 0) != 0) {
        info->has_changes = 1;
    }

    // Check for untracked files
    if (run_command("git ls-files --others --exclude-standard", output,
                    sizeof(output)) == 0 &&
        strlen(output) > 0) {
        info->has_untracked = 1;
    }

    // Check ahead/behind status - only if upstream exists
    // Suppress stderr to prevent "fatal: no upstream configured" warnings
    if (run_command("git rev-parse --abbrev-ref --symbolic-full-name "
                    "@{upstream} 2>/dev/null",
                    NULL, 0) == 0) {
        if (run_command("git rev-list --count --left-right @{upstream}...HEAD",
                        output, sizeof(output)) == 0) {
            sscanf(output, "%d\t%d", &info->behind, &info->ahead);
        }
    }
}

/**
 * update_git_info:
 *      Update git information with caching.
 */
void update_git_info(void) {
    time_t now = time(NULL);

    // Use cached info if it's recent enough
    if (now - last_git_check < GIT_CACHE_SECONDS) {
        return;
    }

    get_git_status(&git_info);
    last_git_check = now;
}

/**
 * format_git_prompt:
 *      Format git information for prompt display.
 */
void format_git_prompt(char *git_prompt, size_t size) {
    if (strlen(git_info.branch) == 0) {
        git_prompt[0] = '\0';
        return;
    }

    char status_indicators[32] = "";

    // Add status indicators
    if (git_info.has_staged) {
        strcat(status_indicators, "+");
    }
    if (git_info.has_changes) {
        strcat(status_indicators, "*");
    }
    if (git_info.has_untracked) {
        strcat(status_indicators, "?");
    }

    // Add ahead/behind indicators (↑/↓ are 3 bytes each in UTF-8)
    char ahead_behind[32] = "";
    if (git_info.ahead > 0 && git_info.behind > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), " ↑%d↓%d", git_info.ahead,
                 git_info.behind);
    } else if (git_info.ahead > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), " ↑%d", git_info.ahead);
    } else if (git_info.behind > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), " ↓%d", git_info.behind);
    }

    // Format the complete git prompt
    if (strlen(status_indicators) > 0 || strlen(ahead_behind) > 0) {
        snprintf(git_prompt, size, " (%s%s%s%s)", git_info.branch,
                 strlen(status_indicators) > 0 ? " " : "", status_indicators,
                 ahead_behind);
    } else {
        snprintf(git_prompt, size, " (%s)", git_info.branch);
    }
}

/**
 * build_prompt:
 *      Builds the user's prompt, either a fancy colored one with
 *      the current working directory or a plain '% or # '.
 *
 *      If config.use_theme_prompt is false, this function returns early
 *      without modifying PS1/PS2, allowing user customization to be respected.
 */
void build_prompt(void) {
    // Check if theme system is disabled - respect user PS1/PS2
    if (!config.use_theme_prompt) {
        // User has disabled theme prompts - don't overwrite PS1/PS2
        // If PS1 is not set at all, provide a minimal default
        const char *current_ps1 = symtable_get_global("PS1");
        if (!current_ps1 || current_ps1[0] == '\0') {
            symtable_set_global("PS1", (getuid() > 0) ? "$ " : "# ");
        }
        const char *current_ps2 = symtable_get_global("PS2");
        if (!current_ps2 || current_ps2[0] == '\0') {
            symtable_set_global("PS2", "> ");
        }
        return;
    }

    char prompt[(MAXLINE * 2) + 1] = {'\0'}; // prompt string

    // Enhanced Performance Monitoring: Start timing for prompt generation
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Initialize cache system if needed (simplified)
    static bool cache_initialized = false;
    if (!cache_initialized) {
        prompt_cache_init();
        cache_initialized = true;
    }

    // Simplified Prompt Caching: Check if cache is valid
    time_t now = time(NULL);
    char current_dir[256];
    bool cache_hit = false;

    if (cache_valid && (now - cache_time <= CACHE_VALIDITY_SECONDS)) {
        if (getcwd(current_dir, sizeof(current_dir)) != NULL &&
            strcmp(current_dir, cached_working_dir) == 0) {

            // Check theme hasn't changed
            if ((!config.theme_name && cached_theme_name[0] == '\0') ||
                (config.theme_name &&
                 strcmp(config.theme_name, cached_theme_name) == 0)) {

                // Check symbol mode hasn't changed
                int current_symbol_mode = (int)symbol_get_compatibility_mode();
                if (cached_symbol_mode == current_symbol_mode) {

                    // Cache hit! Use cached prompt
                    symtable_set_global("PS1", cached_prompt);
                    display_integration_record_cache_operation(true);
                    cache_hit = true;

                    // Enhanced Performance Monitoring: Record timing for cache
                    // hit
                    gettimeofday(&end_time, NULL);
                    uint64_t operation_time_ns =
                        ((uint64_t)(end_time.tv_sec - start_time.tv_sec)) *
                            1000000000ULL +
                        ((uint64_t)(end_time.tv_usec - start_time.tv_usec)) *
                            1000ULL;
                    display_integration_record_display_timing(
                        operation_time_ns);
                    return;
                }
            }
        }
    }

    // Cache miss - record and continue with generation
    if (!cache_hit) {
        display_integration_record_cache_operation(false);
    }

    // Generate prompt using theme system
    // theme_generate_primary_prompt() has its own fallback to "$ " if no theme
    if (!theme_generate_primary_prompt(prompt, sizeof(prompt))) {
        // Ultimate fallback - should never happen
        snprintf(prompt, sizeof(prompt), "%s", (getuid() > 0) ? "$ " : "# ");
    }
    symtable_set_global("PS1", prompt);

    // Generate PS2 using theme system
    char ps2_prompt[MAXLINE + 1] = {'\0'};
    if (!theme_generate_secondary_prompt(ps2_prompt, sizeof(ps2_prompt))) {
        // Fallback PS2
        snprintf(ps2_prompt, sizeof(ps2_prompt), "> ");
    }
    symtable_set_global("PS2", ps2_prompt);

    // Cache the generated prompt for future use
    strncpy(cached_prompt, prompt, sizeof(cached_prompt) - 1);
    cached_prompt[sizeof(cached_prompt) - 1] = '\0';

    if (getcwd(cached_working_dir, sizeof(cached_working_dir)) != NULL) {
        if (config.theme_name) {
            strncpy(cached_theme_name, config.theme_name,
                    sizeof(cached_theme_name) - 1);
            cached_theme_name[sizeof(cached_theme_name) - 1] = '\0';
        } else {
            cached_theme_name[0] = '\0';
        }
        cached_symbol_mode = (int)symbol_get_compatibility_mode();
        cache_time = now;
        cache_valid = true;
    }

    // Enhanced Performance Monitoring: Record prompt generation timing
    gettimeofday(&end_time, NULL);
    uint64_t operation_time_ns =
        ((uint64_t)(end_time.tv_sec - start_time.tv_sec)) * 1000000000ULL +
        ((uint64_t)(end_time.tv_usec - start_time.tv_usec)) * 1000ULL;
    display_integration_record_display_timing(operation_time_ns);
}

void rebuild_prompt(void) {
    // Invalidate cache when explicitly rebuilding
    cache_valid = false;
    build_prompt();
}

// ============================================================================
// INTELLIGENT PROMPT CACHING SYSTEM
// ============================================================================

/**
 * Initialize the prompt caching system.
 */
bool prompt_cache_init(void) {
    cache_valid = false;
    cache_time = 0;
    cached_prompt[0] = '\0';
    cached_working_dir[0] = '\0';
    cached_theme_name[0] = '\0';
    cached_symbol_mode = -1;
    return true;
}

/**
 * Cleanup the prompt caching system.
 */
void prompt_cache_cleanup(void) {
    cache_valid = false;
    cache_time = 0;
    cached_prompt[0] = '\0';
    cached_working_dir[0] = '\0';
    cached_theme_name[0] = '\0';
    cached_symbol_mode = -1;
}

/**
 * Check if current cache is valid for current context.
 */
bool prompt_cache_is_valid_for_context(void) { return cache_valid; }

/**
 * Get cached prompt if valid.
 */
bool prompt_cache_get(lusush_prompt_cache_t *entry) {
    (void)entry; /* Cache check handled directly in build_prompt */
    return false;
}

/**
 * Cache a prompt with current context.
 */
void prompt_cache_set(const lusush_prompt_cache_t *entry) {
    (void)entry; /* Caching handled directly in build_prompt */
}

/**
 * Invalidate the prompt cache.
 */
void prompt_cache_invalidate(void) { cache_valid = false; }

// ============================================================================
// ASYNC GIT STATUS SYSTEM
// ============================================================================

/**
 * Callback when async git status completes.
 * Updates the cached git_info with fresh data.
 */
static void git_async_completion(const lle_async_response_t *response,
                                 void *user_data) {
    (void)user_data;

    pthread_mutex_lock(&git_async_mutex);

    if (response->result == LLE_SUCCESS) {
        const lle_git_status_data_t *status = &response->data.git_status;

        // Update cached git_info from async response
        memset(&git_info, 0, sizeof(git_info));

        if (status->is_git_repo) {
            snprintf(git_info.branch, sizeof(git_info.branch), "%s", status->branch);
            git_info.has_staged = status->has_staged ? 1 : 0;
            git_info.has_changes = status->has_unstaged ? 1 : 0;
            git_info.has_untracked = status->has_untracked ? 1 : 0;
            git_info.ahead = status->ahead;
            git_info.behind = status->behind;
        }

        last_git_check = time(NULL);
    }

    git_async_pending = false;
    pthread_mutex_unlock(&git_async_mutex);

    // Invalidate prompt cache so next prompt shows fresh git info
    prompt_cache_invalidate();
}

/**
 * Initialize the async git status system.
 * Should be called during shell initialization.
 */
bool prompt_async_init(void) {
    if (git_async_worker != NULL) {
        return true; // Already initialized
    }

    lle_result_t result = lle_async_worker_init(&git_async_worker,
                                                 git_async_completion, NULL);
    if (result != LLE_SUCCESS) {
        return false;
    }

    result = lle_async_worker_start(git_async_worker);
    if (result != LLE_SUCCESS) {
        lle_async_worker_destroy(git_async_worker);
        git_async_worker = NULL;
        return false;
    }

    return true;
}

/**
 * Cleanup the async git status system.
 * Should be called during shell cleanup.
 */
void prompt_async_cleanup(void) {
    if (git_async_worker == NULL) {
        return;
    }

    lle_async_worker_shutdown(git_async_worker);
    lle_async_worker_wait(git_async_worker);
    lle_async_worker_destroy(git_async_worker);
    git_async_worker = NULL;
}

/**
 * Queue an async git status fetch for the current directory.
 * Returns immediately - results will update git_info via callback.
 */
void prompt_async_refresh_git(void) {
    if (git_async_worker == NULL || !lle_async_worker_is_running(git_async_worker)) {
        return;
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return;
    }

    pthread_mutex_lock(&git_async_mutex);

    // Don't queue if already pending for same directory
    if (git_async_pending && strcmp(git_async_cwd, cwd) == 0) {
        pthread_mutex_unlock(&git_async_mutex);
        return;
    }

    lle_async_request_t *req = lle_async_request_create(LLE_ASYNC_GIT_STATUS);
    if (req == NULL) {
        pthread_mutex_unlock(&git_async_mutex);
        return;
    }

    snprintf(req->cwd, sizeof(req->cwd), "%s", cwd);

    if (lle_async_worker_submit(git_async_worker, req) == LLE_SUCCESS) {
        git_async_pending = true;
        snprintf(git_async_cwd, sizeof(git_async_cwd), "%s", cwd);
    } else {
        lle_async_request_free(req);
    }

    pthread_mutex_unlock(&git_async_mutex);
}
