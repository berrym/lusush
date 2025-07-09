/**
 * Theme System Implementation for Lusush Shell
 *
 * Phase 3 Target 2: Advanced Configuration Themes
 * Comprehensive theming system with color schemes, prompt templates,
 * corporate branding, and enterprise customization capabilities
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "../include/themes.h"

#include "../include/config.h"
#include "../include/lusush.h"
#include "../include/prompt.h"
#include "../include/strings.h"
#include "../include/symtable.h"
#include "../include/termcap.h"

#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Global theme context
static theme_context_t theme_ctx = {0};
static bool theme_system_initialized = false;

// Theme system version
static const char *THEME_VERSION = "1.0.0";

// Default theme registry capacity
#define DEFAULT_THEME_CAPACITY 20
#define DEFAULT_VARIABLE_CAPACITY 50

// Terminal capability detection
static int terminal_color_support = 0;
static bool termcap_integrated = false;

// Debug flag
static bool debug_enabled = false;

// Forward declarations
static bool theme_register(theme_definition_t *theme);

// =============================================================================
// BUILT-IN THEME DEFINITIONS
// =============================================================================

/**
 * Corporate Theme: Professional, clean, business-appropriate
 */
static theme_definition_t *create_corporate_theme(void) {
    theme_definition_t *theme = malloc(sizeof(theme_definition_t));
    if (!theme) {
        if (debug_enabled) {
            printf("DEBUG: Failed to allocate memory for corporate theme\n");
        }
        return NULL;
    }
    if (debug_enabled) {
        printf("DEBUG: Creating corporate theme\n");
    }

    // Basic theme information
    strncpy(theme->name, "corporate", THEME_NAME_MAX - 1);
    strncpy(theme->description, "Professional theme for business environments",
            THEME_DESC_MAX - 1);
    strncpy(theme->author, "Lusush Team", BRAND_TEXT_MAX - 1);
    strncpy(theme->version, "1.0", 15);
    theme->category = THEME_CATEGORY_PROFESSIONAL;
    theme->is_built_in = true;
    theme->supports_256_color = true;
    theme->supports_true_color = false;
    theme->requires_powerline_fonts = false;

    // Corporate color scheme - professional blues and grays
    strncpy(theme->colors.primary, "\033[38;5;24m",
            COLOR_CODE_MAX - 1); // Dark blue
    strncpy(theme->colors.secondary, "\033[38;5;67m",
            COLOR_CODE_MAX - 1); // Steel blue
    strncpy(theme->colors.success, "\033[38;5;28m",
            COLOR_CODE_MAX - 1); // Dark green
    strncpy(theme->colors.warning, "\033[38;5;172m",
            COLOR_CODE_MAX - 1); // Orange
    strncpy(theme->colors.error, "\033[38;5;124m",
            COLOR_CODE_MAX - 1); // Dark red
    strncpy(theme->colors.info, "\033[38;5;31m", COLOR_CODE_MAX - 1); // Cyan
    strncpy(theme->colors.text, "\033[38;5;250m",
            COLOR_CODE_MAX - 1); // Light gray
    strncpy(theme->colors.text_dim, "\033[38;5;242m",
            COLOR_CODE_MAX - 1); // Dim gray
    strncpy(theme->colors.background, "\033[48;5;235m",
            COLOR_CODE_MAX - 1); // Dark gray bg
    strncpy(theme->colors.border, "\033[38;5;238m",
            COLOR_CODE_MAX - 1); // Medium gray
    strncpy(theme->colors.highlight, "\033[38;5;117m",
            COLOR_CODE_MAX - 1); // Light blue
    strncpy(theme->colors.git_clean, "\033[38;5;28m",
            COLOR_CODE_MAX - 1); // Green
    strncpy(theme->colors.git_dirty, "\033[38;5;172m",
            COLOR_CODE_MAX - 1); // Orange
    strncpy(theme->colors.git_staged, "\033[38;5;34m",
            COLOR_CODE_MAX - 1); // Bright green
    strncpy(theme->colors.git_branch, "\033[38;5;67m",
            COLOR_CODE_MAX - 1); // Steel blue
    strncpy(theme->colors.path_home, "\033[38;5;117m",
            COLOR_CODE_MAX - 1); // Light blue
    strncpy(theme->colors.path_root, "\033[38;5;124m",
            COLOR_CODE_MAX - 1); // Red
    strncpy(theme->colors.path_normal, "\033[38;5;250m",
            COLOR_CODE_MAX - 1); // Gray

    // Corporate prompt templates
    strncpy(theme->templates.primary_template,
            "%{primary}[%{text}%u@%h%{primary}] "
            "%{path_normal}%d%{git_branch}%g %{primary}$ %{text}",
            TEMPLATE_MAX - 1);
    strncpy(theme->templates.secondary_template, "%{border}> %{text}",
            TEMPLATE_MAX - 1);
    strncpy(theme->templates.right_prompt, "%{text_dim}%t", TEMPLATE_MAX - 1);
    theme->templates.enable_right_prompt = true;
    theme->templates.enable_timestamp = false;
    theme->templates.enable_git_status = true;
    theme->templates.enable_exit_code = false;
    theme->templates.enable_job_count = false;
    theme->templates.compact_mode = false;

    // Corporate branding defaults
    memset(&theme->branding, 0, sizeof(branding_config_t));
    theme->branding.show_company_in_prompt = false;
    theme->branding.show_logo_on_startup = false;

    // Visual effects - minimal for professional use
    theme->effects.enable_animations = false;
    theme->effects.enable_transitions = false;
    theme->effects.enable_gradient_colors = false;
    theme->effects.enable_icons = false;
    theme->effects.enable_powerline = false;

    return theme;
}

/**
 * Dark Theme: Modern dark theme with bright accents
 */
static theme_definition_t *create_dark_theme(void) {
    theme_definition_t *theme = malloc(sizeof(theme_definition_t));
    if (!theme) {
        return NULL;
    }

    strncpy(theme->name, "dark", THEME_NAME_MAX - 1);
    strncpy(theme->description, "Modern dark theme with bright accent colors",
            THEME_DESC_MAX - 1);
    strncpy(theme->author, "Lusush Team", BRAND_TEXT_MAX - 1);
    strncpy(theme->version, "1.0", 15);
    theme->category = THEME_CATEGORY_DEVELOPER;
    theme->is_built_in = true;
    theme->supports_256_color = true;
    theme->supports_true_color = true;
    theme->requires_powerline_fonts = false;

    // Dark theme colors - bright on dark
    strncpy(theme->colors.primary, "\033[38;5;39m",
            COLOR_CODE_MAX - 1); // Bright blue
    strncpy(theme->colors.secondary, "\033[38;5;141m",
            COLOR_CODE_MAX - 1); // Purple
    strncpy(theme->colors.success, "\033[38;5;46m",
            COLOR_CODE_MAX - 1); // Bright green
    strncpy(theme->colors.warning, "\033[38;5;226m",
            COLOR_CODE_MAX - 1); // Bright yellow
    strncpy(theme->colors.error, "\033[38;5;196m",
            COLOR_CODE_MAX - 1); // Bright red
    strncpy(theme->colors.info, "\033[38;5;51m",
            COLOR_CODE_MAX - 1); // Bright cyan
    strncpy(theme->colors.text, "\033[38;5;255m", COLOR_CODE_MAX - 1); // White
    strncpy(theme->colors.text_dim, "\033[38;5;244m",
            COLOR_CODE_MAX - 1); // Gray
    strncpy(theme->colors.background, "\033[48;5;232m",
            COLOR_CODE_MAX - 1); // Very dark
    strncpy(theme->colors.border, "\033[38;5;240m",
            COLOR_CODE_MAX - 1); // Dark gray
    strncpy(theme->colors.highlight, "\033[38;5;51m",
            COLOR_CODE_MAX - 1); // Bright cyan
    strncpy(theme->colors.git_clean, "\033[38;5;46m",
            COLOR_CODE_MAX - 1); // Bright green
    strncpy(theme->colors.git_dirty, "\033[38;5;226m",
            COLOR_CODE_MAX - 1); // Yellow
    strncpy(theme->colors.git_staged, "\033[38;5;82m",
            COLOR_CODE_MAX - 1); // Lime green
    strncpy(theme->colors.git_branch, "\033[38;5;141m",
            COLOR_CODE_MAX - 1); // Purple
    strncpy(theme->colors.path_home, "\033[38;5;39m",
            COLOR_CODE_MAX - 1); // Bright blue
    strncpy(theme->colors.path_root, "\033[38;5;196m",
            COLOR_CODE_MAX - 1); // Bright red
    strncpy(theme->colors.path_normal, "\033[38;5;255m",
            COLOR_CODE_MAX - 1); // White

    // Dark theme templates
    strncpy(theme->templates.primary_template,
            "%{primary}┌─[%{text}%u%{text_dim}@%{text}%h%{primary}]─[%{path_"
            "normal}%d%{primary}]%{git_branch}%g\n"
            "%{primary}└─%{highlight}$ %{text}",
            TEMPLATE_MAX - 1);
    strncpy(theme->templates.secondary_template,
            "%{primary}└─%{highlight}> %{text}", TEMPLATE_MAX - 1);
    strncpy(theme->templates.right_prompt, "%{text_dim}%t", TEMPLATE_MAX - 1);
    theme->templates.enable_right_prompt = true;
    theme->templates.enable_timestamp = true;
    theme->templates.enable_git_status = true;
    theme->templates.enable_exit_code = true;
    theme->templates.enable_job_count = false;
    theme->templates.compact_mode = false;

    // Branding defaults
    memset(&theme->branding, 0, sizeof(branding_config_t));

    // Visual effects - moderate for development
    theme->effects.enable_animations = false;
    theme->effects.enable_transitions = true;
    theme->effects.enable_gradient_colors = false;
    theme->effects.enable_icons = true;
    theme->effects.enable_powerline = false;

    return theme;
}

/**
 * Light Theme: Clean light theme with good contrast
 */
static theme_definition_t *create_light_theme(void) {
    theme_definition_t *theme = malloc(sizeof(theme_definition_t));
    if (!theme) {
        return NULL;
    }

    strncpy(theme->name, "light", THEME_NAME_MAX - 1);
    strncpy(theme->description, "Clean light theme with excellent readability",
            THEME_DESC_MAX - 1);
    strncpy(theme->author, "Lusush Team", BRAND_TEXT_MAX - 1);
    strncpy(theme->version, "1.0", 15);
    theme->category = THEME_CATEGORY_MINIMAL;
    theme->is_built_in = true;
    theme->supports_256_color = true;
    theme->supports_true_color = false;
    theme->requires_powerline_fonts = false;

    // Light theme colors - dark on light
    strncpy(theme->colors.primary, "\033[38;5;21m",
            COLOR_CODE_MAX - 1); // Dark blue
    strncpy(theme->colors.secondary, "\033[38;5;90m",
            COLOR_CODE_MAX - 1); // Dark purple
    strncpy(theme->colors.success, "\033[38;5;22m",
            COLOR_CODE_MAX - 1); // Dark green
    strncpy(theme->colors.warning, "\033[38;5;130m",
            COLOR_CODE_MAX - 1); // Dark orange
    strncpy(theme->colors.error, "\033[38;5;88m",
            COLOR_CODE_MAX - 1); // Dark red
    strncpy(theme->colors.info, "\033[38;5;23m",
            COLOR_CODE_MAX - 1); // Dark cyan
    strncpy(theme->colors.text, "\033[38;5;232m",
            COLOR_CODE_MAX - 1); // Very dark gray
    strncpy(theme->colors.text_dim, "\033[38;5;243m",
            COLOR_CODE_MAX - 1); // Medium gray
    strncpy(theme->colors.background, "\033[48;5;255m",
            COLOR_CODE_MAX - 1); // White
    strncpy(theme->colors.border, "\033[38;5;250m",
            COLOR_CODE_MAX - 1); // Light gray
    strncpy(theme->colors.highlight, "\033[38;5;27m",
            COLOR_CODE_MAX - 1); // Blue
    strncpy(theme->colors.git_clean, "\033[38;5;22m",
            COLOR_CODE_MAX - 1); // Dark green
    strncpy(theme->colors.git_dirty, "\033[38;5;130m",
            COLOR_CODE_MAX - 1); // Orange
    strncpy(theme->colors.git_staged, "\033[38;5;28m",
            COLOR_CODE_MAX - 1); // Green
    strncpy(theme->colors.git_branch, "\033[38;5;90m",
            COLOR_CODE_MAX - 1); // Purple
    strncpy(theme->colors.path_home, "\033[38;5;21m",
            COLOR_CODE_MAX - 1); // Blue
    strncpy(theme->colors.path_root, "\033[38;5;88m",
            COLOR_CODE_MAX - 1); // Dark red
    strncpy(theme->colors.path_normal, "\033[38;5;232m",
            COLOR_CODE_MAX - 1); // Dark gray

    // Light theme templates - clean and simple
    strncpy(theme->templates.primary_template,
            "%{text}%u@%h:%{path_normal}%d%{git_branch}%g%{primary}$ %{text}",
            TEMPLATE_MAX - 1);
    strncpy(theme->templates.secondary_template, "%{border}> %{text}",
            TEMPLATE_MAX - 1);
    strncpy(theme->templates.right_prompt, "", TEMPLATE_MAX - 1);
    theme->templates.enable_right_prompt = false;
    theme->templates.enable_timestamp = false;
    theme->templates.enable_git_status = true;
    theme->templates.enable_exit_code = false;
    theme->templates.enable_job_count = false;
    theme->templates.compact_mode = true;

    // Branding defaults
    memset(&theme->branding, 0, sizeof(branding_config_t));

    // Visual effects - minimal for clean appearance
    theme->effects.enable_animations = false;
    theme->effects.enable_transitions = false;
    theme->effects.enable_gradient_colors = false;
    theme->effects.enable_icons = false;
    theme->effects.enable_powerline = false;

    return theme;
}

/**
 * Colorful Theme: Vibrant theme for creative workflows
 */
static theme_definition_t *create_colorful_theme(void) {
    theme_definition_t *theme = malloc(sizeof(theme_definition_t));
    if (!theme) {
        return NULL;
    }

    strncpy(theme->name, "colorful", THEME_NAME_MAX - 1);
    strncpy(theme->description, "Vibrant colorful theme for creative workflows",
            THEME_DESC_MAX - 1);
    strncpy(theme->author, "Lusush Team", BRAND_TEXT_MAX - 1);
    strncpy(theme->version, "1.0", 15);
    theme->category = THEME_CATEGORY_CREATIVE;
    theme->is_built_in = true;
    theme->supports_256_color = true;
    theme->supports_true_color = true;
    theme->requires_powerline_fonts = false;

    // Colorful theme - vibrant rainbow colors
    strncpy(theme->colors.primary, "\033[38;5;201m",
            COLOR_CODE_MAX - 1); // Magenta
    strncpy(theme->colors.secondary, "\033[38;5;45m",
            COLOR_CODE_MAX - 1); // Cyan
    strncpy(theme->colors.success, "\033[38;5;118m",
            COLOR_CODE_MAX - 1); // Lime
    strncpy(theme->colors.warning, "\033[38;5;220m",
            COLOR_CODE_MAX - 1);                                        // Gold
    strncpy(theme->colors.error, "\033[38;5;196m", COLOR_CODE_MAX - 1); // Red
    strncpy(theme->colors.info, "\033[38;5;75m",
            COLOR_CODE_MAX - 1); // Sky blue
    strncpy(theme->colors.text, "\033[38;5;255m", COLOR_CODE_MAX - 1); // White
    strncpy(theme->colors.text_dim, "\033[38;5;245m",
            COLOR_CODE_MAX - 1); // Gray
    strncpy(theme->colors.background, "\033[48;5;233m",
            COLOR_CODE_MAX - 1); // Dark
    strncpy(theme->colors.border, "\033[38;5;93m",
            COLOR_CODE_MAX - 1); // Purple
    strncpy(theme->colors.highlight, "\033[38;5;226m",
            COLOR_CODE_MAX - 1); // Yellow
    strncpy(theme->colors.git_clean, "\033[38;5;118m",
            COLOR_CODE_MAX - 1); // Lime
    strncpy(theme->colors.git_dirty, "\033[38;5;220m",
            COLOR_CODE_MAX - 1); // Gold
    strncpy(theme->colors.git_staged, "\033[38;5;46m",
            COLOR_CODE_MAX - 1); // Green
    strncpy(theme->colors.git_branch, "\033[38;5;201m",
            COLOR_CODE_MAX - 1); // Magenta
    strncpy(theme->colors.path_home, "\033[38;5;45m",
            COLOR_CODE_MAX - 1); // Cyan
    strncpy(theme->colors.path_root, "\033[38;5;196m",
            COLOR_CODE_MAX - 1); // Red
    strncpy(theme->colors.path_normal, "\033[38;5;75m",
            COLOR_CODE_MAX - 1); // Sky blue

    // Colorful templates with Unicode symbols
    strncpy(theme->templates.primary_template,
            "%{primary}● %{text}%u%{text_dim}@%{secondary}%h "
            "%{path_normal}%d%{git_branch}%g %{highlight}➜ %{text}",
            TEMPLATE_MAX - 1);
    strncpy(theme->templates.secondary_template, "%{secondary}▶ %{text}",
            TEMPLATE_MAX - 1);
    strncpy(theme->templates.right_prompt, "%{text_dim}⏰ %t",
            TEMPLATE_MAX - 1);
    theme->templates.enable_right_prompt = true;
    theme->templates.enable_timestamp = true;
    theme->templates.enable_git_status = true;
    theme->templates.enable_exit_code = true;
    theme->templates.enable_job_count = true;
    theme->templates.compact_mode = false;

    // Branding defaults
    memset(&theme->branding, 0, sizeof(branding_config_t));

    // Visual effects - full experience for creative users
    theme->effects.enable_animations = false;
    theme->effects.enable_transitions = true;
    theme->effects.enable_gradient_colors = true;
    theme->effects.enable_icons = true;
    theme->effects.enable_powerline = false;

    return theme;
}

/**
 * Minimal Theme: Ultra-clean theme for focused work
 */
static theme_definition_t *create_minimal_theme(void) {
    theme_definition_t *theme = malloc(sizeof(theme_definition_t));
    if (!theme) {
        return NULL;
    }

    strncpy(theme->name, "minimal", THEME_NAME_MAX - 1);
    strncpy(theme->description, "Ultra-minimal theme for distraction-free work",
            THEME_DESC_MAX - 1);
    strncpy(theme->author, "Lusush Team", BRAND_TEXT_MAX - 1);
    strncpy(theme->version, "1.0", 15);
    theme->category = THEME_CATEGORY_MINIMAL;
    theme->is_built_in = true;
    theme->supports_256_color = false;
    theme->supports_true_color = false;
    theme->requires_powerline_fonts = false;

    // Minimal colors - only basic ANSI
    strncpy(theme->colors.primary, "\033[0m", COLOR_CODE_MAX - 1);    // Reset
    strncpy(theme->colors.secondary, "\033[0m", COLOR_CODE_MAX - 1);  // Reset
    strncpy(theme->colors.success, "\033[32m", COLOR_CODE_MAX - 1);   // Green
    strncpy(theme->colors.warning, "\033[33m", COLOR_CODE_MAX - 1);   // Yellow
    strncpy(theme->colors.error, "\033[31m", COLOR_CODE_MAX - 1);     // Red
    strncpy(theme->colors.info, "\033[36m", COLOR_CODE_MAX - 1);      // Cyan
    strncpy(theme->colors.text, "\033[0m", COLOR_CODE_MAX - 1);       // Default
    strncpy(theme->colors.text_dim, "\033[2m", COLOR_CODE_MAX - 1);   // Dim
    strncpy(theme->colors.background, "\033[0m", COLOR_CODE_MAX - 1); // Default
    strncpy(theme->colors.border, "\033[0m", COLOR_CODE_MAX - 1);     // Default
    strncpy(theme->colors.highlight, "\033[1m", COLOR_CODE_MAX - 1);  // Bold
    strncpy(theme->colors.git_clean, "\033[32m", COLOR_CODE_MAX - 1); // Green
    strncpy(theme->colors.git_dirty, "\033[33m", COLOR_CODE_MAX - 1); // Yellow
    strncpy(theme->colors.git_staged, "\033[32m", COLOR_CODE_MAX - 1); // Green
    strncpy(theme->colors.git_branch, "\033[0m", COLOR_CODE_MAX - 1); // Default
    strncpy(theme->colors.path_home, "\033[0m", COLOR_CODE_MAX - 1);  // Default
    strncpy(theme->colors.path_root, "\033[31m", COLOR_CODE_MAX - 1); // Red
    strncpy(theme->colors.path_normal, "\033[0m",
            COLOR_CODE_MAX - 1); // Default

    // Minimal template - extremely simple
    strncpy(theme->templates.primary_template, "$ ", TEMPLATE_MAX - 1);
    strncpy(theme->templates.secondary_template, "> ", TEMPLATE_MAX - 1);
    strncpy(theme->templates.right_prompt, "", TEMPLATE_MAX - 1);
    theme->templates.enable_right_prompt = false;
    theme->templates.enable_timestamp = false;
    theme->templates.enable_git_status = false;
    theme->templates.enable_exit_code = false;
    theme->templates.enable_job_count = false;
    theme->templates.compact_mode = true;

    // Branding defaults
    memset(&theme->branding, 0, sizeof(branding_config_t));

    // No visual effects for minimal theme
    memset(&theme->effects, 0, sizeof(visual_effects_t));

    return theme;
}

/**
 * Classic Theme: Traditional shell appearance
 */
static theme_definition_t *create_classic_theme(void) {
    theme_definition_t *theme = malloc(sizeof(theme_definition_t));
    if (!theme) {
        return NULL;
    }

    strncpy(theme->name, "classic", THEME_NAME_MAX - 1);
    strncpy(theme->description,
            "Traditional shell appearance with basic colors",
            THEME_DESC_MAX - 1);
    strncpy(theme->author, "Lusush Team", BRAND_TEXT_MAX - 1);
    strncpy(theme->version, "1.0", 15);
    theme->category = THEME_CATEGORY_CLASSIC;
    theme->is_built_in = true;
    theme->supports_256_color = false;
    theme->supports_true_color = false;
    theme->requires_powerline_fonts = false;

    // Classic ANSI colors
    strncpy(theme->colors.primary, "\033[34m", COLOR_CODE_MAX - 1);   // Blue
    strncpy(theme->colors.secondary, "\033[35m", COLOR_CODE_MAX - 1); // Magenta
    strncpy(theme->colors.success, "\033[32m", COLOR_CODE_MAX - 1);   // Green
    strncpy(theme->colors.warning, "\033[33m", COLOR_CODE_MAX - 1);   // Yellow
    strncpy(theme->colors.error, "\033[31m", COLOR_CODE_MAX - 1);     // Red
    strncpy(theme->colors.info, "\033[36m", COLOR_CODE_MAX - 1);      // Cyan
    strncpy(theme->colors.text, "\033[0m", COLOR_CODE_MAX - 1);       // Default
    strncpy(theme->colors.text_dim, "\033[2m", COLOR_CODE_MAX - 1);   // Dim
    strncpy(theme->colors.background, "\033[0m", COLOR_CODE_MAX - 1); // Default
    strncpy(theme->colors.border, "\033[37m", COLOR_CODE_MAX - 1);    // White
    strncpy(theme->colors.highlight, "\033[1;37m",
            COLOR_CODE_MAX - 1); // Bright white
    strncpy(theme->colors.git_clean, "\033[32m", COLOR_CODE_MAX - 1);  // Green
    strncpy(theme->colors.git_dirty, "\033[33m", COLOR_CODE_MAX - 1);  // Yellow
    strncpy(theme->colors.git_staged, "\033[32m", COLOR_CODE_MAX - 1); // Green
    strncpy(theme->colors.git_branch, "\033[35m",
            COLOR_CODE_MAX - 1);                                      // Magenta
    strncpy(theme->colors.path_home, "\033[34m", COLOR_CODE_MAX - 1); // Blue
    strncpy(theme->colors.path_root, "\033[31m", COLOR_CODE_MAX - 1); // Red
    strncpy(theme->colors.path_normal, "\033[0m",
            COLOR_CODE_MAX - 1); // Default

    // Classic bash-style template
    strncpy(theme->templates.primary_template,
            "%{text}%u@%h:%{path_normal}%d%{git_branch}%g%{primary}$ %{text}",
            TEMPLATE_MAX - 1);
    strncpy(theme->templates.secondary_template, "> ", TEMPLATE_MAX - 1);
    strncpy(theme->templates.right_prompt, "", TEMPLATE_MAX - 1);
    theme->templates.enable_right_prompt = false;
    theme->templates.enable_timestamp = false;
    theme->templates.enable_git_status = true;
    theme->templates.enable_exit_code = false;
    theme->templates.enable_job_count = false;
    theme->templates.compact_mode = false;

    // Branding defaults
    memset(&theme->branding, 0, sizeof(branding_config_t));

    // No visual effects for classic theme
    memset(&theme->effects, 0, sizeof(visual_effects_t));

    return theme;
}

// =============================================================================
// THEME MANAGEMENT IMPLEMENTATION
// =============================================================================

/**
 * Initialize the theme system
 */
bool theme_init(void) {
    if (theme_system_initialized) {
        return true;
    }

    // Initialize theme registry
    theme_ctx.registry.themes =
        malloc(DEFAULT_THEME_CAPACITY * sizeof(theme_definition_t *));
    if (!theme_ctx.registry.themes) {
        return false;
    }

    theme_ctx.registry.count = 0;
    theme_ctx.registry.capacity = DEFAULT_THEME_CAPACITY;
    theme_ctx.registry.active_theme_name = NULL;
    theme_ctx.registry.active_theme = NULL;
    theme_ctx.registry.initialized = true;

    // Set default directories
    theme_ctx.user_theme_dir = NULL;
    theme_ctx.system_theme_dir = NULL;
    theme_ctx.auto_detect_capabilities = true;
    theme_ctx.fallback_to_basic = true;
    theme_ctx.debug_mode = false;

    // Integrate with termcap system for enhanced terminal detection
    const terminal_info_t *term_info = termcap_get_info();
    if (term_info && term_info->is_tty) {
        termcap_integrated = true;
        // Use termcap for more accurate terminal detection
        terminal_color_support = theme_detect_color_support();
        
        // Adjust theme capabilities based on terminal type
        if (termcap_is_iterm2()) {
            theme_ctx.auto_detect_capabilities = true;
        } else if (termcap_is_tmux() || termcap_is_screen()) {
            // More conservative for multiplexers
            theme_ctx.auto_detect_capabilities = false;
        }
    } else {
        // Fallback to traditional detection
        terminal_color_support = theme_detect_color_support();
    }

    // Set initialized flag before registering themes
    theme_system_initialized = true;

    // Register built-in themes
    int registered = theme_register_builtin_themes();
    if (registered < 6) { // Expecting 6 built-in themes
        if (debug_enabled) {
            fprintf(stderr, "Warning: Only %d built-in themes registered\n",
                    registered);
        }
    }

    // Set default theme to corporate
    if (!theme_set_active("corporate")) {
        // Fallback to minimal if corporate fails
        theme_set_active("minimal");
    }
    return true;
}

/**
 * Cleanup the theme system
 */
void theme_cleanup(void) {
    if (!theme_system_initialized) {
        return;
    }

    // Free all registered themes
    if (theme_ctx.registry.themes) {
        for (size_t i = 0; i < theme_ctx.registry.count; i++) {
            if (theme_ctx.registry.themes[i]) {
                theme_free_definition(theme_ctx.registry.themes[i]);
            }
        }
        free(theme_ctx.registry.themes);
    }

    // Free active theme name
    if (theme_ctx.registry.active_theme_name) {
        free(theme_ctx.registry.active_theme_name);
    }

    // Free directory paths
    if (theme_ctx.user_theme_dir) {
        free(theme_ctx.user_theme_dir);
    }
    if (theme_ctx.system_theme_dir) {
        free(theme_ctx.system_theme_dir);
    }

    // Reset context
    memset(&theme_ctx, 0, sizeof(theme_context_t));
    theme_system_initialized = false;
}

/**
 * Register all built-in themes
 */
int theme_register_builtin_themes(void) {
    int count = 0;
    theme_definition_t *theme;

    // Register corporate theme
    theme = create_corporate_theme();
    if (debug_enabled) {
        printf("DEBUG: Corporate theme created: %p\n", (void *)theme);
    }
    if (theme && theme_register(theme)) {
        count++;
        if (debug_enabled) {
            printf("DEBUG: Corporate theme registered successfully\n");
        }
    } else {
        if (debug_enabled) {
            printf("DEBUG: Failed to register corporate theme\n");
        }
    }

    // Register dark theme
    theme = create_dark_theme();
    if (theme && theme_register(theme)) {
        count++;
    }

    // Register light theme
    theme = create_light_theme();
    if (theme && theme_register(theme)) {
        count++;
    }

    // Register colorful theme
    theme = create_colorful_theme();
    if (theme && theme_register(theme)) {
        count++;
    }

    // Register minimal theme
    theme = create_minimal_theme();
    if (theme && theme_register(theme)) {
        count++;
    }

    // Register classic theme
    theme = create_classic_theme();
    if (theme && theme_register(theme)) {
        count++;
    }

    if (debug_enabled) {
        printf("Registered %d built-in themes\n", count);
    }

    return count;
}

/**
 * Register a theme in the registry
 */
static bool theme_register(theme_definition_t *theme) {
    if (!theme) {
        if (debug_enabled) {
            printf("DEBUG: theme_register: theme is NULL\n");
        }
        return false;
    }
    if (!theme_system_initialized) {
        if (debug_enabled) {
            printf("DEBUG: theme_register: theme system not initialized\n");
        }
        return false;
    }
    if (debug_enabled) {
        printf("DEBUG: theme_register: entering for theme '%s'\n", theme->name);
    }

    // Check if registry needs expansion
    if (theme_ctx.registry.count >= theme_ctx.registry.capacity) {
        if (debug_enabled) {
            printf("DEBUG: theme_register: expanding registry capacity\n");
        }
        size_t new_capacity = theme_ctx.registry.capacity * 2;
        theme_definition_t **new_themes =
            realloc(theme_ctx.registry.themes,
                    new_capacity * sizeof(theme_definition_t *));
        if (!new_themes) {
            if (debug_enabled) {
                printf("DEBUG: theme_register: failed to expand registry\n");
            }
            return false;
        }
        theme_ctx.registry.themes = new_themes;
        theme_ctx.registry.capacity = new_capacity;
    }

    // Validate theme before registering
    if (!theme_validate(theme)) {
        if (debug_enabled) {
            printf("DEBUG: Theme validation failed for: %s\n", theme->name);
        }
        return false;
    }
    if (debug_enabled) {
        printf("DEBUG: Theme validation passed for: %s\n", theme->name);
    }

    // Check for duplicate names
    for (size_t i = 0; i < theme_ctx.registry.count; i++) {
        if (strcmp(theme_ctx.registry.themes[i]->name, theme->name) == 0) {
            if (debug_enabled) {
                printf("Theme with name '%s' already exists\n", theme->name);
            }
            return false;
        }
    }

    // Register the theme
    theme_ctx.registry.themes[theme_ctx.registry.count] = theme;
    theme_ctx.registry.count++;

    if (debug_enabled) {
        printf("Registered theme: %s\n", theme->name);
    }

    return true;
}

/**
 * Load a theme by name
 */
theme_definition_t *theme_load(const char *name) {
    if (!name || !theme_system_initialized) {
        return NULL;
    }

    // Search in registry
    for (size_t i = 0; i < theme_ctx.registry.count; i++) {
        if (strcmp(theme_ctx.registry.themes[i]->name, name) == 0) {
            return theme_ctx.registry.themes[i];
        }
    }

    if (debug_enabled) {
        printf("Theme not found: %s\n", name);
    }

    return NULL;
}

/**
 * Set active theme by name
 */
bool theme_set_active(const char *name) {
    if (!name || !theme_system_initialized) {
        return false;
    }

    theme_definition_t *theme = theme_load(name);
    if (!theme) {
        return false;
    }

    // Update active theme
    if (theme_ctx.registry.active_theme_name) {
        free(theme_ctx.registry.active_theme_name);
    }
    theme_ctx.registry.active_theme_name = strdup(name);
    theme_ctx.registry.active_theme = theme;

    if (debug_enabled) {
        printf("Set active theme: %s\n", name);
    }

    return true;
}

/**
 * Get currently active theme
 */
theme_definition_t *theme_get_active(void) {
    if (!theme_system_initialized) {
        return NULL;
    }
    return theme_ctx.registry.active_theme;
}

/**
 * Validate theme definition
 */
bool theme_validate(const theme_definition_t *theme) {
    if (!theme) {
        if (debug_enabled) {
            printf("DEBUG: theme_validate: theme is NULL\n");
        }
        return false;
    }

    // Check required fields
    if (strlen(theme->name) == 0) {
        if (debug_enabled) {
            printf("DEBUG: theme_validate: theme name is empty\n");
        }
        return false;
    }

    if (strlen(theme->description) == 0) {
        if (debug_enabled) {
            printf("DEBUG: theme_validate: theme description is empty\n");
        }
        return false;
    }

    // Validate color codes
    if (!theme_validate_color_code(theme->colors.primary)) {
        if (debug_enabled) {
            printf("DEBUG: theme_validate: primary color invalid: '%s'\n",
                   theme->colors.primary);
        }
        return false;
    }
    if (!theme_validate_color_code(theme->colors.text)) {
        if (debug_enabled) {
            printf("DEBUG: theme_validate: text color invalid: '%s'\n",
                   theme->colors.text);
        }
        return false;
    }
    if (!theme_validate_color_code(theme->colors.success)) {
        if (debug_enabled) {
            printf("DEBUG: theme_validate: success color invalid: '%s'\n",
                   theme->colors.success);
        }
        return false;
    }
    if (!theme_validate_color_code(theme->colors.error)) {
        if (debug_enabled) {
            printf("DEBUG: theme_validate: error color invalid: '%s'\n",
                   theme->colors.error);
        }
        return false;
    }

    // Check template validity
    if (strlen(theme->templates.primary_template) == 0) {
        if (debug_enabled) {
            printf("DEBUG: theme_validate: primary template is empty\n");
        }
        return false;
    }

    if (debug_enabled) {
        printf("DEBUG: theme_validate: all validations passed for theme '%s'\n",
               theme->name);
    }
    return true;
}

/**
 * Detect terminal color capabilities
 */
int theme_detect_color_support(void) {
    const char *term = getenv("TERM");
    const char *colorterm = getenv("COLORTERM");

    // Check for true color support
    if (colorterm &&
        (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit"))) {
        return 16777216; // True color
    }

    // Check for 256 color support
    if (term && (strstr(term, "256") || strstr(term, "xterm"))) {
        return 256;
    }

    // Check for basic color support
    if (term && (strstr(term, "color") || strcmp(term, "linux") == 0)) {
        return 8;
    }

    // No color support detected
    return 0;
}

/**
 * Validate color code format
 */
bool theme_validate_color_code(const char *color_code) {
    if (!color_code) {
        return false;
    }

    // Empty string is valid (no color)
    if (strlen(color_code) == 0) {
        return true;
    }

    // Must start with escape sequence
    if (strncmp(color_code, "\033[", 2) != 0) {
        return false;
    }

    // Must end with 'm'
    size_t len = strlen(color_code);
    if (len < 3 || color_code[len - 1] != 'm') {
        return false;
    }

    return true;
}

/**
 * Get color code for semantic color name
 */
const char *theme_get_color(const char *color_name) {
    if (!color_name || !theme_system_initialized) {
        return "";
    }

    theme_definition_t *theme = theme_get_active();
    if (!theme) {
        return "";
    }

    // Map semantic names to color codes
    if (strcmp(color_name, "primary") == 0) {
        return theme->colors.primary;
    }
    if (strcmp(color_name, "secondary") == 0) {
        return theme->colors.secondary;
    }
    if (strcmp(color_name, "success") == 0) {
        return theme->colors.success;
    }
    if (strcmp(color_name, "warning") == 0) {
        return theme->colors.warning;
    }
    if (strcmp(color_name, "error") == 0) {
        return theme->colors.error;
    }
    if (strcmp(color_name, "info") == 0) {
        return theme->colors.info;
    }
    if (strcmp(color_name, "text") == 0) {
        return theme->colors.text;
    }
    if (strcmp(color_name, "text_dim") == 0) {
        return theme->colors.text_dim;
    }
    if (strcmp(color_name, "background") == 0) {
        return theme->colors.background;
    }
    if (strcmp(color_name, "border") == 0) {
        return theme->colors.border;
    }
    if (strcmp(color_name, "highlight") == 0) {
        return theme->colors.highlight;
    }
    if (strcmp(color_name, "git_clean") == 0) {
        return theme->colors.git_clean;
    }
    if (strcmp(color_name, "git_dirty") == 0) {
        return theme->colors.git_dirty;
    }
    if (strcmp(color_name, "git_staged") == 0) {
        return theme->colors.git_staged;
    }
    if (strcmp(color_name, "git_branch") == 0) {
        return theme->colors.git_branch;
    }
    if (strcmp(color_name, "path_home") == 0) {
        return theme->colors.path_home;
    }
    if (strcmp(color_name, "path_root") == 0) {
        return theme->colors.path_root;
    }
    if (strcmp(color_name, "path_normal") == 0) {
        return theme->colors.path_normal;
    }

    return "";
}

/**
 * Apply color to text
 */
bool theme_colorize_text(const char *text, const char *color_name, char *output,
                         size_t output_size) {
    if (!text || !color_name || !output || output_size < strlen(text) + 20) {
        return false;
    }

    const char *color_code = theme_get_color(color_name);
    if (strlen(color_code) == 0) {
        // No color, just copy text
        strncpy(output, text, output_size - 1);
        output[output_size - 1] = '\0';
        return true;
    }

    // Apply color: color_code + text + reset
    snprintf(output, output_size, "%s%s\033[0m", color_code, text);
    return true;
}

/**
 * Free theme definition
 */
void theme_free_definition(theme_definition_t *theme) {
    if (theme) {
        free(theme);
    }
}

/**
 * Get theme system version
 */
const char *theme_get_version(void) { return THEME_VERSION; }

/**
 * Set debug mode
 */
void theme_set_debug_mode(bool enabled) {
    debug_enabled = enabled;
    theme_ctx.debug_mode = enabled;
}

/**
 * Get theme getters for built-in themes
 */
theme_definition_t *theme_get_corporate(void) {
    return theme_load("corporate");
}
theme_definition_t *theme_get_dark(void) { return theme_load("dark"); }
theme_definition_t *theme_get_light(void) { return theme_load("light"); }
theme_definition_t *theme_get_colorful(void) { return theme_load("colorful"); }
theme_definition_t *theme_get_minimal(void) { return theme_load("minimal"); }
theme_definition_t *theme_get_classic(void) { return theme_load("classic"); }
theme_definition_t *theme_get_developer(void) {
    return theme_load("dark");
} // Alias for dark

// =============================================================================
// TEMPLATE ENGINE IMPLEMENTATION
// =============================================================================

/**
 * Initialize template context
 */
template_context_t *template_init_context(void) {
    template_context_t *ctx = malloc(sizeof(template_context_t));
    if (!ctx) {
        return NULL;
    }

    ctx->variables =
        malloc(DEFAULT_VARIABLE_CAPACITY * sizeof(template_variable_t));
    if (!ctx->variables) {
        free(ctx);
        return NULL;
    }

    ctx->count = 0;
    ctx->capacity = DEFAULT_VARIABLE_CAPACITY;
    ctx->current_template = NULL;
    ctx->multiline_mode = false;

    return ctx;
}

/**
 * Free template context
 */
void template_free_context(template_context_t *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->variables) {
        for (size_t i = 0; i < ctx->count; i++) {
            if (ctx->variables[i].name) {
                free(ctx->variables[i].name);
            }
            if (ctx->variables[i].value) {
                free(ctx->variables[i].value);
            }
            if (ctx->variables[i].color) {
                free(ctx->variables[i].color);
            }
        }
        free(ctx->variables);
    }

    if (ctx->current_template) {
        free(ctx->current_template);
    }

    free(ctx);
}

/**
 * Add template variable
 */
bool template_add_variable(template_context_t *ctx, const char *name,
                           const char *value, const char *color, bool dynamic) {
    if (!ctx || !name || !value) {
        return false;
    }

    // Check if we need to expand capacity
    if (ctx->count >= ctx->capacity) {
        size_t new_capacity = ctx->capacity * 2;
        template_variable_t *new_vars =
            realloc(ctx->variables, new_capacity * sizeof(template_variable_t));
        if (!new_vars) {
            return false;
        }
        ctx->variables = new_vars;
        ctx->capacity = new_capacity;
    }

    // Add the variable
    template_variable_t *var = &ctx->variables[ctx->count];
    var->name = strdup(name);
    var->value = strdup(value);
    var->color = color ? strdup(color) : NULL;
    var->dynamic = dynamic;

    if (!var->name || !var->value) {
        if (var->name) {
            free(var->name);
        }
        if (var->value) {
            free(var->value);
        }
        if (var->color) {
            free(var->color);
        }
        return false;
    }

    ctx->count++;
    return true;
}

/**
 * Process template with variables
 */
bool template_process(const char *template, template_context_t *ctx,
                      char *output, size_t output_size) {
    if (!template || !ctx || !output || output_size < 1) {
        return false;
    }

    size_t template_len = strlen(template);
    size_t output_pos = 0;
    size_t i = 0;

    while (i < template_len && output_pos < output_size - 1) {
        if (template[i] == '%' && i + 1 < template_len) {
            if (template[i + 1] == '{') {
                // Found %{variable} format
                i += 2; // Skip %{
                size_t var_start = i;

                // Find variable end
                while (i < template_len && template[i] != '}') {
                    i++;
                }

                if (i >= template_len) {
                    // Malformed variable, copy literally
                    if (output_pos < output_size - 2) {
                        output[output_pos++] = '%';
                        output[output_pos++] = '{';
                    }
                    i = var_start;
                    continue;
                }

                // Extract variable name
                size_t var_len = i - var_start;
                char var_name[64];
                if (var_len < sizeof(var_name)) {
                    strncpy(var_name, template + var_start, var_len);
                    var_name[var_len] = '\0';

                    // Look up variable
                    const char *var_value = NULL;
                    const char *var_color = NULL;

                    for (size_t j = 0; j < ctx->count; j++) {
                        if (strcmp(ctx->variables[j].name, var_name) == 0) {
                            var_value = ctx->variables[j].value;
                            var_color = ctx->variables[j].color;
                            break;
                        }
                    }

                    // If not found in context, try theme colors
                    if (!var_value) {
                        var_color = theme_get_color(var_name);
                        if (strlen(var_color) > 0) {
                            var_value = var_color;
                            var_color = NULL; // Don't double-apply color
                        }
                    }

                    // Apply variable value
                    if (var_value) {
                        // Apply color if specified
                        if (var_color && strlen(var_color) > 0) {
                            size_t color_len = strlen(var_color);
                            if (output_pos + color_len < output_size) {
                                strcpy(output + output_pos, var_color);
                                output_pos += color_len;
                            }
                        }

                        // Add variable value
                        size_t value_len = strlen(var_value);
                        if (output_pos + value_len < output_size) {
                            strcpy(output + output_pos, var_value);
                            output_pos += value_len;
                        }

                        // Add reset if color was applied
                        if (var_color && strlen(var_color) > 0) {
                            const char *reset = "\033[0m";
                            size_t reset_len = strlen(reset);
                            if (output_pos + reset_len < output_size) {
                                strcpy(output + output_pos, reset);
                                output_pos += reset_len;
                            }
                        }
                    }
                }

                i++; // Skip }
            } else {
                // Found %variable format (single character)
                char var_name[2] = {template[i + 1], '\0'};

                // Look up single character variable
                const char *var_value = NULL;

                for (size_t j = 0; j < ctx->count; j++) {
                    if (strcmp(ctx->variables[j].name, var_name) == 0) {
                        var_value = ctx->variables[j].value;
                        break;
                    }
                }

                if (var_value) {
                    // Add variable value
                    size_t value_len = strlen(var_value);
                    if (output_pos + value_len < output_size) {
                        strcpy(output + output_pos, var_value);
                        output_pos += value_len;
                    }
                    i += 2; // Skip %var
                } else {
                    // Unknown variable, copy literally
                    output[output_pos++] = template[i];
                    i++;
                }
            }
        } else {
            // Regular character
            output[output_pos++] = template[i];
            i++;
        }
    }

    output[output_pos] = '\0';
    return true;
}

/**
 * Process template with responsive layout for terminal-aware rendering
 */
bool template_process_responsive(const char *template, template_context_t *ctx,
                               char *output, size_t output_size,
                               int terminal_width, bool use_colors) {
    if (!template || !ctx || !output || output_size < 1) {
        return false;
    }

    // First, do basic template processing
    if (!template_process(template, ctx, output, output_size)) {
        return false;
    }

    // Apply responsive adjustments based on terminal capabilities
    if (terminal_width < 60) {
        // For narrow terminals, simplify the prompt
        char simple_output[output_size];
        size_t len = strlen(output);
        size_t pos = 0;
        
        // Remove excessive spacing and decorations for narrow terminals
        for (size_t i = 0; i < len && pos < output_size - 1; i++) {
            if (output[i] == '\t') {
                // Replace tabs with single spaces in narrow terminals
                simple_output[pos++] = ' ';
            } else if (output[i] == ' ' && i > 0 && output[i-1] == ' ') {
                // Skip multiple consecutive spaces
                continue;
            } else {
                simple_output[pos++] = output[i];
            }
        }
        simple_output[pos] = '\0';
        strncpy(output, simple_output, output_size - 1);
        output[output_size - 1] = '\0';
    }

    // Remove color codes if terminal doesn't support colors
    if (!use_colors) {
        char no_color_output[output_size];
        size_t len = strlen(output);
        size_t pos = 0;
        
        for (size_t i = 0; i < len && pos < output_size - 1; i++) {
            if (output[i] == '\033') {
                // Skip ANSI escape sequences
                while (i < len && output[i] != 'm') {
                    i++;
                }
                continue;
            }
            no_color_output[pos++] = output[i];
        }
        no_color_output[pos] = '\0';
        strncpy(output, no_color_output, output_size - 1);
        output[output_size - 1] = '\0';
    }

    return true;
}

/**
 * Generate primary prompt using active theme
 */
bool theme_generate_primary_prompt(char *output, size_t output_size) {
    if (!output || output_size < 1) {
        return false;
    }

    theme_definition_t *theme = theme_get_active();
    if (!theme) {
        // Fallback to simple prompt
        strncpy(output, "$ ", output_size - 1);
        output[output_size - 1] = '\0';
        return true;
    }

    // Get terminal capabilities for responsive template rendering
    const terminal_info_t *term_info = termcap_get_info();
    bool has_terminal = term_info && term_info->is_tty;
    int terminal_width = term_info ? term_info->cols : 80;
    bool use_colors = has_terminal && terminal_color_support;

    // Create template context
    template_context_t *ctx = template_init_context();
    if (!ctx) {
        return false;
    }

    // Gather system information
    char username[_POSIX_LOGIN_NAME_MAX + 1] = {0};
    char hostname[_POSIX_HOST_NAME_MAX + 1] = {0};
    char directory[_POSIX_PATH_MAX + 1] = {0};
    char time_str[64] = {0};
    char git_info[256] = {0};

    // Get username
    if (getlogin_r(username, sizeof(username) - 1) != 0) {
        strncpy(username, "user", sizeof(username) - 1);
    }

    // Get hostname
    if (gethostname(hostname, sizeof(hostname) - 1) != 0) {
        strncpy(hostname, "localhost", sizeof(hostname) - 1);
    }

    // Get current directory
    if (!getcwd(directory, sizeof(directory) - 1)) {
        strncpy(directory, "~", sizeof(directory) - 1);
    } else {
        // Simplify home directory display
        const char *home = getenv("HOME");
        if (home && strncmp(directory, home, strlen(home)) == 0) {
            char temp[_POSIX_PATH_MAX + 1];
            snprintf(temp, sizeof(temp), "~%s", directory + strlen(home));
            strncpy(directory, temp, sizeof(directory) - 1);
        }
    }

    // Get time if enabled
    if (theme->templates.enable_timestamp) {
        time_t tm = time(NULL);
        struct tm *lt = localtime(&tm);
        if (lt) {
            strftime(time_str, sizeof(time_str), "%H:%M:%S", lt);
        }
    }

    // Get git information if enabled
    if (theme->templates.enable_git_status) {
        // Use the existing git prompt functionality from prompt.c
        update_git_info();
        format_git_prompt(git_info, sizeof(git_info) - 1);

        // Ensure null termination
        git_info[sizeof(git_info) - 1] = '\0';
    }

    // Add variables to context with terminal-aware formatting
    template_add_variable(ctx, "u", username, NULL, false);
    template_add_variable(ctx, "h", hostname, NULL, false);
    template_add_variable(ctx, "d", directory, NULL, false);
    template_add_variable(ctx, "t", time_str, NULL, false);
    template_add_variable(ctx, "g", git_info, NULL, false);
    
    // Add terminal capability variables
    theme_add_terminal_variables(ctx);
    
    // Update dynamic variables with current state
    theme_update_dynamic_variables(ctx);

    // Add corporate branding if configured
    const branding_config_t *branding = theme_get_branding();
    if (branding) {
        if (branding->show_company_in_prompt &&
            strlen(branding->company_name) > 0) {
            template_add_variable(ctx, "company", branding->company_name, NULL,
                                  false);
        }
        if (branding->show_department_in_prompt &&
            strlen(branding->department) > 0) {
            template_add_variable(ctx, "dept", branding->department, NULL,
                                  false);
        }
    }

    // Process template with responsive layout
    bool success = template_process_responsive(theme->templates.primary_template, ctx,
                                             output, output_size,
                                             terminal_width, use_colors);

    template_free_context(ctx);
    return success;
}

/**
 * Generate secondary prompt (PS2) using active theme
 */
bool theme_generate_secondary_prompt(char *output, size_t output_size) {
    if (!output || output_size < 1) {
        return false;
    }

    theme_definition_t *theme = theme_get_active();
    if (!theme) {
        strncpy(output, "> ", output_size - 1);
        output[output_size - 1] = '\0';
        return true;
    }

    // Get terminal capabilities for responsive template rendering
    const terminal_info_t *term_info = termcap_get_info();
    bool has_terminal = term_info && term_info->is_tty;
    int terminal_width = term_info ? term_info->cols : 80;
    bool use_colors = has_terminal && terminal_color_support;

    // Create template context
    template_context_t *ctx = template_init_context();
    if (!ctx) {
        return false;
    }

    // Add terminal capability variables for consistent access
    theme_add_terminal_variables(ctx);
    
    // Update dynamic variables with current state
    theme_update_dynamic_variables(ctx);

    // Process template with responsive layout
    bool success = template_process_responsive(theme->templates.secondary_template, ctx,
                                             output, output_size,
                                             terminal_width, use_colors);

    template_free_context(ctx);
    return success;
}

// =============================================================================
// CORPORATE BRANDING IMPLEMENTATION
// =============================================================================

static branding_config_t current_branding = {0};
static bool branding_configured = false;

/**
 * Set corporate branding information
 */
bool theme_set_branding(const branding_config_t *branding) {
    if (!branding) {
        return false;
    }

    current_branding = *branding;
    branding_configured = true;
    return true;
}

/**
 * Get current branding configuration
 */
const branding_config_t *theme_get_branding(void) {
    return branding_configured ? &current_branding : NULL;
}

/**
 * Display startup logo/branding
 */
void theme_display_startup_branding(void) {
    if (!branding_configured || !current_branding.show_logo_on_startup) {
        return;
    }

    // Get terminal capabilities for responsive branding display
    const terminal_info_t *term_info = termcap_get_info();
    bool has_terminal = term_info && term_info->is_tty;
    int terminal_width = term_info ? term_info->cols : 80;
    bool use_colors = has_terminal && terminal_color_support;

    if (strlen(current_branding.logo_ascii) > 0) {
        // Only display logo if terminal is wide enough
        if (terminal_width >= 60) {
            printf("%s\n", current_branding.logo_ascii);
        }
    }

    if (strlen(current_branding.company_name) > 0) {
        const char *primary_color = use_colors ? theme_get_color("primary") : "";
        const char *reset = use_colors ? "\033[0m" : "";
        
        // Center the company name if terminal is wide enough
        if (terminal_width >= 40) {
            char company_text[256];
            snprintf(company_text, sizeof(company_text), "%s Shell Environment", current_branding.company_name);
            int padding = (terminal_width - strlen(company_text)) / 2;
            if (padding > 0) {
                printf("%*s", padding, "");
            }
            printf("%s%s%s\n", primary_color, company_text, reset);
        } else {
            // Simple display for narrow terminals
            printf("%s%s%s\n", primary_color, current_branding.company_name, reset);
        }
    }
}

// =============================================================================
// DYNAMIC TEMPLATE VARIABLES
// =============================================================================

/**
 * Update dynamic template variables with current terminal state
 */
void theme_update_dynamic_variables(template_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    // Get current terminal capabilities
    const terminal_info_t *term_info = termcap_get_info();
    if (!term_info) {
        return;
    }
    
    // Update terminal size variables
    char cols_str[16], rows_str[16];
    snprintf(cols_str, sizeof(cols_str), "%d", term_info->cols);
    snprintf(rows_str, sizeof(rows_str), "%d", term_info->rows);
    
    // Update or add terminal capability variables
    for (size_t i = 0; i < ctx->count; i++) {
        if (ctx->variables[i].dynamic) {
            if (strcmp(ctx->variables[i].name, "cols") == 0) {
                free(ctx->variables[i].value);
                ctx->variables[i].value = strdup(cols_str);
            } else if (strcmp(ctx->variables[i].name, "rows") == 0) {
                free(ctx->variables[i].value);
                ctx->variables[i].value = strdup(rows_str);
            } else if (strcmp(ctx->variables[i].name, "terminal") == 0) {
                free(ctx->variables[i].value);
                ctx->variables[i].value = strdup(term_info->term_type ? term_info->term_type : "unknown");
            } else if (strcmp(ctx->variables[i].name, "has_colors") == 0) {
                free(ctx->variables[i].value);
                ctx->variables[i].value = strdup(term_info->is_tty && terminal_color_support ? "1" : "0");
            }
        }
    }
}

/**
 * Add terminal-specific dynamic variables to template context
 */
void theme_add_terminal_variables(template_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    const terminal_info_t *term_info = termcap_get_info();
    if (!term_info) {
        return;
    }
    
    // Add dynamic terminal variables
    char cols_str[16], rows_str[16];
    snprintf(cols_str, sizeof(cols_str), "%d", term_info->cols);
    snprintf(rows_str, sizeof(rows_str), "%d", term_info->rows);
    
    template_add_variable(ctx, "cols", cols_str, NULL, true);
    template_add_variable(ctx, "rows", rows_str, NULL, true);
    template_add_variable(ctx, "terminal", term_info->term_type ? term_info->term_type : "unknown", NULL, true);
    template_add_variable(ctx, "has_colors", term_info->is_tty && terminal_color_support ? "1" : "0", NULL, true);
    
    // Add platform-specific variables
    if (termcap_is_iterm2()) {
        template_add_variable(ctx, "iterm2", "1", NULL, false);
    }
    if (termcap_is_tmux()) {
        template_add_variable(ctx, "tmux", "1", NULL, false);
    }
    if (termcap_is_screen()) {
        template_add_variable(ctx, "screen", "1", NULL, false);
    }
}

// =============================================================================
// ADDITIONAL UTILITY FUNCTIONS
// =============================================================================

/**
 * List all available themes
 */
char **theme_list_available(theme_category_t category) {
    if (!theme_system_initialized) {
        return NULL;
    }

    // Count matching themes
    size_t count = 0;
    for (size_t i = 0; i < theme_ctx.registry.count; i++) {
        if ((int)category == -1 ||
            theme_ctx.registry.themes[i]->category == category) {
            count++;
        }
    }

    // Allocate string array
    char **list = malloc((count + 1) * sizeof(char *));
    if (!list) {
        return NULL;
    }

    // Fill array
    size_t idx = 0;
    for (size_t i = 0; i < theme_ctx.registry.count; i++) {
        if ((int)category == -1 ||
            theme_ctx.registry.themes[i]->category == category) {
            list[idx] = strdup(theme_ctx.registry.themes[i]->name);
            if (!list[idx]) {
                // Cleanup on failure
                for (size_t j = 0; j < idx; j++) {
                    free(list[j]);
                }
                free(list);
                return NULL;
            }
            idx++;
        }
    }
    list[count] = NULL; // NULL-terminate

    return list;
}

/**
 * Get theme statistics
 */
void theme_get_statistics(size_t *total_themes, size_t *builtin_themes,
                          size_t *custom_themes) {
    if (!theme_system_initialized) {
        if (total_themes) {
            *total_themes = 0;
        }
        if (builtin_themes) {
            *builtin_themes = 0;
        }
        if (custom_themes) {
            *custom_themes = 0;
        }
        return;
    }

    size_t builtin_count = 0;
    size_t custom_count = 0;

    for (size_t i = 0; i < theme_ctx.registry.count; i++) {
        if (theme_ctx.registry.themes[i]->is_built_in) {
            builtin_count++;
        } else {
            custom_count++;
        }
    }

    if (total_themes) {
        *total_themes = theme_ctx.registry.count;
    }
    if (builtin_themes) {
        *builtin_themes = builtin_count;
    }
    if (custom_themes) {
        *custom_themes = custom_count;
    }
}
