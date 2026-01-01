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
#include "lle/async_worker.h"
#include "lle/prompt/theme.h"
#include "lle/utf8_support.h"

#include <pthread.h>
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
            return LLE_ERROR_INVALID_STATE; /* Duplicate */
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

lle_prompt_segment_t *
lle_segment_registry_find(const lle_segment_registry_t *registry,
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
                                 const char **names, size_t max_names) {
    if (!registry || !registry->initialized) {
        return 0;
    }

    if (names && max_names > 0) {
        size_t count =
            (registry->count < max_names) ? registry->count : max_names;
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
    ctx->is_ssh_session =
        (getenv("SSH_CLIENT") != NULL || getenv("SSH_TTY") != NULL);

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
    lle_result_t det_result =
        lle_detect_terminal_capabilities_optimized(&detection);
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

void lle_prompt_context_update(lle_prompt_context_t *ctx, int exit_code,
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

void lle_prompt_context_set_job_count(lle_prompt_context_t *ctx,
                                      int job_count) {
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
            snprintf(ctx->cwd_display, sizeof(ctx->cwd_display), "~%s",
                     ctx->cwd + home_len);
            ctx->cwd_is_home = false;
        } else {
            snprintf(ctx->cwd_display, sizeof(ctx->cwd_display), "%s",
                     ctx->cwd);
            ctx->cwd_is_home = false;
        }
    } else {
        snprintf(ctx->cwd_display, sizeof(ctx->cwd_display), "%s", ctx->cwd);
        ctx->cwd_is_home = strcmp(ctx->cwd, ctx->home_dir) == 0;
    }

    /* Check if at filesystem root */
    ctx->cwd_is_root = (strcmp(ctx->cwd, "/") == 0);

    /* Check writability */
    ctx->cwd_is_writable = (access(ctx->cwd, W_OK) == 0);

    /* Check for git repo */
    char git_dir[PATH_MAX + 8]; /* Extra space for /.git */
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
        snprintf(segment->description, sizeof(segment->description), "%s",
                 description);
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
                                             const lle_theme_t *theme,
                                             lle_segment_output_t *output) {
    (void)self;

    const char *display =
        strlen(ctx->cwd_display) > 0 ? ctx->cwd_display : ctx->cwd;

    /* Check if we should apply path_root color (when at filesystem root) */
    if (theme && ctx->cwd_is_root &&
        theme->colors.path_root.mode != LLE_COLOR_MODE_NONE) {
        static const char *reset = "\033[0m";
        char color_code[32];
        lle_color_to_ansi(&theme->colors.path_root, true, color_code,
                          sizeof(color_code));

        snprintf(output->content, sizeof(output->content), "%s%.470s%s", color_code,
                 display, reset);
        output->content_len = strlen(output->content);
        output->visual_width = lle_utf8_string_width(display, strlen(display));
    } else {
        /* Standard path display */
        size_t display_len = strlen(display);
        size_t copy_len = (display_len < sizeof(output->content) - 1)
                              ? display_len
                              : sizeof(output->content) - 1;
        memcpy(output->content, display, copy_len);
        output->content[copy_len] = '\0';

        output->content_len = copy_len;
        output->visual_width = lle_utf8_string_width(output->content, copy_len);
    }

    output->is_empty = (output->content_len == 0);
    output->needs_separator = true;

    return LLE_SUCCESS;
}

static const char *
segment_directory_get_property(const lle_prompt_segment_t *self,
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
    lle_prompt_segment_t *seg =
        lle_segment_create("directory", "Current working directory",
                           LLE_SEG_CAP_CACHEABLE | LLE_SEG_CAP_THEME_AWARE |
                               LLE_SEG_CAP_PROPERTIES);

    if (!seg)
        return NULL;

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
                                        const lle_theme_t *theme,
                                        lle_segment_output_t *output) {
    (void)self;
    (void)theme;

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
static void segment_user_invalidate(lle_prompt_segment_t *self) { (void)self; }

lle_prompt_segment_t *lle_segment_create_user(void) {
    lle_prompt_segment_t *seg =
        lle_segment_create("user", "Current username", LLE_SEG_CAP_CACHEABLE);

    if (!seg)
        return NULL;

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
                                        const lle_theme_t *theme,
                                        lle_segment_output_t *output) {
    (void)self;
    (void)theme;

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
static void segment_host_invalidate(lle_prompt_segment_t *self) { (void)self; }

lle_prompt_segment_t *lle_segment_create_host(void) {
    lle_prompt_segment_t *seg =
        lle_segment_create("host", "Hostname", LLE_SEG_CAP_CACHEABLE);

    if (!seg)
        return NULL;

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
                                        const lle_theme_t *theme,
                                        lle_segment_output_t *output) {
    (void)self;
    (void)theme;

    strftime(output->content, sizeof(output->content), "%H:%M:%S",
             &ctx->current_tm);
    output->content_len = strlen(output->content);
    output->visual_width = output->content_len;
    output->is_empty = false;
    output->needs_separator = true;

    return LLE_SUCCESS;
}

lle_prompt_segment_t *lle_segment_create_time(void) {
    lle_prompt_segment_t *seg =
        lle_segment_create("time", "Current time", LLE_SEG_CAP_DYNAMIC);

    if (!seg)
        return NULL;

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
                                          const lle_theme_t *theme,
                                          lle_segment_output_t *output) {
    (void)self;

    if (ctx->last_exit_code != 0) {
        /* Use error symbol from theme if available */
        const char *sym_error =
            (theme && theme->symbols.error[0]) ? theme->symbols.error : "";
        if (sym_error[0]) {
            snprintf(output->content, sizeof(output->content), "%s%d",
                     sym_error, ctx->last_exit_code);
        } else {
            snprintf(output->content, sizeof(output->content), "%d",
                     ctx->last_exit_code);
        }
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
    lle_prompt_segment_t *seg =
        lle_segment_create("status", "Exit code of last command",
                           LLE_SEG_CAP_OPTIONAL | LLE_SEG_CAP_THEME_AWARE);

    if (!seg)
        return NULL;

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
                                        const lle_theme_t *theme,
                                        lle_segment_output_t *output) {
    (void)self;

    if (ctx->background_job_count > 0) {
        /* Use jobs symbol from theme if available */
        const char *sym_jobs =
            (theme && theme->symbols.jobs[0]) ? theme->symbols.jobs : "";
        if (sym_jobs[0]) {
            snprintf(output->content, sizeof(output->content), "%s%d", sym_jobs,
                     ctx->background_job_count);
        } else {
            snprintf(output->content, sizeof(output->content), "%d",
                     ctx->background_job_count);
        }
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
        "jobs", "Number of background jobs", LLE_SEG_CAP_OPTIONAL);

    if (!seg)
        return NULL;

    seg->is_visible = segment_jobs_is_visible;
    seg->render = segment_jobs_render;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: Symbol                                                   */
/* ========================================================================== */

static lle_result_t segment_symbol_render(const lle_prompt_segment_t *self,
                                          const lle_prompt_context_t *ctx,
                                          const lle_theme_t *theme,
                                          lle_segment_output_t *output) {
    (void)self;

    /* Use theme symbols if available, fall back to defaults */
    const char *symbol;
    if (ctx->is_root) {
        symbol = (theme && theme->symbols.prompt_root[0])
                     ? theme->symbols.prompt_root
                     : "#";
    } else {
        symbol =
            (theme && theme->symbols.prompt[0]) ? theme->symbols.prompt : "$";
    }
    snprintf(output->content, sizeof(output->content), "%s", symbol);
    output->content_len = strlen(output->content);
    output->visual_width = output->content_len;
    output->is_empty = false;
    output->needs_separator = false; /* Symbol is at end, no separator */

    return LLE_SUCCESS;
}

lle_prompt_segment_t *lle_segment_create_symbol(void) {
    lle_prompt_segment_t *seg = lle_segment_create(
        "symbol", "Prompt symbol ($ or #)", LLE_SEG_CAP_THEME_AWARE);

    if (!seg)
        return NULL;

    seg->render = segment_symbol_render;

    return seg;
}

/* ========================================================================== */
/* Built-in Segment: Git                                                      */
/* ========================================================================== */

/**
 * @brief Git segment state with async support
 *
 * The git segment uses an async worker to fetch git status in the background,
 * preventing prompt rendering from blocking on slow git operations.
 *
 * Async flow:
 * 1. On render, if cache invalid, queue async request (if not pending)
 * 2. Return cached/stale data immediately for non-blocking render
 * 3. When async completes, callback updates state and invalidates prompt
 * 4. Next render uses fresh data
 */
typedef struct {
    /* Git status data */
    char branch[256];
    int staged;
    int unstaged;
    int untracked;
    int ahead;
    int behind;
    int stash_count;
    bool has_conflicts;
    bool is_repo;
    bool cache_valid;

    /* Async worker state */
    lle_async_worker_t *async_worker;
    pthread_mutex_t async_mutex;
    bool async_pending;       /**< Async request in flight */
    char async_cwd[PATH_MAX]; /**< CWD for pending request */
    bool async_initialized;   /**< Async system initialized */
} segment_git_state_t;

/* Forward declaration for async callback */
static void segment_git_async_callback(const lle_async_response_t *response,
                                       void *user_data);

static lle_result_t segment_git_init(lle_prompt_segment_t *self) {
    segment_git_state_t *state = calloc(1, sizeof(*state));
    if (!state) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize mutex for async state protection */
    if (pthread_mutex_init(&state->async_mutex, NULL) != 0) {
        free(state);
        return LLE_ERROR_SYSTEM_CALL;
    }

    /* Initialize async worker for background git status fetching */
    lle_result_t result = lle_async_worker_init(
        &state->async_worker, segment_git_async_callback, self);
    if (result == LLE_SUCCESS) {
        result = lle_async_worker_start(state->async_worker);
        if (result == LLE_SUCCESS) {
            state->async_initialized = true;
        } else {
            lle_async_worker_destroy(state->async_worker);
            state->async_worker = NULL;
        }
    }

    /* Async init failure is non-fatal - fall back to sync git */
    self->state = state;
    return LLE_SUCCESS;
}

static void segment_git_cleanup(lle_prompt_segment_t *self) {
    segment_git_state_t *state = self->state;
    if (!state) {
        return;
    }

    /* Shutdown async worker if running */
    if (state->async_initialized && state->async_worker) {
        lle_async_worker_shutdown(state->async_worker);
        lle_async_worker_wait(state->async_worker);
        lle_async_worker_destroy(state->async_worker);
        state->async_worker = NULL;
        state->async_initialized = false;
    }

    pthread_mutex_destroy(&state->async_mutex);

    /* Note: state itself is freed by lle_segment_free() */
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

    /* Drain any remaining output to prevent child from blocking */
    char drain[128];
    while (fgets(drain, sizeof(drain), fp)) {
    }

    return pclose(fp);
}

/**
 * @brief Check if we're in a git repository (checks parent dirs too)
 */
static bool is_in_git_repo(void) {
    char output[16];
    int ret = run_git_command("rev-parse --is-inside-work-tree", output,
                              sizeof(output));
    return (ret == 0 && strcmp(output, "true") == 0);
}

/**
 * @brief Fetch git status and populate state
 */
static void fetch_git_status(segment_git_state_t *state) {
    if (!state)
        return;

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
    run_git_command("symbolic-ref --short HEAD", state->branch,
                    sizeof(state->branch));
    if (state->branch[0] == '\0') {
        /* Detached HEAD - get short commit hash */
        run_git_command("rev-parse --short HEAD", state->branch,
                        sizeof(state->branch));
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

    /* Get stash count */
    FILE *stash_fp = popen("git stash list 2>/dev/null | wc -l", "r");
    if (stash_fp) {
        char stash_buf[16];
        if (fgets(stash_buf, sizeof(stash_buf), stash_fp)) {
            state->stash_count = atoi(stash_buf);
        }
        pclose(stash_fp);
    }

    /* Check for merge conflicts (unmerged files) */
    state->has_conflicts = false;
    FILE *conflict_fp = popen("git ls-files -u 2>/dev/null | head -1", "r");
    if (conflict_fp) {
        char conflict_buf[8];
        if (fgets(conflict_buf, sizeof(conflict_buf), conflict_fp) &&
            conflict_buf[0]) {
            state->has_conflicts = true;
        }
        /* Drain any remaining output to prevent child from blocking */
        while (fgets(conflict_buf, sizeof(conflict_buf), conflict_fp)) {
        }
        pclose(conflict_fp);
    }

    state->cache_valid = true;
}

/**
 * @brief Async completion callback for git status
 *
 * Called from the async worker thread when git status fetch completes.
 * Updates the segment state with fresh data thread-safely.
 */
static void segment_git_async_callback(const lle_async_response_t *response,
                                       void *user_data) {
    lle_prompt_segment_t *self = (lle_prompt_segment_t *)user_data;
    if (!self || !self->state) {
        return;
    }

    segment_git_state_t *state = self->state;

    pthread_mutex_lock(&state->async_mutex);

    if (response->result == LLE_SUCCESS) {
        const lle_git_status_data_t *git = &response->data.git_status;

        /* Update state from async response */
        state->is_repo = git->is_git_repo;

        if (git->is_git_repo) {
            snprintf(state->branch, sizeof(state->branch), "%s", git->branch);
            state->staged = git->staged_count;
            state->unstaged = git->unstaged_count;
            state->untracked = git->untracked_count;
            state->ahead = git->ahead;
            state->behind = git->behind;
            /* Note: stash_count and has_conflicts not in async response,
             * these would require extending lle_git_status_data_t */
            state->cache_valid = true;
        } else {
            state->branch[0] = '\0';
            state->staged = 0;
            state->unstaged = 0;
            state->untracked = 0;
            state->ahead = 0;
            state->behind = 0;
            state->cache_valid = true;
        }
    }

    state->async_pending = false;
    pthread_mutex_unlock(&state->async_mutex);

    /* Note: Prompt will show updated data on next render cycle */
}

/**
 * @brief Queue an async git status fetch
 *
 * Non-blocking - queues request and returns immediately.
 * If async is not available or queue fails, returns false.
 */
static bool queue_async_git_fetch(segment_git_state_t *state) {
    if (!state || !state->async_initialized || !state->async_worker) {
        return false;
    }

    if (!lle_async_worker_is_running(state->async_worker)) {
        return false;
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return false;
    }

    pthread_mutex_lock(&state->async_mutex);

    /* Don't queue if already pending for same directory */
    if (state->async_pending && strcmp(state->async_cwd, cwd) == 0) {
        pthread_mutex_unlock(&state->async_mutex);
        return true; /* Already queued */
    }

    lle_async_request_t *req = lle_async_request_create(LLE_ASYNC_GIT_STATUS);
    if (!req) {
        pthread_mutex_unlock(&state->async_mutex);
        return false;
    }

    snprintf(req->cwd, sizeof(req->cwd), "%s", cwd);

    if (lle_async_worker_submit(state->async_worker, req) == LLE_SUCCESS) {
        state->async_pending = true;
        snprintf(state->async_cwd, sizeof(state->async_cwd), "%s", cwd);
        pthread_mutex_unlock(&state->async_mutex);
        return true;
    }

    lle_async_request_free(req);
    pthread_mutex_unlock(&state->async_mutex);
    return false;
}

/**
 * @brief Helper to append colored text to buffer
 *
 * Appends text with optional ANSI color prefix and reset suffix.
 *
 * @param buf         Output buffer
 * @param buf_size    Buffer size
 * @param pos         Current position in buffer (updated)
 * @param text        Text to append
 * @param color       Color to use (NULL for no color)
 * @param reset       Reset sequence (NULL to skip reset)
 * @return Number of visible characters added (excluding ANSI codes)
 */
static size_t append_colored(char *buf, size_t buf_size, size_t *pos,
                             const char *text, const lle_color_t *color,
                             const char *reset) {
    if (*pos >= buf_size - 1)
        return 0;

    size_t remaining = buf_size - *pos - 1;
    size_t visible = 0;

    /* Add color code if provided */
    if (color && color->mode != LLE_COLOR_MODE_NONE) {
        char color_code[32];
        lle_color_to_ansi(color, true, color_code, sizeof(color_code));
        size_t code_len = strlen(color_code);
        if (code_len < remaining) {
            memcpy(buf + *pos, color_code, code_len);
            *pos += code_len;
            remaining -= code_len;
        }
    }

    /* Add text */
    size_t text_len = strlen(text);
    if (text_len < remaining) {
        memcpy(buf + *pos, text, text_len);
        *pos += text_len;
        remaining -= text_len;
        visible = lle_utf8_string_width(text, text_len);
    }

    /* Add reset if color was used */
    if (color && color->mode != LLE_COLOR_MODE_NONE && reset) {
        size_t reset_len = strlen(reset);
        if (reset_len < remaining) {
            memcpy(buf + *pos, reset, reset_len);
            *pos += reset_len;
        }
    }

    buf[*pos] = '\0';
    return visible;
}

static lle_result_t segment_git_render(const lle_prompt_segment_t *self,
                                       const lle_prompt_context_t *ctx,
                                       const lle_theme_t *theme,
                                       lle_segment_output_t *output) {
    segment_git_state_t *state = self->state;
    static const char *reset = "\033[0m";

    if (!state) {
        output->is_empty = true;
        output->content[0] = '\0';
        output->content_len = 0;
        return LLE_SUCCESS;
    }

    /* Fetch git status if cache invalid */
    if (!state->cache_valid) {
        /* Try async first for non-blocking operation */
        if (state->async_initialized) {
            /* Queue async request - will update on next render */
            queue_async_git_fetch(state);

            /* If we have no cached data at all, do one sync fetch
             * to avoid showing empty git segment on first prompt */
            if (!state->is_repo && !state->branch[0]) {
                fetch_git_status(state);
            }
            /* Otherwise, render with stale cached data while async runs */
        } else {
            /* No async available - do sync fetch */
            fetch_git_status(state);
        }
    }

    if (!state->is_repo) {
        output->is_empty = true;
        output->content[0] = '\0';
        output->content_len = 0;
        return LLE_SUCCESS;
    }

    /* Get symbols from theme or use defaults */
    const char *sym_staged =
        (theme && theme->symbols.staged[0]) ? theme->symbols.staged : "+";
    const char *sym_unstaged =
        (theme && theme->symbols.unstaged[0]) ? theme->symbols.unstaged : "*";
    const char *sym_untracked =
        (theme && theme->symbols.untracked[0]) ? theme->symbols.untracked : "?";
    const char *sym_ahead =
        (theme && theme->symbols.ahead[0]) ? theme->symbols.ahead : "↑";
    const char *sym_behind =
        (theme && theme->symbols.behind[0]) ? theme->symbols.behind : "↓";
    const char *sym_branch =
        (theme && theme->symbols.branch[0]) ? theme->symbols.branch : "";
    const char *sym_stash =
        (theme && theme->symbols.stash[0]) ? theme->symbols.stash : "≡";
    const char *sym_conflict =
        (theme && theme->symbols.conflict[0]) ? theme->symbols.conflict : "!";

    /* Get colors from theme (NULL if no theme or no specific color) */
    const lle_color_t *color_staged = theme ? &theme->colors.git_staged : NULL;
    const lle_color_t *color_unstaged = theme ? &theme->colors.git_dirty : NULL;
    const lle_color_t *color_untracked =
        theme ? &theme->colors.git_untracked : NULL;
    const lle_color_t *color_ahead = theme ? &theme->colors.git_ahead : NULL;
    const lle_color_t *color_behind = theme ? &theme->colors.git_behind : NULL;

    /* Build output with embedded colors */
    size_t pos = 0;
    size_t visual_width = 0;
    char *buf = output->content;
    size_t buf_size = sizeof(output->content);

    /* Opening paren and branch */
    buf[pos++] = '(';
    visual_width++;

    /* Add branch symbol if configured */
    if (sym_branch[0]) {
        size_t sym_len = strlen(sym_branch);
        if (pos + sym_len < buf_size) {
            memcpy(buf + pos, sym_branch, sym_len);
            pos += sym_len;
            visual_width += lle_utf8_string_width(sym_branch, sym_len);
        }
    }

    size_t branch_len = strlen(state->branch);
    if (pos + branch_len < buf_size) {
        memcpy(buf + pos, state->branch, branch_len);
        pos += branch_len;
        visual_width += lle_utf8_string_width(state->branch, branch_len);
    }

    /* Status indicators with colors */
    bool has_status =
        (state->staged > 0 || state->unstaged > 0 || state->untracked > 0);
    if (has_status) {
        buf[pos++] = ' ';
        visual_width++;
    }

    if (state->staged > 0) {
        char indicator[32];
        snprintf(indicator, sizeof(indicator), "%.15s%d", sym_staged,
                 state->staged);
        visual_width +=
            append_colored(buf, buf_size, &pos, indicator, color_staged, reset);
    }
    if (state->unstaged > 0) {
        char indicator[32];
        snprintf(indicator, sizeof(indicator), "%.15s%d", sym_unstaged,
                 state->unstaged);
        visual_width += append_colored(buf, buf_size, &pos, indicator,
                                       color_unstaged, reset);
    }
    if (state->untracked > 0) {
        char indicator[32];
        snprintf(indicator, sizeof(indicator), "%.15s%d", sym_untracked,
                 state->untracked);
        visual_width += append_colored(buf, buf_size, &pos, indicator,
                                       color_untracked, reset);
    }

    /* Ahead/behind with colors */
    if (state->ahead > 0 || state->behind > 0) {
        buf[pos++] = ' ';
        visual_width++;
    }
    if (state->ahead > 0) {
        char indicator[32];
        snprintf(indicator, sizeof(indicator), "%.15s%d", sym_ahead, state->ahead);
        visual_width +=
            append_colored(buf, buf_size, &pos, indicator, color_ahead, reset);
    }
    if (state->behind > 0) {
        char indicator[32];
        snprintf(indicator, sizeof(indicator), "%.15s%d", sym_behind,
                 state->behind);
        visual_width +=
            append_colored(buf, buf_size, &pos, indicator, color_behind, reset);
    }

    /* Stash indicator */
    if (state->stash_count > 0) {
        buf[pos++] = ' ';
        visual_width++;
        char indicator[32];
        snprintf(indicator, sizeof(indicator), "%.15s%d", sym_stash,
                 state->stash_count);
        visual_width +=
            append_colored(buf, buf_size, &pos, indicator, NULL, NULL);
    }

    /* Conflict indicator - use error color for prominence */
    if (state->has_conflicts) {
        buf[pos++] = ' ';
        visual_width++;
        const lle_color_t *color_conflict = theme ? &theme->colors.error : NULL;
        visual_width += append_colored(buf, buf_size, &pos, sym_conflict,
                                       color_conflict, reset);
    }

    /* Closing paren */
    if (pos < buf_size - 1) {
        buf[pos++] = ')';
        visual_width++;
    }
    buf[pos] = '\0';

    output->content_len = pos;
    output->visual_width = visual_width;
    output->is_empty = false;
    output->needs_separator = true;

    (void)ctx;
    return LLE_SUCCESS;
}

static const char *segment_git_get_property(const lle_prompt_segment_t *self,
                                            const char *property) {
    segment_git_state_t *state = self->state;
    if (!state)
        return NULL;

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
    lle_prompt_segment_t *seg =
        lle_segment_create("git", "Git repository status",
                           LLE_SEG_CAP_ASYNC | LLE_SEG_CAP_CACHEABLE |
                               LLE_SEG_CAP_EXPENSIVE | LLE_SEG_CAP_THEME_AWARE |
                               LLE_SEG_CAP_OPTIONAL | LLE_SEG_CAP_PROPERTIES);

    if (!seg)
        return NULL;

    seg->init = segment_git_init;
    seg->cleanup = segment_git_cleanup;
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
        lle_segment_create_directory(), lle_segment_create_user(),
        lle_segment_create_host(),      lle_segment_create_time(),
        lle_segment_create_status(),    lle_segment_create_jobs(),
        lle_segment_create_symbol(),    lle_segment_create_git(),
    };

    for (size_t i = 0; i < sizeof(segments) / sizeof(segments[0]); i++) {
        if (segments[i]) {
            if (lle_segment_registry_register(registry, segments[i]) ==
                LLE_SUCCESS) {
                count++;
            } else {
                lle_segment_free(segments[i]);
            }
        }
    }

    return count;
}
