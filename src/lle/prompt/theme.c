/**
 * @file theme.c
 * @brief LLE Theme Registry System Implementation
 *
 * Specification: Spec 25 Section 4 - Theme Registry System
 * Version: 1.0.0
 *
 * Implements theme registry, theme creation, inheritance resolution,
 * and built-in themes.
 */

#include "lle/prompt/theme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Theme Registry Implementation
 * ============================================================================
 */

/**
 * @brief Initialize the theme registry
 */
lle_result_t lle_theme_registry_init(lle_theme_registry_t *registry) {
    if (!registry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(registry, 0, sizeof(*registry));
    snprintf(registry->default_theme_name, sizeof(registry->default_theme_name),
             "minimal");
    registry->initialized = true;

    return LLE_SUCCESS;
}

/**
 * @brief Cleanup the theme registry
 */
void lle_theme_registry_cleanup(lle_theme_registry_t *registry) {
    if (!registry || !registry->initialized) {
        return;
    }

    for (size_t i = 0; i < registry->count; i++) {
        if (registry->themes[i]) {
            lle_theme_free(registry->themes[i]);
            registry->themes[i] = NULL;
        }
    }

    registry->count = 0;
    registry->active_theme = NULL;
    registry->initialized = false;
}

/**
 * @brief Register a theme with the registry
 */
lle_result_t lle_theme_registry_register(lle_theme_registry_t *registry,
                                          lle_theme_t *theme) {
    if (!registry || !theme || !registry->initialized) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (strlen(theme->name) == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (registry->count >= LLE_THEME_REGISTRY_MAX) {
        return LLE_ERROR_BUFFER_OVERFLOW;
    }

    /* Check for duplicate name */
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->themes[i]->name, theme->name) == 0) {
            return LLE_ERROR_INVALID_STATE;  /* Duplicate */
        }
    }

    /* Resolve inheritance if specified */
    if (strlen(theme->inherits_from) > 0) {
        lle_result_t result = lle_theme_resolve_inheritance(registry, theme);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }

    /* Register theme */
    registry->themes[registry->count] = theme;
    registry->count++;

    /* Update statistics */
    if (theme->source == LLE_THEME_SOURCE_BUILTIN) {
        registry->builtin_count++;
    } else {
        registry->user_count++;
    }

    return LLE_SUCCESS;
}

/**
 * @brief Find a theme by name
 */
lle_theme_t *lle_theme_registry_find(const lle_theme_registry_t *registry,
                                      const char *name) {
    if (!registry || !name || !registry->initialized) {
        return NULL;
    }

    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->themes[i]->name, name) == 0) {
            return registry->themes[i];
        }
    }

    return NULL;
}

/**
 * @brief Set the active theme
 */
lle_result_t lle_theme_registry_set_active(lle_theme_registry_t *registry,
                                            const char *name) {
    if (!registry || !name || !registry->initialized) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_theme_t *theme = lle_theme_registry_find(registry, name);
    if (!theme) {
        return LLE_ERROR_NOT_FOUND;
    }

    /* Deactivate current theme */
    if (registry->active_theme) {
        registry->active_theme->is_active = false;
    }

    /* Activate new theme */
    registry->active_theme = theme;
    theme->is_active = true;
    snprintf(registry->active_theme_name, sizeof(registry->active_theme_name),
             "%s", name);
    registry->total_switches++;

    return LLE_SUCCESS;
}

/**
 * @brief Get the currently active theme
 */
lle_theme_t *lle_theme_registry_get_active(const lle_theme_registry_t *registry) {
    if (!registry || !registry->initialized) {
        return NULL;
    }
    return registry->active_theme;
}

/**
 * @brief Get all registered theme names
 */
size_t lle_theme_registry_list(const lle_theme_registry_t *registry,
                                const char **names,
                                size_t max_names) {
    if (!registry || !registry->initialized) {
        return 0;
    }

    if (names && max_names > 0) {
        size_t count = (registry->count < max_names) ?
                       registry->count : max_names;
        for (size_t i = 0; i < count; i++) {
            names[i] = registry->themes[i]->name;
        }
    }

    return registry->count;
}

/* ============================================================================
 * Theme Creation and Management
 * ============================================================================
 */

/**
 * @brief Create a new theme
 */
lle_theme_t *lle_theme_create(const char *name,
                               const char *description,
                               lle_theme_category_t category) {
    if (!name || strlen(name) == 0) {
        return NULL;
    }

    lle_theme_t *theme = calloc(1, sizeof(*theme));
    if (!theme) {
        return NULL;
    }

    snprintf(theme->name, sizeof(theme->name), "%s", name);
    if (description) {
        snprintf(theme->description, sizeof(theme->description), "%s", description);
    }
    theme->category = category;
    theme->source = LLE_THEME_SOURCE_RUNTIME;

    /* Initialize with Unicode symbols by default */
    lle_symbol_set_init_unicode(&theme->symbols);

    return theme;
}

/**
 * @brief Free a theme
 */
void lle_theme_free(lle_theme_t *theme) {
    free(theme);
}

/**
 * @brief Resolve theme inheritance
 */
lle_result_t lle_theme_resolve_inheritance(lle_theme_registry_t *registry,
                                            lle_theme_t *theme) {
    if (!registry || !theme) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (strlen(theme->inherits_from) == 0) {
        return LLE_SUCCESS;  /* No inheritance */
    }

    /* Find parent theme */
    lle_theme_t *parent = lle_theme_registry_find(registry, theme->inherits_from);
    if (!parent) {
        return LLE_ERROR_NOT_FOUND;
    }

    /* Check for inheritance cycle (max depth 10) */
    lle_theme_t *ancestor = parent;
    for (int depth = 0; depth < 10 && ancestor; depth++) {
        if (strcmp(ancestor->name, theme->name) == 0) {
            return LLE_ERROR_INVALID_STATE;  /* Cycle detected */
        }
        ancestor = ancestor->parent;
    }

    /* Link parent */
    theme->parent = parent;

    /* Inherit colors (only if child's is unset) */
    lle_color_scheme_t *child = &theme->colors;
    const lle_color_scheme_t *par = &parent->colors;

#define INHERIT_COLOR(field) \
    if (child->field.mode == LLE_COLOR_MODE_NONE) { \
        child->field = par->field; \
    }

    INHERIT_COLOR(primary);
    INHERIT_COLOR(secondary);
    INHERIT_COLOR(success);
    INHERIT_COLOR(warning);
    INHERIT_COLOR(error);
    INHERIT_COLOR(info);
    INHERIT_COLOR(text);
    INHERIT_COLOR(text_dim);
    INHERIT_COLOR(text_bright);
    INHERIT_COLOR(border);
    INHERIT_COLOR(background);
    INHERIT_COLOR(highlight);
    INHERIT_COLOR(git_clean);
    INHERIT_COLOR(git_dirty);
    INHERIT_COLOR(git_staged);
    INHERIT_COLOR(git_untracked);
    INHERIT_COLOR(git_branch);
    INHERIT_COLOR(git_ahead);
    INHERIT_COLOR(git_behind);
    INHERIT_COLOR(path_home);
    INHERIT_COLOR(path_root);
    INHERIT_COLOR(path_normal);
    INHERIT_COLOR(path_separator);
    INHERIT_COLOR(status_ok);
    INHERIT_COLOR(status_error);
    INHERIT_COLOR(status_running);

#undef INHERIT_COLOR

    /* Inherit symbols if empty */
#define INHERIT_SYMBOL(field) \
    if (strlen(theme->symbols.field) == 0) { \
        memcpy(theme->symbols.field, parent->symbols.field, \
               sizeof(theme->symbols.field)); \
    }

    INHERIT_SYMBOL(prompt);
    INHERIT_SYMBOL(prompt_root);
    INHERIT_SYMBOL(continuation);
    INHERIT_SYMBOL(separator_left);
    INHERIT_SYMBOL(separator_right);
    INHERIT_SYMBOL(branch);
    INHERIT_SYMBOL(staged);
    INHERIT_SYMBOL(unstaged);
    INHERIT_SYMBOL(untracked);
    INHERIT_SYMBOL(ahead);
    INHERIT_SYMBOL(behind);
    INHERIT_SYMBOL(stash);
    INHERIT_SYMBOL(conflict);
    INHERIT_SYMBOL(directory);
    INHERIT_SYMBOL(home);
    INHERIT_SYMBOL(error);
    INHERIT_SYMBOL(success);
    INHERIT_SYMBOL(time);
    INHERIT_SYMBOL(jobs);

#undef INHERIT_SYMBOL

    /* Inherit layout if not set */
    if (strlen(theme->layout.ps1_format) == 0) {
        memcpy(theme->layout.ps1_format, parent->layout.ps1_format,
               sizeof(theme->layout.ps1_format));
    }
    if (strlen(theme->layout.ps2_format) == 0) {
        memcpy(theme->layout.ps2_format, parent->layout.ps2_format,
               sizeof(theme->layout.ps2_format));
    }
    if (strlen(theme->layout.rps1_format) == 0) {
        memcpy(theme->layout.rps1_format, parent->layout.rps1_format,
               sizeof(theme->layout.rps1_format));
    }
    if (strlen(theme->layout.transient_format) == 0) {
        memcpy(theme->layout.transient_format, parent->layout.transient_format,
               sizeof(theme->layout.transient_format));
    }

    /* Inherit capabilities (additive for inheritable flags) */
    theme->capabilities |= (parent->capabilities & LLE_THEME_CAP_INHERITABLE);

    return LLE_SUCCESS;
}

/* ============================================================================
 * Color Helpers
 * ============================================================================
 */

/**
 * @brief Create a basic ANSI color
 */
lle_color_t lle_color_basic(lle_basic_color_t color) {
    lle_color_t c = {0};
    c.mode = LLE_COLOR_MODE_BASIC;
    c.value.basic = (uint8_t)color;
    return c;
}

/**
 * @brief Create a 256-palette color
 */
lle_color_t lle_color_256(uint8_t index) {
    lle_color_t c = {0};
    c.mode = LLE_COLOR_MODE_256;
    c.value.palette = index;
    return c;
}

/**
 * @brief Create a true color (RGB)
 */
lle_color_t lle_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    lle_color_t c = {0};
    c.mode = LLE_COLOR_MODE_TRUE;
    c.value.rgb.r = r;
    c.value.rgb.g = g;
    c.value.rgb.b = b;
    return c;
}

/**
 * @brief Generate ANSI escape sequence for a color
 */
size_t lle_color_to_ansi(const lle_color_t *color,
                          bool foreground,
                          char *output,
                          size_t output_size) {
    if (!color || !output || output_size == 0) {
        return 0;
    }

    int base = foreground ? 30 : 40;
    int len = 0;

    switch (color->mode) {
    case LLE_COLOR_MODE_NONE:
        output[0] = '\0';
        return 0;

    case LLE_COLOR_MODE_BASIC:
        len = snprintf(output, output_size, "\033[%dm", base + color->value.basic);
        break;

    case LLE_COLOR_MODE_256:
        len = snprintf(output, output_size, "\033[%d;5;%dm",
                       foreground ? 38 : 48, color->value.palette);
        break;

    case LLE_COLOR_MODE_TRUE:
        len = snprintf(output, output_size, "\033[%d;2;%d;%d;%dm",
                       foreground ? 38 : 48,
                       color->value.rgb.r,
                       color->value.rgb.g,
                       color->value.rgb.b);
        break;
    }

    /* Add attributes if foreground */
    if (foreground && len > 0 && (size_t)len < output_size - 20) {
        char attrs[32] = "";
        if (color->bold) {
            strcat(attrs, "\033[1m");
        }
        if (color->dim) {
            strcat(attrs, "\033[2m");
        }
        if (color->italic) {
            strcat(attrs, "\033[3m");
        }
        if (color->underline) {
            strcat(attrs, "\033[4m");
        }
        if (strlen(attrs) > 0) {
            size_t remaining = output_size - (size_t)len;
            size_t attrs_len = strlen(attrs);
            if (attrs_len < remaining) {
                memcpy(output + len, attrs, attrs_len + 1);
                len += (int)attrs_len;
            }
        }
    }

    return (size_t)len;
}

/* ============================================================================
 * Symbol Set Helpers
 * ============================================================================
 */

/**
 * @brief Initialize symbol set with Unicode defaults
 */
void lle_symbol_set_init_unicode(lle_symbol_set_t *symbols) {
    if (!symbols) {
        return;
    }

    memset(symbols, 0, sizeof(*symbols));

    snprintf(symbols->prompt, sizeof(symbols->prompt), "%s", "$");
    snprintf(symbols->prompt_root, sizeof(symbols->prompt_root), "%s", "#");
    snprintf(symbols->continuation, sizeof(symbols->continuation), "%s", ">");
    snprintf(symbols->branch, sizeof(symbols->branch), "%s", "");
    snprintf(symbols->staged, sizeof(symbols->staged), "%s", "+");
    snprintf(symbols->unstaged, sizeof(symbols->unstaged), "%s", "!");
    snprintf(symbols->untracked, sizeof(symbols->untracked), "%s", "?");
    snprintf(symbols->ahead, sizeof(symbols->ahead), "%s", "↑");
    snprintf(symbols->behind, sizeof(symbols->behind), "%s", "↓");
    snprintf(symbols->stash, sizeof(symbols->stash), "%s", "≡");
    snprintf(symbols->conflict, sizeof(symbols->conflict), "%s", "!");
    snprintf(symbols->home, sizeof(symbols->home), "%s", "~");
    snprintf(symbols->error, sizeof(symbols->error), "%s", "✗");
    snprintf(symbols->success, sizeof(symbols->success), "%s", "✓");
    snprintf(symbols->jobs, sizeof(symbols->jobs), "%s", "⚙");
}

/**
 * @brief Initialize symbol set with ASCII fallbacks
 */
void lle_symbol_set_init_ascii(lle_symbol_set_t *symbols) {
    if (!symbols) {
        return;
    }

    memset(symbols, 0, sizeof(*symbols));

    snprintf(symbols->prompt, sizeof(symbols->prompt), "%s", "$");
    snprintf(symbols->prompt_root, sizeof(symbols->prompt_root), "%s", "#");
    snprintf(symbols->continuation, sizeof(symbols->continuation), "%s", ">");
    snprintf(symbols->branch, sizeof(symbols->branch), "%s", "");
    snprintf(symbols->staged, sizeof(symbols->staged), "%s", "+");
    snprintf(symbols->unstaged, sizeof(symbols->unstaged), "%s", "!");
    snprintf(symbols->untracked, sizeof(symbols->untracked), "%s", "?");
    snprintf(symbols->ahead, sizeof(symbols->ahead), "%s", "^");
    snprintf(symbols->behind, sizeof(symbols->behind), "%s", "v");
    snprintf(symbols->stash, sizeof(symbols->stash), "%s", "=");
    snprintf(symbols->conflict, sizeof(symbols->conflict), "%s", "!");
    snprintf(symbols->home, sizeof(symbols->home), "%s", "~");
    snprintf(symbols->error, sizeof(symbols->error), "%s", "x");
    snprintf(symbols->success, sizeof(symbols->success), "%s", "o");
    snprintf(symbols->jobs, sizeof(symbols->jobs), "%s", "*");
}

/* ============================================================================
 * Built-in Themes
 * ============================================================================
 */

/**
 * @brief Create the minimal theme
 */
lle_theme_t *lle_theme_create_minimal(void) {
    lle_theme_t *theme = lle_theme_create("minimal",
                                          "Minimal prompt with directory only",
                                          LLE_THEME_CATEGORY_MINIMAL);
    if (!theme) {
        return NULL;
    }

    theme->source = LLE_THEME_SOURCE_BUILTIN;
    theme->capabilities = LLE_THEME_CAP_ASCII_FALLBACK | LLE_THEME_CAP_INHERITABLE;

    /* Colors */
    theme->colors.path_normal = lle_color_basic(LLE_COLOR_BLUE);

    /* Layout */
    snprintf(theme->layout.ps1_format, sizeof(theme->layout.ps1_format),
             "${directory} ${symbol} ");
    snprintf(theme->layout.ps2_format, sizeof(theme->layout.ps2_format),
             "> ");

    return theme;
}

/**
 * @brief Create the default theme
 */
lle_theme_t *lle_theme_create_default(void) {
    lle_theme_t *theme = lle_theme_create("default",
                                          "Default lusush theme",
                                          LLE_THEME_CATEGORY_MODERN);
    if (!theme) {
        return NULL;
    }

    theme->source = LLE_THEME_SOURCE_BUILTIN;
    theme->capabilities = LLE_THEME_CAP_UNICODE | LLE_THEME_CAP_INHERITABLE;

    /* Colors */
    theme->colors.primary = lle_color_basic(LLE_COLOR_GREEN);
    theme->colors.path_normal = lle_color_basic(LLE_COLOR_BLUE);
    theme->colors.git_branch = lle_color_basic(LLE_COLOR_MAGENTA);
    theme->colors.git_dirty = lle_color_basic(LLE_COLOR_RED);
    theme->colors.git_clean = lle_color_basic(LLE_COLOR_GREEN);
    theme->colors.error = lle_color_basic(LLE_COLOR_RED);

    /* Layout */
    snprintf(theme->layout.ps1_format, sizeof(theme->layout.ps1_format),
             "${user}@${host}:${directory}${?git: (${git})} ${symbol} ");
    snprintf(theme->layout.ps2_format, sizeof(theme->layout.ps2_format),
             "> ");

    return theme;
}

/**
 * @brief Create the classic theme (bash-like)
 */
lle_theme_t *lle_theme_create_classic(void) {
    lle_theme_t *theme = lle_theme_create("classic",
                                          "Classic bash-style prompt",
                                          LLE_THEME_CATEGORY_CLASSIC);
    if (!theme) {
        return NULL;
    }

    theme->source = LLE_THEME_SOURCE_BUILTIN;
    theme->capabilities = LLE_THEME_CAP_ASCII_FALLBACK | LLE_THEME_CAP_INHERITABLE;

    /* ASCII-only symbols */
    lle_symbol_set_init_ascii(&theme->symbols);

    /* Layout */
    snprintf(theme->layout.ps1_format, sizeof(theme->layout.ps1_format),
             "[${user}@${host} ${directory}]${symbol} ");
    snprintf(theme->layout.ps2_format, sizeof(theme->layout.ps2_format),
             "> ");

    return theme;
}

/**
 * @brief Create the powerline theme
 */
lle_theme_t *lle_theme_create_powerline(void) {
    lle_theme_t *theme = lle_theme_create("powerline",
                                          "Powerline-style prompt",
                                          LLE_THEME_CATEGORY_POWERLINE);
    if (!theme) {
        return NULL;
    }

    theme->source = LLE_THEME_SOURCE_BUILTIN;
    theme->capabilities = LLE_THEME_CAP_UNICODE | LLE_THEME_CAP_POWERLINE |
                          LLE_THEME_CAP_TRANSIENT | LLE_THEME_CAP_ASYNC_SEGMENTS;

    /* Colors */
    theme->colors.primary = lle_color_basic(LLE_COLOR_CYAN);
    theme->colors.path_normal = lle_color_basic(LLE_COLOR_BLUE);
    theme->colors.git_branch = lle_color_basic(LLE_COLOR_MAGENTA);
    theme->colors.git_dirty = lle_color_basic(LLE_COLOR_YELLOW);
    theme->colors.git_clean = lle_color_basic(LLE_COLOR_GREEN);
    theme->colors.error = lle_color_basic(LLE_COLOR_RED);

    /* Powerline symbols - these are special Unicode characters */
    strncpy(theme->symbols.separator_left, "\xee\x82\xb0", /* U+E0B0 */
            sizeof(theme->symbols.separator_left) - 1);
    strncpy(theme->symbols.separator_right, "\xee\x82\xb2", /* U+E0B2 */
            sizeof(theme->symbols.separator_right) - 1);

    /* Layout with transient */
    snprintf(theme->layout.ps1_format, sizeof(theme->layout.ps1_format),
             "${?status:${status} }${directory}${?git: ${git}} ${symbol} ");
    snprintf(theme->layout.ps2_format, sizeof(theme->layout.ps2_format),
             "... ");
    snprintf(theme->layout.transient_format, sizeof(theme->layout.transient_format),
             "${symbol} ");
    theme->layout.enable_transient = true;

    return theme;
}

/**
 * @brief Create the informative theme
 */
lle_theme_t *lle_theme_create_informative(void) {
    lle_theme_t *theme = lle_theme_create("informative",
                                          "Detailed information prompt",
                                          LLE_THEME_CATEGORY_MODERN);
    if (!theme) {
        return NULL;
    }

    theme->source = LLE_THEME_SOURCE_BUILTIN;
    theme->capabilities = LLE_THEME_CAP_UNICODE | LLE_THEME_CAP_MULTILINE |
                          LLE_THEME_CAP_RIGHT_PROMPT | LLE_THEME_CAP_ASYNC_SEGMENTS;

    /* Colors */
    theme->colors.primary = lle_color_basic(LLE_COLOR_CYAN);
    theme->colors.path_normal = lle_color_basic(LLE_COLOR_BLUE);
    theme->colors.git_branch = lle_color_basic(LLE_COLOR_MAGENTA);
    theme->colors.error = lle_color_basic(LLE_COLOR_RED);

    /* Multi-line layout */
    snprintf(theme->layout.ps1_format, sizeof(theme->layout.ps1_format),
             "${user}@${host}:${directory}${?git:\\n  git:(${git})}\n"
             "${?status:[${status}] }${symbol} ");
    snprintf(theme->layout.ps2_format, sizeof(theme->layout.ps2_format),
             "... ");
    snprintf(theme->layout.rps1_format, sizeof(theme->layout.rps1_format),
             "${time}${?jobs: [${jobs}]}");
    theme->layout.enable_right_prompt = true;
    theme->layout.enable_multiline = true;

    return theme;
}

/**
 * @brief Create the two-line theme
 */
lle_theme_t *lle_theme_create_two_line(void) {
    lle_theme_t *theme = lle_theme_create("two-line",
                                          "Two-line prompt with box drawing",
                                          LLE_THEME_CATEGORY_MODERN);
    if (!theme) {
        return NULL;
    }

    theme->source = LLE_THEME_SOURCE_BUILTIN;
    theme->capabilities = LLE_THEME_CAP_UNICODE | LLE_THEME_CAP_MULTILINE;

    /* Colors */
    theme->colors.primary = lle_color_basic(LLE_COLOR_GREEN);
    theme->colors.path_normal = lle_color_basic(LLE_COLOR_BLUE);
    theme->colors.git_branch = lle_color_basic(LLE_COLOR_YELLOW);

    /* Two-line layout */
    snprintf(theme->layout.ps1_format, sizeof(theme->layout.ps1_format),
             "┌─[${user}@${host}]─[${directory}]${?git:─[${git}]}\n"
             "└─${symbol} ");
    snprintf(theme->layout.ps2_format, sizeof(theme->layout.ps2_format),
             "   > ");
    theme->layout.enable_multiline = true;

    return theme;
}

/**
 * @brief Register all built-in themes
 */
size_t lle_theme_register_builtins(lle_theme_registry_t *registry) {
    if (!registry || !registry->initialized) {
        return 0;
    }

    size_t count = 0;
    lle_theme_t *theme;

    /* Minimal theme */
    theme = lle_theme_create_minimal();
    if (theme && lle_theme_registry_register(registry, theme) == LLE_SUCCESS) {
        count++;
    }

    /* Default theme */
    theme = lle_theme_create_default();
    if (theme && lle_theme_registry_register(registry, theme) == LLE_SUCCESS) {
        count++;
    }

    /* Classic theme */
    theme = lle_theme_create_classic();
    if (theme && lle_theme_registry_register(registry, theme) == LLE_SUCCESS) {
        count++;
    }

    /* Powerline theme */
    theme = lle_theme_create_powerline();
    if (theme && lle_theme_registry_register(registry, theme) == LLE_SUCCESS) {
        count++;
    }

    /* Informative theme */
    theme = lle_theme_create_informative();
    if (theme && lle_theme_registry_register(registry, theme) == LLE_SUCCESS) {
        count++;
    }

    /* Two-line theme */
    theme = lle_theme_create_two_line();
    if (theme && lle_theme_registry_register(registry, theme) == LLE_SUCCESS) {
        count++;
    }

    /* Set default active theme */
    if (count > 0) {
        lle_theme_registry_set_active(registry, "minimal");
    }

    return count;
}
