/**
 * @file theme_integration.c
 * @brief LLE Theme System Integration Implementation
 *
 * This module integrates LLE's display system with Lusush's theme manager,
 * extracting colors from themes and handling theme change events.
 *
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION PLAN: docs/lle_implementation/SPEC_08_IMPLEMENTATION_PLAN.md
 *                      Week 6 Day 3-5: Theme Integration
 */

#include "display/command_layer.h"
#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/syntax_highlighting.h"
#include <stdlib.h>
#include <string.h>

/* ========================================================================== */
/*                         HELPER FUNCTIONS                                   */
/* ========================================================================== */

/**
 * Parse ANSI color code to RGB value
 *
 * Converts color codes like "\033[38;2;255;128;0m" to uint32_t RGB
 * Also handles basic color codes and 256-color codes
 *
 * @param color_code ANSI color code string
 * @return RGB value (0xRRGGBB) or 0 if parsing fails
 */
static uint32_t parse_color_code(const char *color_code) {
    if (!color_code || strlen(color_code) == 0) {
        return 0xFFFFFF; /* Default white */
    }

    /* Check for truecolor (24-bit RGB) format: \033[38;2;R;G;Bm */
    if (strstr(color_code, "38;2;") != NULL) {
        int r = 0, g = 0, b = 0;
        const char *rgb_start = strstr(color_code, "38;2;");
        if (rgb_start && sscanf(rgb_start, "38;2;%d;%d;%d", &r, &g, &b) == 3) {
            return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }

    /* Check for 256-color format: \033[38;5;Nm */
    if (strstr(color_code, "38;5;") != NULL) {
        int color_index = 0;
        const char *idx_start = strstr(color_code, "38;5;");
        if (idx_start && sscanf(idx_start, "38;5;%d", &color_index) == 1) {
            /* Convert 256-color index to approximate RGB */
            if (color_index < 16) {
                /* Basic 16 colors - use standard palette */
                static const uint32_t basic_colors[16] = {
                    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080,
                    0x008080, 0xC0C0C0, 0x808080, 0xFF0000, 0x00FF00, 0xFFFF00,
                    0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF};
                return basic_colors[color_index];
            } else if (color_index < 232) {
                /* 216-color cube (6x6x6) */
                int idx = color_index - 16;
                int r = (idx / 36) * 51;
                int g = ((idx / 6) % 6) * 51;
                int b = (idx % 6) * 51;
                return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
            } else {
                /* Grayscale (24 shades) */
                int gray = 8 + (color_index - 232) * 10;
                return ((uint32_t)gray << 16) | ((uint32_t)gray << 8) |
                       (uint32_t)gray;
            }
        }
    }

    /* Basic color names mapping (fallback) */
    if (strstr(color_code, "30m"))
        return 0x000000; /* Black */
    if (strstr(color_code, "31m"))
        return 0xFF0000; /* Red */
    if (strstr(color_code, "32m"))
        return 0x00FF00; /* Green */
    if (strstr(color_code, "33m"))
        return 0xFFFF00; /* Yellow */
    if (strstr(color_code, "34m"))
        return 0x0000FF; /* Blue */
    if (strstr(color_code, "35m"))
        return 0xFF00FF; /* Magenta */
    if (strstr(color_code, "36m"))
        return 0x00FFFF; /* Cyan */
    if (strstr(color_code, "37m"))
        return 0xFFFFFF; /* White */

    /* Default to white if parsing fails */
    return 0xFFFFFF;
}

/* ========================================================================== */
/*                    THEME COLOR EXTRACTION FUNCTIONS                        */
/* ========================================================================== */

/**
 * Extract syntax highlighting colors from theme
 *
 * Converts theme color scheme to LLE syntax color table format
 */
lle_result_t
lle_extract_syntax_colors_from_theme(theme_definition_t *theme,
                                     lle_syntax_color_table_t **table,
                                     lle_memory_pool_t *memory_pool) {
    if (!theme || !table) {
        return LLE_ERROR_NULL_POINTER;
    }

    /* Allocate color table */
    lle_syntax_color_table_t *new_table =
        (lle_syntax_color_table_t *)lle_pool_allocate(
            (lle_memory_pool_base_t *)memory_pool,
            sizeof(lle_syntax_color_table_t));
    if (!new_table) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Extract colors from theme color scheme */
    color_scheme_t *colors = &theme->colors;

    /* Map theme colors to syntax highlighting categories */
    new_table->keyword_color =
        parse_color_code(colors->primary); /* Keywords use primary color */
    new_table->string_color =
        parse_color_code(colors->success); /* Strings use success (green) */
    new_table->comment_color =
        parse_color_code(colors->text_dim); /* Comments use dimmed text */
    new_table->number_color =
        parse_color_code(colors->info); /* Numbers use info (blue) */
    new_table->operator_color =
        parse_color_code(colors->secondary); /* Operators use secondary */
    new_table->variable_color =
        parse_color_code(colors->text); /* Variables use normal text */
    new_table->function_color =
        parse_color_code(colors->highlight); /* Functions highlighted */
    new_table->type_color =
        parse_color_code(colors->info); /* Types use info color */
    new_table->default_color =
        parse_color_code(colors->text); /* Default is normal text */

    /* Store theme context for future reference */
    new_table->theme_context = (void *)theme;

    *table = new_table;
    return LLE_SUCCESS;
}

/**
 * Extract syntax highlighting colors from theme to LLE syntax highlighter
 * format
 *
 * Converts theme's syntax_color_scheme_t (ANSI codes) to lle_syntax_colors_t
 * (RGB)
 */
lle_result_t lle_theme_get_syntax_colors(theme_definition_t *theme,
                                         lle_syntax_colors_t *colors) {
    if (!theme || !colors) {
        return LLE_ERROR_NULL_POINTER;
    }

    /* Get theme's syntax color scheme */
    syntax_color_scheme_t *syn = &theme->syntax;

    /* Convert ANSI codes to RGB values */
    colors->command_valid = parse_color_code(syn->command_valid);
    colors->command_invalid = parse_color_code(syn->command_invalid);
    colors->command_builtin = parse_color_code(syn->command_builtin);
    colors->command_alias = parse_color_code(syn->command_alias);
    colors->command_function =
        parse_color_code(syn->command_builtin); /* Same as builtin */

    colors->keyword = parse_color_code(syn->keyword);

    colors->string = parse_color_code(syn->string);
    colors->string_escape =
        parse_color_code(syn->error_syntax); /* Use error color for escapes */

    colors->variable = parse_color_code(syn->variable);
    colors->variable_special = parse_color_code(syn->variable_special);

    colors->path_valid = parse_color_code(syn->path_valid);
    colors->path_invalid = parse_color_code(syn->path_invalid);

    colors->pipe = parse_color_code(syn->pipe);
    colors->redirect = parse_color_code(syn->redirect);
    colors->operator_other = parse_color_code(syn->operator_sym);

    colors->comment = parse_color_code(syn->comment);
    colors->number = parse_color_code(syn->number);
    colors->option = parse_color_code(syn->option);
    colors->glob = parse_color_code(syn->glob);
    colors->argument = parse_color_code(syn->option); /* Same as option */

    colors->error = parse_color_code(syn->error_syntax);
    colors->error_fg = 0xFFFFFF; /* White on error background */

    /* Set text attributes based on typical conventions */
    colors->keyword_bold = 1;
    colors->command_bold = 1;
    colors->error_underline = 1;
    colors->path_underline = 1;
    colors->comment_dim = 1;

    return LLE_SUCCESS;
}

/**
 * Apply theme syntax colors to command layer color scheme
 *
 * Converts theme's syntax_color_scheme_t to command_color_scheme_t
 * and applies it to the command layer for syntax highlighting.
 *
 * @param theme Theme containing syntax colors
 * @param cmd_layer Command layer to update
 * @return LLE_SUCCESS on success, error code on failure
 */
static lle_result_t
apply_theme_syntax_to_command_layer(theme_definition_t *theme,
                                    command_layer_t *cmd_layer) {
    if (!theme || !cmd_layer) {
        return LLE_ERROR_NULL_POINTER;
    }

    syntax_color_scheme_t *syn = &theme->syntax;
    command_color_scheme_t cmd_colors;

    /* Map theme syntax colors to command layer color scheme */
    /* command_valid -> command_color (commands in command position) */
    strncpy(cmd_colors.command_color, syn->command_valid,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.command_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* Use option color for arguments (what follows commands) */
    strncpy(cmd_colors.argument_color, syn->option,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.argument_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* option -> option_color */
    strncpy(cmd_colors.option_color, syn->option,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.option_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* string -> string_color */
    strncpy(cmd_colors.string_color, syn->string,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.string_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* variable -> variable_color */
    strncpy(cmd_colors.variable_color, syn->variable,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.variable_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* redirect -> redirect_color */
    strncpy(cmd_colors.redirect_color, syn->redirect,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.redirect_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* pipe -> pipe_color */
    strncpy(cmd_colors.pipe_color, syn->pipe,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.pipe_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* keyword -> keyword_color */
    strncpy(cmd_colors.keyword_color, syn->keyword,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.keyword_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* operator_sym -> operator_color */
    strncpy(cmd_colors.operator_color, syn->operator_sym,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.operator_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* path_valid -> path_color */
    strncpy(cmd_colors.path_color, syn->path_valid,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.path_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* number -> number_color */
    strncpy(cmd_colors.number_color, syn->number,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.number_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* comment -> comment_color */
    strncpy(cmd_colors.comment_color, syn->comment,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.comment_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* error_syntax -> error_color */
    strncpy(cmd_colors.error_color, syn->error_syntax,
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.error_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* Reset color is always the standard ANSI reset */
    strncpy(cmd_colors.reset_color, "\033[0m",
            COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1);
    cmd_colors.reset_color[COMMAND_LAYER_MAX_COLOR_CODE_SIZE - 1] = '\0';

    /* Apply the color scheme to the command layer */
    command_layer_error_t result =
        command_layer_set_color_scheme(cmd_layer, &cmd_colors);
    if (result != COMMAND_LAYER_SUCCESS) {
        return LLE_ERROR_DISPLAY_INTEGRATION;
    }

    return LLE_SUCCESS;
}

/**
 * Extract cursor colors from theme
 *
 * Converts theme color scheme to LLE cursor color format
 */
lle_result_t
lle_extract_cursor_colors_from_theme(theme_definition_t *theme,
                                     lle_cursor_colors_t **colors,
                                     lle_memory_pool_t *memory_pool) {
    if (!theme || !colors) {
        return LLE_ERROR_NULL_POINTER;
    }

    /* Allocate cursor colors */
    lle_cursor_colors_t *new_colors = (lle_cursor_colors_t *)lle_pool_allocate(
        (lle_memory_pool_base_t *)memory_pool, sizeof(lle_cursor_colors_t));
    if (!new_colors) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Extract cursor colors from theme */
    color_scheme_t *theme_colors = &theme->colors;

    /* Cursor uses highlight color */
    new_colors->cursor_color = parse_color_code(theme_colors->highlight);

    /* Text under cursor uses background for contrast */
    new_colors->cursor_text_color = parse_color_code(theme_colors->background);

    /* Cursor background uses primary/secondary blend */
    new_colors->cursor_background_color =
        parse_color_code(theme_colors->primary);

    *colors = new_colors;
    return LLE_SUCCESS;
}

/* ========================================================================== */
/*                    THEME INTEGRATION FUNCTIONS                             */
/* ========================================================================== */

/**
 * Integrate theme system with display integration
 *
 * Sets up theme system integration and extracts initial colors
 */
lle_result_t
lle_display_integrate_theme_system(lle_display_integration_t *integration,
                                   theme_definition_t *theme) {
    if (!integration) {
        return LLE_ERROR_NULL_POINTER;
    }

    /* Null theme is allowed - it means use defaults */
    if (!theme) {
        integration->theme_system = NULL;
        return LLE_SUCCESS;
    }

    /* Store theme reference (opaque pointer) */
    integration->theme_system = (void *)theme;

    /* Extract syntax colors from theme (store in render controller) */
    if (integration->render_controller) {
        lle_syntax_color_table_t *syntax_colors = NULL;
        lle_result_t result = lle_extract_syntax_colors_from_theme(
            theme, &syntax_colors, integration->memory_pool);

        if (result != LLE_SUCCESS) {
            return result;
        }

        /* Store in render controller's theme_colors field */
        integration->render_controller->theme_colors = syntax_colors;
    }

    /* Extract cursor colors from theme (store in render controller) */
    if (integration->render_controller) {
        lle_cursor_colors_t *cursor_colors = NULL;
        lle_result_t result = lle_extract_cursor_colors_from_theme(
            theme, &cursor_colors, integration->memory_pool);

        if (result != LLE_SUCCESS) {
            return result;
        }

        /* Store in render controller's cursor_theme_colors field */
        integration->render_controller->cursor_theme_colors = cursor_colors;
    }

    /* Apply theme syntax colors to command layer for real-time highlighting */
    if (integration->display_bridge &&
        integration->display_bridge->command_layer) {
        command_layer_t *cmd_layer =
            (command_layer_t *)integration->display_bridge->command_layer;
        lle_result_t result =
            apply_theme_syntax_to_command_layer(theme, cmd_layer);
        if (result != LLE_SUCCESS) {
            /* Non-fatal: continue with default colors if this fails */
            /* Log would go here in debug builds */
        }
    }

    return LLE_SUCCESS;
}

/**
 * Handle theme change event
 *
 * Reloads theme colors and invalidates caches when theme changes
 */
lle_result_t lle_on_theme_changed(lle_display_integration_t *integration,
                                  const char *theme_name) {
    if (!integration || !theme_name) {
        return LLE_ERROR_NULL_POINTER;
    }

    /* Load new theme using Lusush theme system */
    theme_definition_t *new_theme = theme_load(theme_name);
    if (!new_theme) {
        return LLE_ERROR_DISPLAY_INTEGRATION;
    }

    /* Invalidate display cache (colors changed) */
    if (integration->display_cache) {
        lle_result_t result =
            lle_display_cache_invalidate_all(integration->display_cache);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }

    /* Re-integrate theme system with new theme */
    lle_result_t result =
        lle_display_integrate_theme_system(integration, new_theme);
    if (result != LLE_SUCCESS) {
        return result;
    }

    /* Mark full redraw needed */
    if (integration->current_state) {
        integration->current_state->needs_full_redraw = true;
    }

    /* Clear dirty tracker (full redraw) */
    if (integration->render_controller &&
        integration->render_controller->dirty_tracker) {
        lle_dirty_tracker_mark_full(
            integration->render_controller->dirty_tracker);
    }

    return LLE_SUCCESS;
}
