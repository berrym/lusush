/**
 * @file segment.c
 * @brief LLE Prompt Segment System Implementation
 *
 * Specification: Spec 25 Section 5 - Segment Architecture
 * Version: 1.0.0
 *
 * Implements segment registry, prompt context, and built-in segments.
 */

#include "lle/prompt/segment.h"

#include "lle/adaptive_terminal_integration.h"
#include "lle/utf8_support.h"

#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ========================================================================== */
/* Segment Registry Implementation                                            */
/* ========================================================================== */

lle_result_t lle_segment_registry_init(lle_segment_registry_t *registry) {
    if (!registry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(registry, 0, sizeof(*registry));
    registry->initialized = true;

    return LLE_SUCCESS;
}

void lle_segment_registry_cleanup(lle_segment_registry_t *registry) {
    if (!registry || !registry->initialized) {
        return;
    }

    for (size_t i = 0; i < registry->count; i++) {
        if (registry->segments[i]) {
            lle_segment_free(registry->segments[i]);
            registry->segments[i] = NULL;
        }
    }

    registry->count = 0;
    registry->initialized = false;
}

lle_result_t lle_segment_registry_register(lle_segment_registry_t *registry,
                                            lle_prompt_segment_t *segment) {
    if (!registry || !segment || !registry->initialized) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (registry->count >= LLE_SEGMENT_REGISTRY_MAX) {
        return LLE_ERROR_BUFFER_OVERFLOW;
    }

    /* Check for duplicate name */
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->segments[i]->name, segment->name) == 0) {
            return LLE_ERROR_INVALID_STATE;  /* Duplicate */
        }
    }

    /* Initialize segment if it has an init function */
    if (segment->init) {
        lle_result_t result = segment->init(segment);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }

    registry->segments[registry->count] = segment;
    registry->count++;

    return LLE_SUCCESS;
}

lle_prompt_segment_t *lle_segment_registry_find(
    const lle_segment_registry_t *registry,
    const char *name) {
    if (!registry || !name || !registry->initialized) {
        return NULL;
    }

    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->segments[i]->name, name) == 0) {
            return registry->segments[i];
        }
    }

    return NULL;
}

size_t lle_segment_registry_list(const lle_segment_registry_t *registry,
                                  const char **names,
                                  size_t max_names) {
    if (!registry || !registry->initialized) {
        return 0;
    }

    if (names && max_names > 0) {
        size_t count = (registry->count < max_names) ?
                       registry->count : max_names;
        for (size_t i = 0; i < count; i++) {
            names[i] = registry->segments[i]->name;
        }
    }

    return registry->count;
}

void lle_segment_registry_invalidate_all(lle_segment_registry_t *registry) {
    if (!registry || !registry->initialized) {
        return;
    }

    for (size_t i = 0; i < registry->count; i++) {
        if (registry->segments[i]->invalidate_cache) {
            registry->segments[i]->invalidate_cache(registry->segments[i]);
        }
    }
}

/* ========================================================================== */
/* Prompt Context Implementation                                              */
/* ========================================================================== */

lle_result_t lle_prompt_context_init(lle_prompt_context_t *ctx) {
    if (!ctx) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(ctx, 0, sizeof(*ctx));

    /* Get current user */
    uid_t uid = getuid();
    ctx->uid = uid;
    ctx->is_root = (uid == 0);

    struct passwd *pw = getpwuid(uid);
    if (pw) {
        snprintf(ctx->username, sizeof(ctx->username), "%s", pw->pw_name);
        snprintf(ctx->home_dir, sizeof(ctx->home_dir), "%s", pw->pw_dir);
    } else {
        snprintf(ctx->username, sizeof(ctx->username), "user");
        const char *home = getenv("HOME");
        if (home) {
            snprintf(ctx->home_dir, sizeof(ctx->home_dir), "%s", home);
        }
    }

    /* Get hostname */
    if (gethostname(ctx->hostname, sizeof(ctx->hostname) - 1) != 0) {
        snprintf(ctx->hostname, sizeof(ctx->hostname), "localhost");
    }

    /* Get current directory */
    lle_prompt_context_refresh_directory(ctx);

    /* Get current time */
    ctx->current_time = time(NULL);
    localtime_r(&ctx->current_time, &ctx->current_tm);

    /* Get shell level */
    const char *shlvl = getenv("SHLVL");
    ctx->shlvl = shlvl ? atoi(shlvl) : 1;

    /* Check for SSH session */
    ctx->is_ssh_session = (getenv("SSH_CLIENT") != NULL ||
                           getenv("SSH_TTY") != NULL);

    /* Default keymap */
    snprintf(ctx->keymap, sizeof(ctx->keymap), "emacs");

    /* Terminal capabilities - detect from adaptive terminal system */
    ctx->has_unicode = true;
    ctx->terminal_width = 80;
    ctx->terminal_height = 24;

    /* Use adaptive terminal detection for color capabilities.
     * Note: The optimized detection returns a cached result that is
     * managed by the detection system - do NOT destroy it, as that
     * could cause a double-free if the cache was refreshed. */
    lle_terminal_detection_result_t *detection = NULL;
    lle_result_t det_result = lle_detect_terminal_capabilities_optimized(&detection);
    if (det_result == LLE_SUCCESS && detection) {
        ctx->has_256_color = detection->supports_256_colors;
        ctx->has_true_color = detection->supports_truecolor;
        /* Do NOT call lle_terminal_detection_result_destroy here -
         * the optimized version manages the cached result internally */
    } else {
        /* Default to 256 colors if detection fails */
        ctx->has_256_color = true;
        ctx->has_true_color = false;
    }

    return LLE_SUCCESS;
}

void lle_prompt_context_update(lle_prompt_context_t *ctx,
                                int exit_code,
                                uint64_t duration_ms) {
    if (!ctx) {
        return;
    }

    ctx->last_exit_code = exit_code;
    ctx->last_cmd_duration_ms = duration_ms;

    /* Update time */
    ctx->current_time = time(NULL);
    localtime_r(&ctx->current_time, &ctx->current_tm);
}

void lle_prompt_context_set_job_count(lle_prompt_context_t *ctx, int job_count) {
    if (!ctx) {
        return;
    }
    ctx->background_job_count = job_count;
}

lle_result_t lle_prompt_context_refresh_directory(lle_prompt_context_t *ctx) {
    if (!ctx) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Get current directory */
    if (!getcwd(ctx->cwd, sizeof(ctx->cwd))) {
        ctx->cwd[0] = '\0';
        ctx->cwd_display[0] = '\0';
        return LLE_ERROR_SYSTEM_CALL;
    }

    /* Create display version with ~ for home */
    size_t home_len = strlen(ctx->home_dir);
    if (home_len > 0 && strncmp(ctx->cwd, ctx->home_dir, home_len) == 0) {
        if (ctx->cwd[home_len] == '\0') {
            snprintf(ctx->cwd_display, sizeof(ctx->cwd_display), "~");
            ctx->cwd_is_home = true;
        } else if (ctx->cwd[home_len] == '/') {
            snprintf(ctx->cwd_display, sizeof(ctx->cwd_display),
                     "~%s", ctx->cwd + home_len);
            ctx->cwd_is_home = false;
        } else {
            snprintf(ctx->cwd_display, sizeof(ctx->cwd_display),
                     "%s", ctx->cwd);
            ctx->cwd_is_home = false;
        }
    } else {
        snprintf(ctx->cwd_display, sizeof(ctx->cwd_display), "%s", ctx->cwd);
        ctx->cwd_is_home = strcmp(ctx->cwd, ctx->home_dir) == 0;
    }

    /* Check writability */
    ctx->cwd_is_writable = (access(ctx->cwd, W_OK) == 0);

    /* Check for git repo */
    char git_dir[PATH_MAX + 8];  /* Extra space for /.git */
    snprintf(git_dir, sizeof(git_dir), "%s/.git", ctx->cwd);
    struct stat st;
    ctx->cwd_is_git_repo = (stat(git_dir, &st) == 0);

    return LLE_SUCCESS;
}

/* ========================================================================== */
/* Segment Creation Helpers                                                   */
/* ========================================================================== */

lle_prompt_segment_t *lle_segment_create(const char *name,
                                          const char *description,
                                          uint32_t capabilities) {
    if (!name) {
        return NULL;
    }

    lle_prompt_segment_t *segment = calloc(1, sizeof(*segment));
    if (!segment) {
        return NULL;
    }

    snprintf(segment->name, sizeof(segment->name), "%s", name);
    if (description) {
        snprintf(segment->description, sizeof(segment->description),
                 "%s", description);
    }
    segment->capabilities = capabilities;

    return segment;
}

void lle_segment_free(lle_prompt_segment_t *segment) {
    if (!segment) {
        return;
    }

    /* Call cleanup if available */
    if (segment->cleanup) {
        segment->cleanup(segment);
    }

    /* Free state if set */
    free(segment->state);
    free(segment);
}

/* ========================================================================== */
/* Built-in Segment: Directory                                                */
/* ========================================================================== */

typedef struct {
    char cached_cwd[PATH_MAX];
    char cached_output[LLE_SEGMENT_OUTPUT_MAX];
    bool cache_valid;
} segment_directory_state_t;

static lle_result_t segment_directory_init(lle_prompt_segment_t *self) {
    segment_directory_state_t *state = calloc(1, sizeof(*state));
    if (!state) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    self->state = state;
    return LLE_SUCCESS;
}

static void segment_directory_cleanup(lle_prompt_segment_t *self) {
    /* State freed by lle_segment_free */
    (void)self;
}

static bool segment_directory_is_visible(const lle_prompt_segment_t *self,
                                          const lle_prompt_context_t *ctx) {
    (void)self;
    return strlen(ctx->cwd) > 0;
}

static lle_result_t segment_directory_render(const lle_prompt_segment_t *self,
                                              const lle_prompt_context_t *ctx,
                                              lle_segment_output_t *output) {
    (void)self;

    const char *display = strlen(ctx->cwd_display) > 0 ?
                          ctx->cwd_display : ctx->cwd;

    /* Copy with truncation - output buffer is smaller than PATH_MAX */
    size_t display_len = strlen(display);
    size_t copy_len = (display_len < sizeof(output->content) - 1) ?
                      display_len : sizeof(output->content) - 1;
    memcpy(output->content, display, copy_len);
    output->content[copy_len] = '\0';

    output->content_len = copy_len;
    output->visual_width = lle_utf8_string_width(output->content, copy_len);
    output->is_empty = (copy_len == 0);
    output->needs_separator = true;

    return LLE_SUCCESS;
}

static const char *segment_directory_get_property(
    const lle_prompt_segment_t *self,
    const char *property) {
    segment_directory_state_t *state = self->state;

    if (strcmp(property, "full") == 0) {
        return state->cached_cwd;
    }
    return NULL;
}

static void segment_directory_invalidate(lle_prompt_segment_t *self) {
    segment_directory_state_t *state = self->state;
    if (state) {
        state->cache_valid = false;
    }
}

lle_prompt_segment_t *lle_segment_create_directory(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "directory",
        "Current working directory",
        LLE_SEG_CAP_CACHEABLE | LLE_SEG_CAP_THEME_AWARE | LLE_SEG_CAP_PROPERTIES);

    if (!seg) return NULL;

    seg->init = segment_directory_init;
    seg->cleanup = segment_directory_cleanup;
    seg->is_visible = segment_directory_is_visible;
    seg->render = segment_directory_render;
    seg->get_property = segment_directory_get_property;
    seg->invalidate_cache = segment_directory_invalidate;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: User                                                     */
/* ========================================================================== */

static bool segment_user_is_visible(const lle_prompt_segment_t *self,
                                     const lle_prompt_context_t *ctx) {
    (void)self;
    return strlen(ctx->username) > 0;
}

static lle_result_t segment_user_render(const lle_prompt_segment_t *self,
                                         const lle_prompt_context_t *ctx,
                                         lle_segment_output_t *output) {
    (void)self;

    snprintf(output->content, sizeof(output->content), "%s", ctx->username);
    output->content_len = strlen(output->content);
    output->visual_width = output->content_len;
    output->is_empty = (output->content_len == 0);
    output->needs_separator = true;

    return LLE_SUCCESS;
}

/**
 * @brief Cache invalidation callback for user segment
 *
 * User information is static during a shell session, but this callback
 * is required for consistency with the LLE_SEG_CAP_CACHEABLE capability.
 * Called by lle_segment_registry_invalidate_all() on directory change events.
 */
static void segment_user_invalidate(lle_prompt_segment_t *self) {
    (void)self;
}

lle_prompt_segment_t *lle_segment_create_user(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "user",
        "Current username",
        LLE_SEG_CAP_CACHEABLE);

    if (!seg) return NULL;

    seg->is_visible = segment_user_is_visible;
    seg->render = segment_user_render;
    seg->invalidate_cache = segment_user_invalidate;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: Host                                                     */
/* ========================================================================== */

static bool segment_host_is_visible(const lle_prompt_segment_t *self,
                                     const lle_prompt_context_t *ctx) {
    (void)self;
    return strlen(ctx->hostname) > 0;
}

static lle_result_t segment_host_render(const lle_prompt_segment_t *self,
                                         const lle_prompt_context_t *ctx,
                                         lle_segment_output_t *output) {
    (void)self;

    snprintf(output->content, sizeof(output->content), "%s", ctx->hostname);
    output->content_len = strlen(output->content);
    output->visual_width = output->content_len;
    output->is_empty = (output->content_len == 0);
    output->needs_separator = true;

    return LLE_SUCCESS;
}

/**
 * @brief Cache invalidation callback for host segment
 *
 * Hostname is static during a shell session, but this callback
 * is required for consistency with the LLE_SEG_CAP_CACHEABLE capability.
 * Called by lle_segment_registry_invalidate_all() on directory change events.
 */
static void segment_host_invalidate(lle_prompt_segment_t *self) {
    (void)self;
}

lle_prompt_segment_t *lle_segment_create_host(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "host",
        "Hostname",
        LLE_SEG_CAP_CACHEABLE);

    if (!seg) return NULL;

    seg->is_visible = segment_host_is_visible;
    seg->render = segment_host_render;
    seg->invalidate_cache = segment_host_invalidate;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: Time                                                     */
/* ========================================================================== */

static lle_result_t segment_time_render(const lle_prompt_segment_t *self,
                                         const lle_prompt_context_t *ctx,
                                         lle_segment_output_t *output) {
    (void)self;

    strftime(output->content, sizeof(output->content),
             "%H:%M:%S", &ctx->current_tm);
    output->content_len = strlen(output->content);
    output->visual_width = output->content_len;
    output->is_empty = false;
    output->needs_separator = true;

    return LLE_SUCCESS;
}

lle_prompt_segment_t *lle_segment_create_time(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "time",
        "Current time",
        LLE_SEG_CAP_DYNAMIC);

    if (!seg) return NULL;

    seg->render = segment_time_render;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: Status                                                   */
/* ========================================================================== */

static bool segment_status_is_visible(const lle_prompt_segment_t *self,
                                       const lle_prompt_context_t *ctx) {
    (void)self;
    return ctx->last_exit_code != 0;
}

static lle_result_t segment_status_render(const lle_prompt_segment_t *self,
                                           const lle_prompt_context_t *ctx,
                                           lle_segment_output_t *output) {
    (void)self;

    if (ctx->last_exit_code != 0) {
        snprintf(output->content, sizeof(output->content),
                 "%d", ctx->last_exit_code);
    } else {
        output->content[0] = '\0';
    }

    output->content_len = strlen(output->content);
    output->visual_width = output->content_len;
    output->is_empty = (output->content_len == 0);
    output->needs_separator = true;

    return LLE_SUCCESS;
}

lle_prompt_segment_t *lle_segment_create_status(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "status",
        "Exit code of last command",
        LLE_SEG_CAP_OPTIONAL | LLE_SEG_CAP_THEME_AWARE);

    if (!seg) return NULL;

    seg->is_visible = segment_status_is_visible;
    seg->render = segment_status_render;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: Jobs                                                     */
/* ========================================================================== */

static bool segment_jobs_is_visible(const lle_prompt_segment_t *self,
                                     const lle_prompt_context_t *ctx) {
    (void)self;
    return ctx->background_job_count > 0;
}

static lle_result_t segment_jobs_render(const lle_prompt_segment_t *self,
                                         const lle_prompt_context_t *ctx,
                                         lle_segment_output_t *output) {
    (void)self;

    if (ctx->background_job_count > 0) {
        snprintf(output->content, sizeof(output->content),
                 "%d", ctx->background_job_count);
    } else {
        output->content[0] = '\0';
    }

    output->content_len = strlen(output->content);
    output->visual_width = output->content_len;
    output->is_empty = (output->content_len == 0);
    output->needs_separator = true;

    return LLE_SUCCESS;
}

lle_prompt_segment_t *lle_segment_create_jobs(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "jobs",
        "Number of background jobs",
        LLE_SEG_CAP_OPTIONAL);

    if (!seg) return NULL;

    seg->is_visible = segment_jobs_is_visible;
    seg->render = segment_jobs_render;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: Symbol                                                   */
/* ========================================================================== */

static lle_result_t segment_symbol_render(const lle_prompt_segment_t *self,
                                           const lle_prompt_context_t *ctx,
                                           lle_segment_output_t *output) {
    (void)self;

    const char *symbol = ctx->is_root ? "#" : "$";
    snprintf(output->content, sizeof(output->content), "%s", symbol);
    output->content_len = strlen(output->content);
    output->visual_width = output->content_len;
    output->is_empty = false;
    output->needs_separator = false;  /* Symbol is at end, no separator */

    return LLE_SUCCESS;
}

lle_prompt_segment_t *lle_segment_create_symbol(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "symbol",
        "Prompt symbol ($ or #)",
        LLE_SEG_CAP_THEME_AWARE);

    if (!seg) return NULL;

    seg->render = segment_symbol_render;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: Git                                                      */
/* ========================================================================== */

typedef struct {
    char branch[256];
    int staged;
    int unstaged;
    int untracked;
    int ahead;
    int behind;
    bool is_repo;
    bool cache_valid;
} segment_git_state_t;

static lle_result_t segment_git_init(lle_prompt_segment_t *self) {
    segment_git_state_t *state = calloc(1, sizeof(*state));
    if (!state) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    self->state = state;
    return LLE_SUCCESS;
}

static bool segment_git_is_visible(const lle_prompt_segment_t *self,
                                    const lle_prompt_context_t *ctx) {
    segment_git_state_t *state = self->state;
    return ctx->cwd_is_git_repo || (state && state->is_repo);
}

/**
 * @brief Run a git command and capture output
 */
static int run_git_command(const char *args, char *output, size_t output_size) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "git %s 2>/dev/null", args);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }
    
    if (output && output_size > 0) {
        if (fgets(output, output_size, fp) != NULL) {
            /* Remove trailing newline */
            size_t len = strlen(output);
            if (len > 0 && output[len - 1] == '\n') {
                output[len - 1] = '\0';
            }
        } else {
            output[0] = '\0';
        }
    }
    
    return pclose(fp);
}

/**
 * @brief Check if we're in a git repository (checks parent dirs too)
 */
static bool is_in_git_repo(void) {
    char output[16];
    int ret = run_git_command("rev-parse --is-inside-work-tree", output, sizeof(output));
    return (ret == 0 && strcmp(output, "true") == 0);
}

/**
 * @brief Fetch git status and populate state
 */
static void fetch_git_status(segment_git_state_t *state) {
    if (!state) return;
    
    /* Check if in git repo */
    state->is_repo = is_in_git_repo();
    if (!state->is_repo) {
        state->branch[0] = '\0';
        state->staged = 0;
        state->unstaged = 0;
        state->untracked = 0;
        state->ahead = 0;
        state->behind = 0;
        return;
    }
    
    /* Get branch name */
    run_git_command("symbolic-ref --short HEAD", state->branch, sizeof(state->branch));
    if (state->branch[0] == '\0') {
        /* Detached HEAD - get short commit hash */
        run_git_command("rev-parse --short HEAD", state->branch, sizeof(state->branch));
    }
    
    /* Get status counts using git status --porcelain */
    FILE *fp = popen("git status --porcelain 2>/dev/null", "r");
    if (fp) {
        char line[512];
        state->staged = 0;
        state->unstaged = 0;
        state->untracked = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            if (line[0] == '?') {
                state->untracked++;
            } else {
                if (line[0] != ' ' && line[0] != '?') {
                    state->staged++;
                }
                if (line[1] != ' ' && line[1] != '?') {
                    state->unstaged++;
                }
            }
        }
        pclose(fp);
    }
    
    /* Get ahead/behind counts */
    char ab_output[64];
    if (run_git_command("rev-list --left-right --count @{upstream}...HEAD", 
                        ab_output, sizeof(ab_output)) == 0) {
        sscanf(ab_output, "%d\t%d", &state->behind, &state->ahead);
    } else {
        state->ahead = 0;
        state->behind = 0;
    }
    
    state->cache_valid = true;
}

static lle_result_t segment_git_render(const lle_prompt_segment_t *self,
                                        const lle_prompt_context_t *ctx,
                                        lle_segment_output_t *output) {
    segment_git_state_t *state = self->state;
    
    if (!state) {
        output->is_empty = true;
        output->content[0] = '\0';
        output->content_len = 0;
        return LLE_SUCCESS;
    }
    
    /* Fetch git status if cache invalid or not in repo */
    if (!state->cache_valid) {
        fetch_git_status(state);
    }

    if (!state->is_repo) {
        output->is_empty = true;
        output->content[0] = '\0';
        output->content_len = 0;
        return LLE_SUCCESS;
    }

    /* Build git status string */
    char status_indicators[64] = {0};
    size_t pos = 0;

    if (state->staged > 0) {
        pos += snprintf(status_indicators + pos, sizeof(status_indicators) - pos,
                        "+%d", state->staged);
    }
    if (state->unstaged > 0) {
        pos += snprintf(status_indicators + pos, sizeof(status_indicators) - pos,
                        "*%d", state->unstaged);
    }
    if (state->untracked > 0) {
        pos += snprintf(status_indicators + pos, sizeof(status_indicators) - pos,
                        "?%d", state->untracked);
    }

    char ahead_behind[32] = {0};
    if (state->ahead > 0 && state->behind > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), " ↑%d↓%d",
                 state->ahead, state->behind);
    } else if (state->ahead > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), " ↑%d", state->ahead);
    } else if (state->behind > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), " ↓%d", state->behind);
    }

    if (strlen(status_indicators) > 0 || strlen(ahead_behind) > 0) {
        snprintf(output->content, sizeof(output->content),
                 "(%s%s%s%s)",
                 state->branch,
                 strlen(status_indicators) > 0 ? " " : "",
                 status_indicators,
                 ahead_behind);
    } else {
        snprintf(output->content, sizeof(output->content),
                 "(%s)", state->branch);
    }

    output->content_len = strlen(output->content);
    output->visual_width = lle_utf8_string_width(output->content, output->content_len);
    output->is_empty = false;
    output->needs_separator = true;

    (void)ctx;
    return LLE_SUCCESS;
}

static const char *segment_git_get_property(const lle_prompt_segment_t *self,
                                             const char *property) {
    segment_git_state_t *state = self->state;
    if (!state) return NULL;

    if (strcmp(property, "branch") == 0) {
        return state->branch;
    }
    return NULL;
}

static void segment_git_invalidate(lle_prompt_segment_t *self) {
    segment_git_state_t *state = self->state;
    if (state) {
        state->cache_valid = false;
    }
}

lle_prompt_segment_t *lle_segment_create_git(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "git",
        "Git repository status",
        LLE_SEG_CAP_ASYNC | LLE_SEG_CAP_CACHEABLE | LLE_SEG_CAP_EXPENSIVE |
        LLE_SEG_CAP_THEME_AWARE | LLE_SEG_CAP_OPTIONAL | LLE_SEG_CAP_PROPERTIES);

    if (!seg) return NULL;

    seg->init = segment_git_init;
    seg->is_visible = segment_git_is_visible;
    seg->render = segment_git_render;
    seg->get_property = segment_git_get_property;
    seg->invalidate_cache = segment_git_invalidate;

    return seg;
}

/* ========================================================================== */
/* Register Built-in Segments                                                 */
/* ========================================================================== */

size_t lle_segment_register_builtins(lle_segment_registry_t *registry) {
    if (!registry || !registry->initialized) {
        return 0;
    }

    size_t count = 0;

    lle_prompt_segment_t *segments[] = {
        lle_segment_create_directory(),
        lle_segment_create_user(),
        lle_segment_create_host(),
        lle_segment_create_time(),
        lle_segment_create_status(),
        lle_segment_create_jobs(),
        lle_segment_create_symbol(),
        lle_segment_create_git(),
    };

    for (size_t i = 0; i < sizeof(segments) / sizeof(segments[0]); i++) {
        if (segments[i]) {
            if (lle_segment_registry_register(registry, segments[i]) == LLE_SUCCESS) {
                count++;
            } else {
                lle_segment_free(segments[i]);
            }
        }
    }

    return count;
}
