/**
 * Theme System API for Lusush Shell
 *
 * Phase 3 Target 2: Advanced Configuration Themes
 * Provides comprehensive theming with color schemes, prompt templates,
 * and corporate branding support for enterprise environments
 */

#ifndef THEMES_H
#define THEMES_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum lengths for theme components
#define THEME_NAME_MAX 64
#define THEME_DESC_MAX 256
#define COLOR_CODE_MAX 16
#define TEMPLATE_MAX 1024
#define BRAND_TEXT_MAX 128

// Theme categories for organizational deployment
typedef enum {
    THEME_CATEGORY_PROFESSIONAL, // Corporate, business environments
    THEME_CATEGORY_CREATIVE,     // Colorful, artistic workflows
    THEME_CATEGORY_MINIMAL,      // Clean, distraction-free
    THEME_CATEGORY_CLASSIC,      // Traditional shell appearance
    THEME_CATEGORY_DEVELOPER,    // Programming-focused themes
    THEME_CATEGORY_CUSTOM        // User-defined themes
} theme_category_t;

// Semantic color definitions for consistent theming
typedef struct {
    char primary[COLOR_CODE_MAX];     // Main brand/accent color
    char secondary[COLOR_CODE_MAX];   // Secondary accent color
    char success[COLOR_CODE_MAX];     // Success indicators (green tones)
    char warning[COLOR_CODE_MAX];     // Warning indicators (yellow/orange)
    char error[COLOR_CODE_MAX];       // Error indicators (red tones)
    char info[COLOR_CODE_MAX];        // Information (blue tones)
    char text[COLOR_CODE_MAX];        // Primary text color
    char text_dim[COLOR_CODE_MAX];    // Dimmed/secondary text
    char background[COLOR_CODE_MAX];  // Background color
    char border[COLOR_CODE_MAX];      // Borders and separators
    char highlight[COLOR_CODE_MAX];   // Highlighted elements
    char git_clean[COLOR_CODE_MAX];   // Git clean status
    char git_dirty[COLOR_CODE_MAX];   // Git dirty status
    char git_staged[COLOR_CODE_MAX];  // Git staged changes
    char git_branch[COLOR_CODE_MAX];  // Git branch name
    char path_home[COLOR_CODE_MAX];   // Home directory
    char path_root[COLOR_CODE_MAX];   // Root directory
    char path_normal[COLOR_CODE_MAX]; // Normal directories
} color_scheme_t;

// Corporate branding configuration
typedef struct {
    char company_name[BRAND_TEXT_MAX]; // Company name for prompts
    char department[BRAND_TEXT_MAX];   // Department/team name
    char project[BRAND_TEXT_MAX];      // Current project name
    char environment[BRAND_TEXT_MAX];  // Environment (dev/staging/prod)
    char logo_ascii[TEMPLATE_MAX];     // ASCII art logo
    bool show_company_in_prompt;       // Include company in prompt
    bool show_department_in_prompt;    // Include department in prompt
    bool show_project_in_prompt;       // Include project in prompt
    bool show_environment_in_prompt;   // Include environment in prompt
    bool show_logo_on_startup;         // Display logo on shell start
} branding_config_t;

// Prompt template configuration
typedef struct {
    char primary_template[TEMPLATE_MAX];   // Main prompt template
    char secondary_template[TEMPLATE_MAX]; // Continuation prompt (PS2)
    char right_prompt[TEMPLATE_MAX];       // Right-side prompt
    bool enable_right_prompt;              // Show right-side prompt
    bool enable_timestamp;                 // Show timestamp
    bool enable_git_status;                // Show git information
    bool enable_exit_code;                 // Show last command exit code
    bool enable_job_count;                 // Show background job count
    bool compact_mode;                     // Compact single-line format
} prompt_template_t;

// Animation and visual effects
typedef struct {
    bool enable_animations;      // Enable prompt animations
    bool enable_transitions;     // Enable theme transitions
    bool enable_gradient_colors; // Enable color gradients
    bool enable_icons;           // Enable Unicode icons
    bool enable_powerline;       // Enable Powerline symbols
    int animation_speed;         // Animation speed (ms)
    int transition_duration;     // Theme transition time (ms)
} visual_effects_t;

// Complete theme definition
typedef struct {
    char name[THEME_NAME_MAX];        // Theme name
    char description[THEME_DESC_MAX]; // Theme description
    char author[BRAND_TEXT_MAX];      // Theme author
    char version[16];                 // Theme version
    theme_category_t category;        // Theme category
    color_scheme_t colors;            // Color scheme
    prompt_template_t templates;      // Prompt templates
    branding_config_t branding;       // Corporate branding
    visual_effects_t effects;         // Visual effects
    bool is_built_in;                 // Built-in vs custom theme
    bool supports_256_color;          // 256-color support
    bool supports_true_color;         // True color support
    bool requires_powerline_fonts;    // Powerline font requirement
} theme_definition_t;

// Theme registry for managing multiple themes
typedef struct {
    theme_definition_t **themes;      // Array of theme pointers
    size_t count;                     // Number of registered themes
    size_t capacity;                  // Registry capacity
    char *active_theme_name;          // Currently active theme
    theme_definition_t *active_theme; // Pointer to active theme
    bool initialized;                 // Registry initialization state
} theme_registry_t;

// Theme context for runtime management
typedef struct {
    theme_registry_t registry;     // Theme registry
    char *user_theme_dir;          // User theme directory
    char *system_theme_dir;        // System theme directory
    bool auto_detect_capabilities; // Auto-detect terminal capabilities
    bool fallback_to_basic;        // Fallback to basic colors
    int terminal_color_support;    // Terminal color capability
    bool debug_mode;               // Debug theme loading
} theme_context_t;

// Template variable structure for prompt generation
typedef struct {
    char *name;   // Variable name (e.g., "user")
    char *value;  // Variable value
    char *color;  // Variable color
    bool dynamic; // Dynamic value (recalculated)
} template_variable_t;

// Template context for prompt generation
typedef struct {
    template_variable_t *variables; // Array of variables
    size_t count;                   // Number of variables
    size_t capacity;                // Variables capacity
    char *current_template;         // Current template being processed
    bool multiline_mode;            // Multiline processing mode
} template_context_t;

// =============================================================================
// THEME MANAGEMENT API
// =============================================================================

/**
 * Initialize the theme system
 * @return true on success, false on failure
 */
bool theme_init(void);

/**
 * Cleanup the theme system and free resources
 */
void theme_cleanup(void);

/**
 * Register all built-in themes
 * @return number of themes registered
 */
int theme_register_builtin_themes(void);

/**
 * Load a theme by name
 * @param name Theme name to load
 * @return pointer to theme definition, NULL on failure
 */
theme_definition_t *theme_load(const char *name);

/**
 * Set active theme by name
 * @param name Theme name to activate
 * @return true on success, false on failure
 */
bool theme_set_active(const char *name);

/**
 * Get currently active theme
 * @return pointer to active theme, NULL if none
 */
theme_definition_t *theme_get_active(void);

/**
 * List all available themes
 * @param category Filter by category (optional, use -1 for all)
 * @return array of theme names, NULL-terminated
 */
char **theme_list_available(theme_category_t category);

/**
 * Validate theme definition
 * @param theme Theme to validate
 * @return true if valid, false otherwise
 */
bool theme_validate(const theme_definition_t *theme);

/**
 * Save custom theme to user directory
 * @param theme Theme to save
 * @return true on success, false on failure
 */
bool theme_save_custom(const theme_definition_t *theme);

/**
 * Load custom theme from file
 * @param filename Theme file to load
 * @return pointer to loaded theme, NULL on failure
 */
theme_definition_t *theme_load_custom(const char *filename);

// =============================================================================
// PROMPT TEMPLATE ENGINE
// =============================================================================

/**
 * Initialize template context
 * @return pointer to template context, NULL on failure
 */
template_context_t *template_init_context(void);

/**
 * Free template context
 * @param ctx Template context to free
 */
void template_free_context(template_context_t *ctx);

/**
 * Add template variable
 * @param ctx Template context
 * @param name Variable name
 * @param value Variable value
 * @param color Variable color (optional)
 * @param dynamic Whether value is dynamic
 * @return true on success, false on failure
 */
bool template_add_variable(template_context_t *ctx, const char *name,
                           const char *value, const char *color, bool dynamic);

/**
 * Process template with variables
 * @param template_str Template string with variables
 * @param ctx Template context with variables
 * @param output Output buffer
 * @param output_size Output buffer size
 * @return true on success, false on failure
 */
bool template_process(const char *template_str, template_context_t *ctx,
                      char *output, size_t output_size);

/**
 * Process template with responsive layout for terminal-aware rendering
 * @param template_str Template string with variables
 * @param ctx Template context with variables
 * @param output Output buffer
 * @param output_size Output buffer size
 * @param terminal_width Terminal width for responsive adjustments
 * @param use_colors Whether to use color codes
 * @return true on success, false on failure
 */
bool template_process_responsive(const char *template_str, template_context_t *ctx,
                               char *output, size_t output_size,
                               int terminal_width, bool use_colors);

/**
 * Generate primary prompt using active theme
 * @param output Output buffer for generated prompt
 * @param output_size Output buffer size
 * @return true on success, false on failure
 */
bool theme_generate_primary_prompt(char *output, size_t output_size);

/**
 * Generate secondary prompt (PS2) using active theme
 * @param output Output buffer for generated prompt
 * @param output_size Output buffer size
 * @return true on success, false on failure
 */
bool theme_generate_secondary_prompt(char *output, size_t output_size);

/**
 * Generate right prompt using active theme
 * @param output Output buffer for generated prompt
 * @param output_size Output buffer size
 * @return true on success, false on failure
 */
bool theme_generate_right_prompt(char *output, size_t output_size);

// =============================================================================
// COLOR MANAGEMENT
// =============================================================================

/**
 * Get color code for semantic color name
 * @param color_name Semantic color name (e.g., "primary", "success")
 * @return ANSI color code, empty string if not found
 */
const char *theme_get_color(const char *color_name);

/**
 * Apply color to text
 * @param text Text to colorize
 * @param color_name Semantic color name
 * @param output Output buffer
 * @param output_size Output buffer size
 * @return true on success, false on failure
 */
bool theme_colorize_text(const char *text, const char *color_name, char *output,
                         size_t output_size);

/**
 * Detect terminal color capabilities
 * @return color support level (0=none, 8=basic, 256=extended, 16777216=true)
 */
int theme_detect_color_support(void);

/**
 * Validate color code format
 * @param color_code Color code to validate
 * @return true if valid ANSI color code, false otherwise
 */
bool theme_validate_color_code(const char *color_code);

// =============================================================================
// CORPORATE BRANDING
// =============================================================================

/**
 * Set corporate branding information
 * @param branding Branding configuration
 * @return true on success, false on failure
 */
bool theme_set_branding(const branding_config_t *branding);

/**
 * Get current branding configuration
 * @return pointer to branding config, NULL if not set
 */
const branding_config_t *theme_get_branding(void);

/**
 * Display startup logo/branding
 */
void theme_display_startup_branding(void);

/**
 * Generate branded prompt prefix
 * @param output Output buffer
 * @param output_size Output buffer size
 * @return true on success, false on failure
 */
bool theme_generate_brand_prefix(char *output, size_t output_size);

/**
 * Update dynamic template variables with current terminal state
 * @param ctx Template context to update
 */
void theme_update_dynamic_variables(template_context_t *ctx);

/**
 * Add terminal-specific dynamic variables to template context
 * @param ctx Template context to add variables to
 */
void theme_add_terminal_variables(template_context_t *ctx);

// =============================================================================
// BUILT-IN THEME DEFINITIONS
// =============================================================================

/**
 * Get corporate theme definition
 * @return corporate theme definition
 */
theme_definition_t *theme_get_corporate(void);

/**
 * Get dark theme definition
 * @return dark theme definition
 */
theme_definition_t *theme_get_dark(void);

/**
 * Get light theme definition
 * @return light theme definition
 */
theme_definition_t *theme_get_light(void);

/**
 * Get colorful theme definition
 * @return colorful theme definition
 */
theme_definition_t *theme_get_colorful(void);

/**
 * Get minimal theme definition
 * @return minimal theme definition
 */
theme_definition_t *theme_get_minimal(void);

/**
 * Get classic theme definition
 * @return classic theme definition
 */
theme_definition_t *theme_get_classic(void);

/**
 * Get developer theme definition
 * @return developer theme definition
 */
theme_definition_t *theme_get_developer(void);

// =============================================================================
// CONFIGURATION INTEGRATION
// =============================================================================

/**
 * Load theme configuration from config file
 * @return true on success, false on failure
 */
bool theme_load_config(void);

/**
 * Save theme configuration to config file
 * @return true on success, false on failure
 */
bool theme_save_config(void);

/**
 * Apply theme settings from configuration
 * @return true on success, false on failure
 */
bool theme_apply_config(void);

/**
 * Validate theme configuration
 * @return true if valid, false otherwise
 */
bool theme_validate_config(void);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * Free theme definition and its resources
 * @param theme Theme to free
 */
void theme_free_definition(theme_definition_t *theme);

/**
 * Clone theme definition
 * @param source Source theme to clone
 * @return cloned theme, NULL on failure
 */
theme_definition_t *theme_clone_definition(const theme_definition_t *source);

/**
 * Compare two themes for equality
 * @param theme1 First theme
 * @param theme2 Second theme
 * @return true if themes are equal, false otherwise
 */
bool theme_compare(const theme_definition_t *theme1,
                   const theme_definition_t *theme2);

/**
 * Get theme statistics
 * @param total_themes Output: total number of themes
 * @param builtin_themes Output: number of built-in themes
 * @param custom_themes Output: number of custom themes
 */
void theme_get_statistics(size_t *total_themes, size_t *builtin_themes,
                          size_t *custom_themes);

/**
 * Enable/disable theme debug mode
 * @param enabled Whether to enable debug mode
 */
void theme_set_debug_mode(bool enabled);

/**
 * Get theme system version
 * @return version string
 */
const char *theme_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* THEMES_H */
