/**
 * @file composer.c
 * @brief LLE Prompt Composer Implementation
 *
 * Specification: Spec 25 - Prompt Composer
 * Version: 1.0.0
 *
 * Integrates the template engine, segment registry, and theme registry
 * to render complete prompts with dynamic segment content and theme colors.
 */

#include "lle/prompt/composer.h"
#include "lle/adaptive_terminal_integration.h"
#include "lle/display_integration.h"
#include "lle/lle_shell_event_hub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * Terminal Capability Detection Cache
 * ============================================================================
 */

/**
 * @brief Cached terminal color capabilities
 *
 * Detected once at first use and cached for performance.
 */
static struct {
    bool initialized;
    bool has_256_color;
    bool has_true_color;
} g_terminal_color_caps = {false, false, false};

/**
 * @brief Get cached terminal color capabilities
 */
static void get_terminal_color_capabilities(bool *has_256, bool *has_true) {
    if (!g_terminal_color_caps.initialized) {
        /* Detect terminal capabilities.
         * Note: The optimized detection returns a cached result that is
         * managed by the detection system - do NOT destroy it. */
        lle_terminal_detection_result_t *detection = NULL;
        lle_result_t result = lle_detect_terminal_capabilities_optimized(&detection);

        if (result == LLE_SUCCESS && detection) {
            g_terminal_color_caps.has_256_color = detection->supports_256_colors;
            g_terminal_color_caps.has_true_color = detection->supports_truecolor;
        } else {
            /* Default to 256 colors if detection fails */
            g_terminal_color_caps.has_256_color = true;
            g_terminal_color_caps.has_true_color = false;
        }
        g_terminal_color_caps.initialized = true;
    }

    if (has_256) *has_256 = g_terminal_color_caps.has_256_color;
    if (has_true) *has_true = g_terminal_color_caps.has_true_color;
}

/* ============================================================================
 * Internal Helper Types
 * ============================================================================
 */

/**
 * @brief Context passed to template callbacks
 */
typedef struct composer_callback_ctx {
    lle_prompt_composer_t *composer;
    const lle_theme_t *theme;
} composer_callback_ctx_t;

/**
 * @brief Get the semantic color for a segment from theme
 *
 * Maps segment names to their default semantic colors in the theme.
 * This provides automatic coloring for built-in themes while still
 * allowing users to override with explicit ${color:...} syntax.
 *
 * @param theme         Active theme (may be NULL)
 * @param segment_name  Name of the segment
 * @return Pointer to the color, or NULL if no mapping exists
 */
static const lle_color_t *get_segment_color(const lle_theme_t *theme,
                                             const char *segment_name) {
    if (!theme || !segment_name) {
        return NULL;
    }

    /* Map segment names to semantic theme colors */
    if (strcmp(segment_name, "user") == 0) {
        return &theme->colors.primary;
    } else if (strcmp(segment_name, "host") == 0) {
        return &theme->colors.secondary;
    } else if (strcmp(segment_name, "directory") == 0) {
        return &theme->colors.path_normal;
    } else if (strcmp(segment_name, "git") == 0) {
        return &theme->colors.git_branch;
    } else if (strcmp(segment_name, "status") == 0) {
        return &theme->colors.status_error;
    } else if (strcmp(segment_name, "jobs") == 0) {
        return &theme->colors.warning;
    } else if (strcmp(segment_name, "time") == 0) {
        return &theme->colors.text_dim;
    } else if (strcmp(segment_name, "symbol") == 0) {
        return &theme->colors.primary;
    }

    return NULL;
}

/* ============================================================================
 * Template Engine Callbacks
 * ============================================================================
 */

/**
 * @brief Get segment content for template rendering
 *
 * Renders the segment and automatically wraps with theme color if available.
 * This provides colored prompts by default for built-in themes, while users
 * can override by using explicit ${color:...} syntax in custom templates.
 *
 * @param segment_name  Name of segment to render
 * @param property      Property name (NULL for full segment)
 * @param user_data     Composer callback context
 * @return Rendered content (caller must free) or NULL
 */
static char *composer_get_segment(const char *segment_name,
                                   const char *property,
                                   void *user_data) {
    composer_callback_ctx_t *ctx = user_data;
    if (!ctx || !ctx->composer || !segment_name) {
        return NULL;
    }

    lle_prompt_composer_t *composer = ctx->composer;

    /* Find segment in registry */
    lle_prompt_segment_t *segment = lle_segment_registry_find(
        composer->segments, segment_name);
    if (!segment) {
        return NULL;
    }

    /* Handle property access */
    if (property && segment->get_property) {
        const char *value = segment->get_property(segment, property);
        if (value) {
            return strdup(value);
        }
        return NULL;
    }

    /* Render full segment */
    lle_segment_output_t output;
    memset(&output, 0, sizeof(output));

    if (segment->render) {
        lle_result_t result = segment->render(segment,
                                               &composer->context,
                                               &output);
        if (result != LLE_SUCCESS || output.is_empty) {
            return NULL;
        }

        /* Check if theme provides a color for this segment */
        const lle_color_t *color = get_segment_color(ctx->theme, segment_name);
        if (color && color->mode != LLE_COLOR_MODE_NONE) {
            /* Get terminal color capabilities for adaptive color support */
            bool has_256_color = false;
            bool has_true_color = false;
            get_terminal_color_capabilities(&has_256_color, &has_true_color);

            /* Downgrade color to match terminal capabilities */
            lle_color_t adapted_color = lle_color_downgrade(color,
                                                             has_true_color,
                                                             has_256_color);

            /* Wrap content with color escape sequences */
            char color_start[LLE_COLOR_CODE_MAX];
            static const char *color_reset = "\033[0m";

            lle_color_to_ansi(&adapted_color, true, color_start,
                              sizeof(color_start));

            /* Allocate buffer for colored output */
            size_t total_len = strlen(color_start) + output.content_len +
                               strlen(color_reset) + 1;
            char *colored = malloc(total_len);
            if (colored) {
                snprintf(colored, total_len, "%s%s%s",
                         color_start, output.content, color_reset);
                return colored;
            }
        }

        return strdup(output.content);
    }

    return NULL;
}

/**
 * @brief Check if segment is visible
 *
 * @param segment_name  Name of segment to check
 * @param property      Property name (NULL for segment visibility)
 * @param user_data     Composer callback context
 * @return true if visible, false otherwise
 */
static bool composer_is_visible(const char *segment_name,
                                 const char *property,
                                 void *user_data) {
    composer_callback_ctx_t *ctx = user_data;
    if (!ctx || !ctx->composer || !segment_name) {
        return false;
    }

    lle_prompt_composer_t *composer = ctx->composer;

    /* Find segment in registry */
    lle_prompt_segment_t *segment = lle_segment_registry_find(
        composer->segments, segment_name);
    if (!segment) {
        return false;
    }

    /* Check if segment is enabled */
    if (segment->is_enabled && !segment->is_enabled(segment)) {
        return false;
    }

    /* Check if segment is visible in current context */
    if (segment->is_visible) {
        if (!segment->is_visible(segment, &composer->context)) {
            return false;
        }
    }

    /* For property check, verify property exists */
    if (property && segment->get_property) {
        const char *value = segment->get_property(segment, property);
        return (value != NULL && strlen(value) > 0);
    }

    return true;
}

/**
 * @brief Get ANSI color code for semantic color name
 *
 * @param color_name  Semantic color name (e.g., "primary", "git_clean")
 * @param user_data   Composer callback context
 * @return ANSI color code string (static buffer) or empty string
 */
static const char *composer_get_color(const char *color_name,
                                       void *user_data) {
    composer_callback_ctx_t *ctx = user_data;
    if (!ctx || !ctx->theme || !color_name) {
        return "";
    }

    const lle_theme_t *theme = ctx->theme;
    const lle_color_t *color = NULL;
    static char ansi_buf[LLE_COLOR_CODE_MAX];

    /* Map semantic color names to theme colors */
    if (strcmp(color_name, "primary") == 0) {
        color = &theme->colors.primary;
    } else if (strcmp(color_name, "secondary") == 0) {
        color = &theme->colors.secondary;
    } else if (strcmp(color_name, "success") == 0) {
        color = &theme->colors.success;
    } else if (strcmp(color_name, "warning") == 0) {
        color = &theme->colors.warning;
    } else if (strcmp(color_name, "error") == 0) {
        color = &theme->colors.error;
    } else if (strcmp(color_name, "info") == 0) {
        color = &theme->colors.info;
    } else if (strcmp(color_name, "text") == 0) {
        color = &theme->colors.text;
    } else if (strcmp(color_name, "text_dim") == 0) {
        color = &theme->colors.text_dim;
    } else if (strcmp(color_name, "git_clean") == 0) {
        color = &theme->colors.git_clean;
    } else if (strcmp(color_name, "git_dirty") == 0) {
        color = &theme->colors.git_dirty;
    } else if (strcmp(color_name, "git_staged") == 0) {
        color = &theme->colors.git_staged;
    } else if (strcmp(color_name, "git_branch") == 0) {
        color = &theme->colors.git_branch;
    } else if (strcmp(color_name, "path_home") == 0) {
        color = &theme->colors.path_home;
    } else if (strcmp(color_name, "path_normal") == 0) {
        color = &theme->colors.path_normal;
    } else if (strcmp(color_name, "status_ok") == 0) {
        color = &theme->colors.status_ok;
    } else if (strcmp(color_name, "status_error") == 0) {
        color = &theme->colors.status_error;
    }

    if (!color || color->mode == LLE_COLOR_MODE_NONE) {
        return "";
    }

    /* Get terminal color capabilities for adaptive color support */
    bool has_256_color = false;
    bool has_true_color = false;
    get_terminal_color_capabilities(&has_256_color, &has_true_color);

    /* Downgrade color to match terminal capabilities */
    lle_color_t adapted_color = lle_color_downgrade(color,
                                                     has_true_color,
                                                     has_256_color);

    /* Convert color to ANSI escape sequence (foreground) */
    lle_color_to_ansi(&adapted_color, true, ansi_buf, sizeof(ansi_buf));
    return ansi_buf;
}

/* ============================================================================
 * Composer Lifecycle
 * ============================================================================
 */

lle_result_t lle_composer_init(lle_prompt_composer_t *composer,
                                lle_segment_registry_t *segments,
                                lle_theme_registry_t *themes) {
    if (!composer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(composer, 0, sizeof(*composer));

    composer->segments = segments;
    composer->themes = themes;

    /* Initialize prompt context */
    if (segments) {
        lle_prompt_context_init(&composer->context);
    }

    /* Default configuration */
    composer->config.enable_right_prompt = false;
    composer->config.enable_transient = true; /* Transient prompts enabled by default */
    composer->config.respect_user_ps1 = false;
    composer->config.use_external_prompt = false;

    /* Initialize transient prompt state (Spec 25 Section 12) */
    lle_transient_init(&composer->transient);

    composer->initialized = true;

    return LLE_SUCCESS;
}

void lle_composer_cleanup(lle_prompt_composer_t *composer) {
    if (!composer) {
        return;
    }

    /* Free cached templates */
    if (composer->cached_left_template) {
        lle_template_free(composer->cached_left_template);
        composer->cached_left_template = NULL;
    }
    if (composer->cached_right_template) {
        lle_template_free(composer->cached_right_template);
        composer->cached_right_template = NULL;
    }
    if (composer->cached_ps2_template) {
        lle_template_free(composer->cached_ps2_template);
        composer->cached_ps2_template = NULL;
    }

    composer->initialized = false;
}

lle_result_t lle_composer_configure(lle_prompt_composer_t *composer,
                                     const lle_composer_config_t *config) {
    if (!composer || !config) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    composer->config = *config;
    return LLE_SUCCESS;
}

/* ============================================================================
 * Prompt Rendering
 * ============================================================================
 */

lle_template_render_ctx_t lle_composer_create_render_ctx(
    lle_prompt_composer_t *composer) {
    static composer_callback_ctx_t callback_ctx;
    lle_template_render_ctx_t render_ctx;

    memset(&render_ctx, 0, sizeof(render_ctx));

    if (composer && composer->themes) {
        callback_ctx.composer = composer;
        callback_ctx.theme = lle_theme_registry_get_active(composer->themes);

        render_ctx.get_segment = composer_get_segment;
        render_ctx.is_visible = composer_is_visible;
        render_ctx.get_color = composer_get_color;
        render_ctx.user_data = &callback_ctx;
    }

    return render_ctx;
}

/**
 * @brief Calculate visual width of rendered string (excluding ANSI codes)
 *
 * @param str  String to measure
 * @return Visual width in columns
 */
static size_t calculate_visual_width(const char *str) {
    if (!str) {
        return 0;
    }

    size_t width = 0;
    const char *p = str;
    bool in_escape = false;

    while (*p) {
        if (*p == '\033') {
            in_escape = true;
            p++;
            continue;
        }
        if (in_escape) {
            if (*p == 'm') {
                in_escape = false;
            }
            p++;
            continue;
        }

        /* Count UTF-8 character width */
        unsigned char c = (unsigned char)*p;
        if ((c & 0x80) == 0) {
            /* ASCII */
            width++;
            p++;
        } else if ((c & 0xE0) == 0xC0) {
            /* 2-byte UTF-8 */
            width++;
            p += 2;
        } else if ((c & 0xF0) == 0xE0) {
            /* 3-byte UTF-8 (CJK characters are typically double-width) */
            width += 2;
            p += 3;
        } else if ((c & 0xF8) == 0xF0) {
            /* 4-byte UTF-8 */
            width += 2;
            p += 4;
        } else {
            /* Invalid or continuation byte, skip */
            p++;
        }
    }

    return width;
}

lle_result_t lle_composer_render(lle_prompt_composer_t *composer,
                                  lle_prompt_output_t *output) {
    if (!composer || !output || !composer->initialized) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(output, 0, sizeof(*output));

    /* Get active theme */
    lle_theme_t *theme = NULL;
    if (composer->themes) {
        theme = lle_theme_registry_get_active(composer->themes);
    }

    /* Use default template if no theme */
    const char *left_format = "${directory} ${symbol} ";
    const char *ps2_format = "> ";
    const char *right_format = "";

    if (theme) {
        if (strlen(theme->layout.ps1_format) > 0) {
            left_format = theme->layout.ps1_format;
        }
        if (strlen(theme->layout.ps2_format) > 0) {
            ps2_format = theme->layout.ps2_format;
        }
        if (theme->layout.enable_right_prompt &&
            strlen(theme->layout.rps1_format) > 0) {
            right_format = theme->layout.rps1_format;
        }
    }

    /* Create render context */
    lle_template_render_ctx_t render_ctx = lle_composer_create_render_ctx(composer);

    /* Render PS1 */
    lle_result_t result;
    
    /* Prepend newline for visual separation if enabled */
    if (composer->config.newline_before_prompt) {
        output->ps1[0] = '\n';
        result = lle_template_evaluate(left_format,
                                       &render_ctx,
                                       output->ps1 + 1,
                                       sizeof(output->ps1) - 1);
        if (result != LLE_SUCCESS) {
            snprintf(output->ps1, sizeof(output->ps1), "\n$ ");
        }
    } else {
        result = lle_template_evaluate(left_format,
                                       &render_ctx,
                                       output->ps1,
                                       sizeof(output->ps1));
        if (result != LLE_SUCCESS) {
            snprintf(output->ps1, sizeof(output->ps1), "$ ");
        }
    }
    output->ps1_len = strlen(output->ps1);
    output->ps1_visual_width = calculate_visual_width(output->ps1);

    /* Check for multiline */
    output->is_multiline = (strchr(output->ps1, '\n') != NULL);

    /* Render PS2 */
    result = lle_template_evaluate(ps2_format,
                                    &render_ctx,
                                    output->ps2,
                                    sizeof(output->ps2));
    if (result != LLE_SUCCESS) {
        snprintf(output->ps2, sizeof(output->ps2), "> ");
    }
    output->ps2_len = strlen(output->ps2);
    output->ps2_visual_width = calculate_visual_width(output->ps2);

    /* Render RPROMPT if enabled */
    if (composer->config.enable_right_prompt && strlen(right_format) > 0) {
        result = lle_template_evaluate(right_format,
                                        &render_ctx,
                                        output->rprompt,
                                        sizeof(output->rprompt));
        if (result == LLE_SUCCESS && strlen(output->rprompt) > 0) {
            output->has_rprompt = true;
            output->rprompt_len = strlen(output->rprompt);
            output->rprompt_visual_width = calculate_visual_width(output->rprompt);
        }
    }

    composer->total_renders++;

    return LLE_SUCCESS;
}

lle_result_t lle_composer_render_template(lle_prompt_composer_t *composer,
                                           const char *template_str,
                                           char *output,
                                           size_t output_size) {
    if (!composer || !template_str || !output || output_size == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!composer->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }

    lle_template_render_ctx_t render_ctx = lle_composer_create_render_ctx(composer);

    return lle_template_evaluate(template_str, &render_ctx, output, output_size);
}

lle_result_t lle_composer_update_context(lle_prompt_composer_t *composer,
                                          int exit_code,
                                          uint64_t duration_ms) {
    if (!composer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_prompt_context_update(&composer->context, exit_code, duration_ms);
    return LLE_SUCCESS;
}

lle_result_t lle_composer_refresh_directory(lle_prompt_composer_t *composer) {
    if (!composer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_result_t result = lle_prompt_context_refresh_directory(&composer->context);

    /* Invalidate caches on directory change */
    if (result == LLE_SUCCESS && composer->segments) {
        lle_segment_registry_invalidate_all(composer->segments);
    }

    return result;
}

void lle_composer_invalidate_caches(lle_prompt_composer_t *composer) {
    if (!composer || !composer->segments) {
        return;
    }

    lle_segment_registry_invalidate_all(composer->segments);
}

/* ============================================================================
 * Theme Integration
 * ============================================================================
 */

lle_result_t lle_composer_set_theme(lle_prompt_composer_t *composer,
                                     const char *theme_name) {
    if (!composer || !theme_name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!composer->themes) {
        return LLE_ERROR_NOT_INITIALIZED;
    }

    lle_result_t result = lle_theme_registry_set_active(composer->themes,
                                                         theme_name);
    if (result != LLE_SUCCESS) {
        return result;
    }

    /* Clear cached templates since theme changed */
    if (composer->cached_left_template) {
        lle_template_free(composer->cached_left_template);
        composer->cached_left_template = NULL;
    }
    if (composer->cached_right_template) {
        lle_template_free(composer->cached_right_template);
        composer->cached_right_template = NULL;
    }
    if (composer->cached_ps2_template) {
        lle_template_free(composer->cached_ps2_template);
        composer->cached_ps2_template = NULL;
    }

    /* Apply syntax colors from the new LLE theme */
    lle_display_integration_t *integration =
        lle_display_integration_get_global();
    if (integration && integration->display_bridge &&
        integration->display_bridge->command_layer) {
        const lle_theme_t *active_theme =
            lle_theme_registry_get_active(composer->themes);
        if (active_theme && active_theme->has_syntax_colors) {
            lle_apply_theme_syntax_colors(
                active_theme, integration->display_bridge->command_layer);
        }
    }

    return LLE_SUCCESS;
}

const lle_theme_t *lle_composer_get_theme(const lle_prompt_composer_t *composer) {
    if (!composer || !composer->themes) {
        return NULL;
    }

    return lle_theme_registry_get_active(composer->themes);
}

/* ============================================================================
 * Shell Event Integration (Spec 26)
 * 
 * This section implements the Issue #16 fix: when the directory changes,
 * we invalidate all segment caches so stale git info doesn't persist.
 * ============================================================================
 */

/**
 * @brief Directory changed event handler (Issue #16 fix)
 *
 * When the working directory changes (cd, pushd, popd), this handler
 * invalidates all segment caches. This ensures git status, directory
 * info, and other path-dependent data is refreshed for the new location.
 *
 * @param event_data Pointer to lle_directory_changed_event_t
 * @param user_data  Pointer to lle_prompt_composer_t
 */
static void composer_on_directory_changed(void *event_data, void *user_data) {
    lle_prompt_composer_t *composer = (lle_prompt_composer_t *)user_data;
    (void)event_data; /* old_dir/new_dir available if needed */

    if (!composer || !composer->initialized) {
        return;
    }

    /* Refresh the context's directory info (cwd, cwd_display, cwd_is_git_repo).
     * This also invalidates all segment caches. */
    lle_composer_refresh_directory(composer);

    /* Invalidate all segment caches - git status, directory display, etc. */
    if (composer->segments) {
        lle_segment_registry_invalidate_all(composer->segments);
    }

    /* Mark prompt for regeneration on next render */
    composer->needs_regeneration = true;
    composer->event_triggered_refreshes++;
}

/**
 * @brief Handler for LLE_SHELL_EVENT_PRE_COMMAND
 *
 * Called just before command execution. Records command info for
 * transient prompt support.
 *
 * @param event_data Pointer to lle_pre_command_event_t
 * @param user_data  Pointer to lle_prompt_composer_t
 */
static void composer_on_pre_command(void *event_data, void *user_data) {
    lle_prompt_composer_t *composer = (lle_prompt_composer_t *)user_data;
    lle_pre_command_event_t *event = (lle_pre_command_event_t *)event_data;

    if (!composer || !composer->initialized || !event) {
        return;
    }

    /* Save command info for post-command handling */
    composer->current_command = event->command;
    composer->current_command_is_bg = event->is_background;

    /*
     * Note: Transient prompt application (Spec 25 Section 12) is handled by
     * the LINE_ACCEPTED widget hook in lle_readline.c, NOT here.
     *
     * The LINE_ACCEPTED hook fires earlier in the pipeline (before
     * dc_finalize_input writes the newline), when cursor position and
     * screen buffer state are still valid for relative cursor movement.
     *
     * By the time PRE_COMMAND fires here, the cursor has already moved
     * to the output area and screen state has been reset.
     */
}

/**
 * @brief Handler for LLE_SHELL_EVENT_POST_COMMAND
 *
 * Called after command completes. Updates context with exit code and
 * duration, then marks prompt for regeneration.
 *
 * @param event_data Pointer to lle_post_command_event_t
 * @param user_data  Pointer to lle_prompt_composer_t
 */
static void composer_on_post_command(void *event_data, void *user_data) {
    lle_prompt_composer_t *composer = (lle_prompt_composer_t *)user_data;
    lle_post_command_event_t *event = (lle_post_command_event_t *)event_data;

    if (!composer || !composer->initialized || !event) {
        return;
    }

    /* Update context with command results */
    uint64_t duration_ms = event->duration_us / 1000;
    lle_prompt_context_update(&composer->context, event->exit_code, duration_ms);

    /* Clear current command state */
    composer->current_command = NULL;
    composer->current_command_is_bg = false;

    /* Invalidate all segment caches - commands like git push/pull/commit
     * change repository state, so we must refetch git status */
    if (composer->segments) {
        lle_segment_registry_invalidate_all(composer->segments);
    }

    /* Mark prompt for regeneration - exit code/duration affects display */
    composer->needs_regeneration = true;
    composer->event_triggered_refreshes++;
}

lle_result_t lle_composer_register_shell_events(
    lle_prompt_composer_t *composer,
    struct lle_shell_event_hub *event_hub) {
    if (!composer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!event_hub) {
        /* No event hub provided - events won't be wired up.
         * This is acceptable for unit testing or minimal configurations. */
        return LLE_SUCCESS;
    }

    if (!composer->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }

    if (composer->events_registered) {
        /* Already registered */
        return LLE_SUCCESS;
    }

    lle_result_t result;

    /* Register directory change handler (Issue #16 fix) */
    result = lle_shell_event_hub_register(
        event_hub,
        LLE_SHELL_EVENT_DIRECTORY_CHANGED,
        composer_on_directory_changed,
        composer,
        "prompt_composer_dir");

    if (result != LLE_SUCCESS) {
        return result;
    }

    /* Register pre-command handler (transient prompt support) */
    result = lle_shell_event_hub_register(
        event_hub,
        LLE_SHELL_EVENT_PRE_COMMAND,
        composer_on_pre_command,
        composer,
        "prompt_composer_pre");

    if (result != LLE_SUCCESS) {
        /* Rollback directory handler */
        lle_shell_event_hub_unregister(event_hub,
                                        LLE_SHELL_EVENT_DIRECTORY_CHANGED,
                                        "prompt_composer_dir");
        return result;
    }

    /* Register post-command handler (exit code, duration) */
    result = lle_shell_event_hub_register(
        event_hub,
        LLE_SHELL_EVENT_POST_COMMAND,
        composer_on_post_command,
        composer,
        "prompt_composer_post");

    if (result != LLE_SUCCESS) {
        /* Rollback previous handlers */
        lle_shell_event_hub_unregister(event_hub,
                                        LLE_SHELL_EVENT_DIRECTORY_CHANGED,
                                        "prompt_composer_dir");
        lle_shell_event_hub_unregister(event_hub,
                                        LLE_SHELL_EVENT_PRE_COMMAND,
                                        "prompt_composer_pre");
        return result;
    }

    /* Store reference and mark as registered */
    composer->shell_event_hub = event_hub;
    composer->events_registered = true;

    return LLE_SUCCESS;
}

lle_result_t lle_composer_unregister_shell_events(
    lle_prompt_composer_t *composer) {
    if (!composer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!composer->events_registered || !composer->shell_event_hub) {
        /* Nothing to unregister */
        return LLE_SUCCESS;
    }

    lle_shell_event_hub_t *hub = composer->shell_event_hub;

    /* Unregister all handlers */
    lle_shell_event_hub_unregister(hub,
                                    LLE_SHELL_EVENT_DIRECTORY_CHANGED,
                                    "prompt_composer_dir");
    lle_shell_event_hub_unregister(hub,
                                    LLE_SHELL_EVENT_PRE_COMMAND,
                                    "prompt_composer_pre");
    lle_shell_event_hub_unregister(hub,
                                    LLE_SHELL_EVENT_POST_COMMAND,
                                    "prompt_composer_post");

    composer->shell_event_hub = NULL;
    composer->events_registered = false;

    return LLE_SUCCESS;
}

void lle_composer_clear_regeneration_flag(lle_prompt_composer_t *composer) {
    if (!composer) {
        return;
    }

    composer->needs_regeneration = false;
}
