#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

// Configuration file paths
#define USER_CONFIG_FILE ".lusushrc"
#define SYSTEM_CONFIG_FILE "/etc/lusush/lusushrc"
#define MAX_CONFIG_LINE 1024
#define MAX_CONFIG_VALUE 512

// Configuration sections
typedef enum {
    CONFIG_SECTION_NONE,
    CONFIG_SECTION_HISTORY,
    CONFIG_SECTION_COMPLETION,
    CONFIG_SECTION_PROMPT,
    CONFIG_SECTION_BEHAVIOR,
    CONFIG_SECTION_ALIASES,
    CONFIG_SECTION_KEYS,
    CONFIG_SECTION_NETWORK,
    CONFIG_SECTION_SCRIPTS,
    CONFIG_SECTION_SHELL,
    CONFIG_SECTION_DISPLAY
} config_section_t;

// LLE History - Arrow key behavior modes
typedef enum {
    LLE_ARROW_MODE_CONTEXT_AWARE,  // Smart: multiline navigation when in
                                   // multiline
    LLE_ARROW_MODE_CLASSIC,        // GNU Readline: always history navigation
    LLE_ARROW_MODE_ALWAYS_HISTORY, // Always history, use Ctrl-P/N only
    LLE_ARROW_MODE_MULTILINE_FIRST // Prioritize multiline navigation
} lle_arrow_key_mode_t;

// LLE History - Storage modes
typedef enum {
    LLE_STORAGE_MODE_LLE_ONLY,       // Store only in LLE format
    LLE_STORAGE_MODE_BASH_ONLY,      // Store only in bash format
    LLE_STORAGE_MODE_DUAL,           // Store in both formats (recommended)
    LLE_STORAGE_MODE_READLINE_COMPAT // Use GNU Readline's storage
} lle_history_storage_mode_t;

// LLE History - Deduplication scope
typedef enum {
    LLE_DEDUP_SCOPE_NONE,    // No deduplication
    LLE_DEDUP_SCOPE_SESSION, // Within current session
    LLE_DEDUP_SCOPE_RECENT,  // Last N entries
    LLE_DEDUP_SCOPE_GLOBAL   // Entire history
} lle_dedup_scope_t;

// LLE History - Deduplication strategy
typedef enum {
    LLE_DEDUP_STRATEGY_IGNORE,        // Reject new duplicates, keep old
    LLE_DEDUP_STRATEGY_KEEP_RECENT,   // Keep newest, mark old as deleted
                                      // (default)
    LLE_DEDUP_STRATEGY_KEEP_FREQUENT, // Keep entry with highest usage count
    LLE_DEDUP_STRATEGY_MERGE,         // Merge forensic metadata, keep existing
    LLE_DEDUP_STRATEGY_KEEP_ALL       // No dedup (track frequency only)
} lle_dedup_strategy_t;

// Configuration context
typedef struct {
    char *user_config_path;
    char *system_config_path;
    bool user_config_exists;
    bool system_config_exists;
    int line_number;
    const char *current_file;
} config_context_t;

// Configuration values structure
typedef struct {
    // History settings
    bool history_enabled;
    int history_size;
    bool history_no_dups;
    bool history_timestamps;
    char *history_file;

    // LLE History Configuration
    // Arrow key behavior
    lle_arrow_key_mode_t lle_arrow_key_mode;
    bool lle_enable_multiline_navigation;

    // History navigation
    bool lle_wrap_history_navigation;
    bool lle_save_line_on_history_nav;

    // Multiline support
    bool lle_preserve_multiline_structure;
    bool lle_enable_multiline_editing;
    bool lle_show_multiline_indicators;

    // Search behavior
    bool lle_enable_interactive_search;
    bool lle_search_fuzzy_matching;
    bool lle_search_case_sensitive;

    // File format and storage
    lle_history_storage_mode_t lle_storage_mode;
    char *lle_history_file;
    bool lle_sync_with_readline;
    bool lle_export_to_bash_history;

    // Advanced features
    bool lle_enable_forensic_tracking;
    bool lle_enable_deduplication;
    lle_dedup_scope_t lle_dedup_scope;
    lle_dedup_strategy_t
        lle_dedup_strategy;    // Dedup strategy (default: KEEP_RECENT)
    bool lle_dedup_navigation; // Skip duplicates during history navigation
                               // (default: true)
    bool lle_dedup_navigation_unique; // Show only unique entries during
                                      // navigation session (default: true)
    bool lle_dedup_unicode_normalize; // Use Unicode NFC normalization for
                                      // comparison (default: true)

    // Performance
    bool lle_enable_history_cache;
    int lle_cache_size;

    // Compatibility mode
    bool lle_readline_compatible_mode;

    // Completion settings
    bool completion_enabled;
    bool fuzzy_completion;
    int completion_threshold;
    bool completion_case_sensitive;
    bool completion_show_all;
    bool hints_enabled;

    // Prompt settings
    bool use_theme_prompt; // Use theme system for prompts (false = respect user
                           // PS1/PS2)
    char *prompt_theme;
    bool git_prompt_enabled;
    int git_cache_timeout;
    char *prompt_format;

    // Theme settings (Phase 3 Target 2)
    char *theme_name;
    bool theme_auto_detect_colors;
    bool theme_fallback_basic;
    char *theme_corporate_company;
    char *theme_corporate_department;
    char *theme_corporate_project;
    char *theme_corporate_environment;
    bool theme_show_company;
    bool theme_show_department;
    bool theme_show_right_prompt;
    bool theme_enable_animations;
    bool theme_enable_icons;
    int theme_color_support_override;

    // Behavior settings
    bool auto_cd;
    bool spell_correction;
    bool confirm_exit;
    int tab_width;
    bool no_word_expand;
    bool multiline_mode;

    // Auto-correction settings
    int autocorrect_max_suggestions;
    int autocorrect_threshold;
    bool autocorrect_interactive;
    bool autocorrect_learn_history;
    bool autocorrect_builtins;
    bool autocorrect_external;
    bool autocorrect_case_sensitive;

    // Color settings
    char *color_scheme;
    bool colors_enabled;

    // Advanced settings
    bool verbose_errors;
    bool debug_mode;

    // Display system settings - v1.3.0: Layered display is now exclusive
    bool display_syntax_highlighting; // Enable syntax highlighting
    bool display_autosuggestions;     // Enable autosuggestions
    bool display_transient_prompt; // Enable transient prompts (Spec 25 Section
                                   // 12)
    bool display_newline_before_prompt;  // Print newline before prompt (visual
                                         // separation)
    bool display_performance_monitoring; // Enable performance monitoring
    int display_optimization_level;      // Optimization level (0-4)

    // Legacy display setting (deprecated)
    bool enhanced_display_mode;

    // Network settings
    bool ssh_completion_enabled;
    bool cloud_discovery_enabled;
    bool cache_ssh_hosts;
    int cache_timeout_minutes;
    bool show_remote_context;
    bool auto_detect_cloud;
    int max_completion_hosts;

    // Script execution control
    bool script_execution;
} config_values_t;

// Global configuration instance
extern config_values_t config;
extern config_context_t config_ctx;

// Core configuration functions
int config_init(void);
int config_load_user(void);
int config_load_system(void);
int config_load_file(const char *path);
int config_save_user(void);
void config_cleanup(void);

// Configuration parsing functions
int config_parse_line(const char *line, int line_num, const char *filename);
int config_parse_section(const char *section_name);
int config_parse_option(const char *key, const char *value);

// Shell option integration functions
bool config_validate_shell_option(const char *value);
void config_set_shell_option(const char *option_name, bool value);
bool config_get_shell_option(const char *option_name);
config_section_t config_get_current_section(void);

// Configuration validation functions
bool config_validate_bool(const char *value);
bool config_validate_int(const char *value);
bool config_validate_string(const char *value);
bool config_validate_color(const char *value);

bool config_validate_float(const char *value);
bool config_validate_path(const char *value);
bool config_validate_shell_option(const char *value);
bool config_validate_display_mode(const char *value);
bool config_validate_optimization_level(const char *value);
bool config_validate_color_scheme(const char *value);
bool config_validate_lle_arrow_mode(const char *value);
bool config_validate_lle_storage_mode(const char *value);
bool config_validate_lle_dedup_scope(const char *value);
bool config_validate_lle_dedup_strategy(const char *value);

// Configuration value setters and getters
int config_set_bool(const char *key, bool value);
int config_set_int(const char *key, int value);
int config_set_string(const char *key, const char *value);
bool config_get_bool(const char *key, bool default_value);
int config_get_int(const char *key, int default_value);
const char *config_get_string(const char *key, const char *default_value);

// Configuration utility functions
void config_set_defaults(void);
void config_apply_settings(void);
int config_create_user_config(void);
char *config_get_user_config_path(void);
char *config_get_system_config_path(void);

// Script execution support for traditional shell compatibility
int config_execute_startup_scripts(void);
int config_execute_login_scripts(void);
int config_execute_logout_scripts(void);
int config_execute_script_file(const char *path);
bool config_should_execute_scripts(void);
void config_set_script_execution(bool enabled);

// Traditional shell script file detection
char *config_get_profile_script_path(void);
char *config_get_login_script_path(void);
char *config_get_rc_script_path(void);
char *config_get_logout_script_path(void);
bool config_script_exists(const char *path);

// Configuration error handling
void config_error(const char *format, ...);
void config_warning(const char *format, ...);
const char *config_get_last_error(void);

// Configuration display functions
void config_show_all(void);
void config_show_section(config_section_t section);
void config_show_option(const char *key);
void config_get_value(const char *key);
void config_set_value(const char *key, const char *value);

// Configuration save functions
int config_save_user(void);
int config_save_file(const char *path);

// Built-in command integration
void builtin_config(int argc, char **argv);

// Configuration file template
extern const char *CONFIG_FILE_TEMPLATE;

#endif // CONFIG_H
