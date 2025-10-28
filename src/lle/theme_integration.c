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

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include <string.h>
#include <stdlib.h>

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
                    0x000000, 0x800000, 0x008000, 0x808000,
                    0x000080, 0x800080, 0x008080, 0xC0C0C0,
                    0x808080, 0xFF0000, 0x00FF00, 0xFFFF00,
                    0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
                };
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
                return ((uint32_t)gray << 16) | ((uint32_t)gray << 8) | (uint32_t)gray;
            }
        }
    }
    
    /* Basic color names mapping (fallback) */
    if (strstr(color_code, "30m")) return 0x000000; /* Black */
    if (strstr(color_code, "31m")) return 0xFF0000; /* Red */
    if (strstr(color_code, "32m")) return 0x00FF00; /* Green */
    if (strstr(color_code, "33m")) return 0xFFFF00; /* Yellow */
    if (strstr(color_code, "34m")) return 0x0000FF; /* Blue */
    if (strstr(color_code, "35m")) return 0xFF00FF; /* Magenta */
    if (strstr(color_code, "36m")) return 0x00FFFF; /* Cyan */
    if (strstr(color_code, "37m")) return 0xFFFFFF; /* White */
    
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
lle_result_t lle_extract_syntax_colors_from_theme(theme_definition_t *theme,
                                                  lle_syntax_color_table_t **table,
                                                  lle_memory_pool_t *memory_pool) {
    if (!theme || !table) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Allocate color table */
    lle_syntax_color_table_t *new_table = (lle_syntax_color_table_t *)
        lle_pool_allocate((lle_memory_pool_base_t*)memory_pool, sizeof(lle_syntax_color_table_t));
    if (!new_table) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Extract colors from theme color scheme */
    color_scheme_t *colors = &theme->colors;
    
    /* Map theme colors to syntax highlighting categories */
    new_table->keyword_color = parse_color_code(colors->primary);      /* Keywords use primary color */
    new_table->string_color = parse_color_code(colors->success);       /* Strings use success (green) */
    new_table->comment_color = parse_color_code(colors->text_dim);     /* Comments use dimmed text */
    new_table->number_color = parse_color_code(colors->info);          /* Numbers use info (blue) */
    new_table->operator_color = parse_color_code(colors->secondary);   /* Operators use secondary */
    new_table->variable_color = parse_color_code(colors->text);        /* Variables use normal text */
    new_table->function_color = parse_color_code(colors->highlight);   /* Functions highlighted */
    new_table->type_color = parse_color_code(colors->info);            /* Types use info color */
    new_table->default_color = parse_color_code(colors->text);         /* Default is normal text */
    
    /* Store theme context for future reference */
    new_table->theme_context = (void*)theme;
    
    *table = new_table;
    return LLE_SUCCESS;
}

/**
 * Extract cursor colors from theme
 * 
 * Converts theme color scheme to LLE cursor color format
 */
lle_result_t lle_extract_cursor_colors_from_theme(theme_definition_t *theme,
                                                  lle_cursor_colors_t **colors,
                                                  lle_memory_pool_t *memory_pool) {
    if (!theme || !colors) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Allocate cursor colors */
    lle_cursor_colors_t *new_colors = (lle_cursor_colors_t *)
        lle_pool_allocate((lle_memory_pool_base_t*)memory_pool, sizeof(lle_cursor_colors_t));
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
    new_colors->cursor_background_color = parse_color_code(theme_colors->primary);
    
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
lle_result_t lle_display_integrate_theme_system(lle_display_integration_t *integration,
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
    integration->theme_system = (void*)theme;
    
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
        lle_result_t result = lle_display_cache_invalidate_all(integration->display_cache);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    /* Re-integrate theme system with new theme */
    lle_result_t result = lle_display_integrate_theme_system(integration, new_theme);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Mark full redraw needed */
    if (integration->current_state) {
        integration->current_state->needs_full_redraw = true;
    }
    
    /* Clear dirty tracker (full redraw) */
    if (integration->render_controller && integration->render_controller->dirty_tracker) {
        lle_dirty_tracker_mark_full(integration->render_controller->dirty_tracker);
    }
    
    return LLE_SUCCESS;
}
