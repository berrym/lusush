#include "../include/config.h"

#include "../include/alias.h"
#include "../include/autocorrect.h"
#include "../include/completion.h"
#include "../include/errors.h"
#include "../include/history.h"
#include "../include/readline_integration.h"
#include "../include/lusush.h"
#include "../include/prompt.h"
#include "../include/strings.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// Global configuration instance
config_values_t config;
config_context_t config_ctx;

// Current configuration section
static config_section_t current_section = CONFIG_SECTION_NONE;

// Error handling
static char last_error[256] = "";

// Configuration option definitions
static config_option_t config_options[] = {
    // History settings
    {                "history.enabled",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_HISTORY,
     &config.history_enabled,"Enable command history",config_validate_bool                                                                                  },
    {                   "history.size",    CONFIG_TYPE_INT,    CONFIG_SECTION_HISTORY,
     &config.history_size,                       "Maximum history entries",          config_validate_int       },
    {               "history.no_dups",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_HISTORY,
     &config.history_no_dups,              "Remove duplicate history entries",
     config_validate_bool                                                                                      },
    {            "history.timestamps",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_HISTORY,
     &config.history_timestamps,                     "Add timestamps to history",
     config_validate_bool                                                                                      },
    {                  "history.file", CONFIG_TYPE_STRING,    CONFIG_SECTION_HISTORY,
     &config.history_file,                             "History file path",       config_validate_string       },

    // LLE History Configuration
    {"lle.arrow_key_mode", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_arrow_key_mode, "Arrow key behavior mode", config_validate_lle_arrow_mode},
    {"lle.enable_multiline_navigation", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_multiline_navigation, "Enable vertical cursor navigation in multiline", config_validate_bool},
    {"lle.wrap_history_navigation", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_wrap_history_navigation, "Wrap around at history boundaries", config_validate_bool},
    {"lle.save_line_on_history_nav", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_save_line_on_history_nav, "Save current line when navigating history", config_validate_bool},
    {"lle.preserve_multiline_structure", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_preserve_multiline_structure, "Preserve multiline structure in history", config_validate_bool},
    {"lle.enable_multiline_editing", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_multiline_editing, "Enable editing of recalled multiline commands", config_validate_bool},
    {"lle.show_multiline_indicators", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_show_multiline_indicators, "Show visual indicators for multiline", config_validate_bool},
    {"lle.enable_interactive_search", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_interactive_search, "Enable Ctrl-R interactive search", config_validate_bool},
    {"lle.search_fuzzy_matching", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_search_fuzzy_matching, "Use fuzzy matching in search", config_validate_bool},
    {"lle.search_case_sensitive", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_search_case_sensitive, "Case sensitive history search", config_validate_bool},
    {"lle.storage_mode", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_storage_mode, "History storage mode", config_validate_lle_storage_mode},
    {"lle.history_file", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_history_file, "LLE history file path", config_validate_string},
    {"lle.sync_with_readline", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_sync_with_readline, "Sync LLE history with GNU Readline", config_validate_bool},
    {"lle.export_to_bash_history", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_export_to_bash_history, "Export to .bash_history format", config_validate_bool},
    {"lle.enable_forensic_tracking", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_forensic_tracking, "Track metadata (timestamps, exit codes, cwd)", config_validate_bool},
    {"lle.enable_deduplication", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_deduplication, "Enable history deduplication", config_validate_bool},
    {"lle.dedup_scope", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_dedup_scope, "Deduplication scope", config_validate_lle_dedup_scope},
    {"lle.enable_history_cache", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_history_cache, "Enable history caching for performance", config_validate_bool},
    {"lle.cache_size", CONFIG_TYPE_INT, CONFIG_SECTION_HISTORY,
     &config.lle_cache_size, "History cache size", config_validate_int},
    {"lle.readline_compatible_mode", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_readline_compatible_mode, "GNU Readline compatibility mode", config_validate_bool},

    // Completion settings
    {            "completion.enabled",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.completion_enabled,                         "Enable tab completion",         config_validate_bool },
    {                "completion.fuzzy",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.fuzzy_completion,           "Enable fuzzy matching in completion",
     config_validate_bool                                                                                      },
    {            "completion.threshold",    CONFIG_TYPE_INT, CONFIG_SECTION_COMPLETION,
     &config.completion_threshold,              "Fuzzy matching threshold (0-100)",
     config_validate_int                                                                                       },
    {       "completion.case_sensitive",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.completion_case_sensitive,                     "Case sensitive completion",
     config_validate_bool                                                                                      },
    {             "completion.show_all",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.completion_show_all,                          "Show all completions",         config_validate_bool},
    {                 "completion.hints",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.hints_enabled,                            "Enable input hints",         config_validate_bool      },

    // Prompt settings
    {                   "prompt.style", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.prompt_style, "Prompt style (normal, color, fancy, pro, git)",
     config_validate_prompt_style                                                                              },
    {                   "prompt.theme", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.prompt_theme,                            "Prompt color theme",       config_validate_string       },
    {              "prompt.git_enabled",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.git_prompt_enabled,                      "Enable git-aware prompts",
     config_validate_bool                                                                                      },
    {         "prompt.git_cache_timeout",    CONFIG_TYPE_INT,     CONFIG_SECTION_PROMPT,
     &config.git_cache_timeout,           "Git status cache timeout in seconds",
     config_validate_int                                                                                       },
    {                  "prompt.format", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.prompt_format,                   "Custom prompt format string",
     config_validate_string                                                                                    },

    // Theme settings (Phase 3 Target 2)
    {                     "prompt.theme_name", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.theme_name,                             "Active theme name",       config_validate_string         },
    {        "prompt.theme_auto_detect_colors",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.theme_auto_detect_colors,            "Auto-detect terminal color support",
     config_validate_bool                                                                                      },
    {            "prompt.theme_fallback_basic",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.theme_fallback_basic,            "Fallback to basic colors if needed",
     config_validate_bool                                                                                      },
    {         "prompt.theme_corporate_company", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.theme_corporate_company,                        "Corporate company name",
     config_validate_string                                                                                    },
    {      "prompt.theme_corporate_department", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.theme_corporate_department,                     "Corporate department name",
     config_validate_string                                                                                    },
    {         "prompt.theme_corporate_project", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.theme_corporate_project,                        "Corporate project name",
     config_validate_string                                                                                    },
    {     "prompt.theme_corporate_environment", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.theme_corporate_environment,                    "Corporate environment name",
     config_validate_string                                                                                    },
    {              "prompt.theme_show_company",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.theme_show_company,                   "Show company name in prompt",
     config_validate_bool                                                                                      },
    {           "prompt.theme_show_department",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.theme_show_department,                "Show department name in prompt",
     config_validate_bool                                                                                      },
    {         "prompt.theme_show_right_prompt",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.theme_show_right_prompt,                      "Enable right-side prompt",
     config_validate_bool                                                                                      },

    {         "prompt.theme_enable_animations",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.theme_enable_animations,                      "Enable prompt animations",
     config_validate_bool                                                                                      },
    {              "prompt.theme_enable_icons",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.theme_enable_icons,                          "Enable Unicode icons",         config_validate_bool },
    {    "prompt.theme_color_support_override",    CONFIG_TYPE_INT,     CONFIG_SECTION_PROMPT,
     &config.theme_color_support_override,
     "Override color support detection (0/8/256/16777216)",          config_validate_int                       },

    // Behavior settings
    {                        "behavior.auto_cd",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,   &config.auto_cd,
     "Auto-cd to directories",         config_validate_bool                                                    },
    {               "behavior.spell_correction",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.spell_correction,               "Enable command spell correction",
     config_validate_bool                                                                                      },
    {    "behavior.autocorrect_max_suggestions",    CONFIG_TYPE_INT,   CONFIG_SECTION_BEHAVIOR,
     &config.autocorrect_max_suggestions,
     "Maximum auto-correction suggestions (1-5)",          config_validate_int                                 },
    {          "behavior.autocorrect_threshold",    CONFIG_TYPE_INT,   CONFIG_SECTION_BEHAVIOR,
     &config.autocorrect_threshold,
     "Auto-correction similarity threshold (0-100)",          config_validate_int                              },
    {        "behavior.autocorrect_interactive",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.autocorrect_interactive,           "Show interactive correction prompts",
     config_validate_bool                                                                                      },
    {      "behavior.autocorrect_learn_history",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.autocorrect_learn_history,                   "Learn commands from history",
     config_validate_bool                                                                                      },
    {           "behavior.autocorrect_builtins",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.autocorrect_builtins,                   "Suggest builtin corrections",
     config_validate_bool                                                                                      },
    {           "behavior.autocorrect_external",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.autocorrect_external,          "Suggest external command corrections",
     config_validate_bool                                                                                      },
    {     "behavior.autocorrect_case_sensitive",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.autocorrect_case_sensitive,                "Case-sensitive auto-correction",
     config_validate_bool                                                                                      },
    {                   "behavior.confirm_exit",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.confirm_exit,                        "Confirm before exiting",         config_validate_bool       },
    {                      "behavior.tab_width",    CONFIG_TYPE_INT,   CONFIG_SECTION_BEHAVIOR, &config.tab_width,
     "Tab width for display",          config_validate_int                                                     },
    {                 "behavior.no_word_expand",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.no_word_expand,           "Disable word expansion and globbing",
     config_validate_bool                                                                                      },
    {                 "behavior.multiline_mode",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.multiline_mode,           "Enable multiline editing mode",
     config_validate_bool                                                                                      },

    // Color settings
    {                   "behavior.color_scheme", CONFIG_TYPE_STRING,   CONFIG_SECTION_BEHAVIOR,
     &config.color_scheme,                             "Color scheme name", config_validate_color_scheme       },
    {                 "behavior.colors_enabled",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.colors_enabled,                           "Enable color output",         config_validate_bool     },

    // Advanced settings
    {                 "behavior.verbose_errors",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.verbose_errors,                   "Show verbose error messages",
     config_validate_bool                                                                                      },
    {                     "behavior.debug_mode",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.debug_mode,                             "Enable debug mode",         config_validate_bool         },

    // Network settings
    {         "network.ssh_completion",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_NETWORK,
     &config.ssh_completion_enabled,                    "Enable SSH host completion",
     config_validate_bool                                                                                      },
    {        "network.cloud_discovery",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_NETWORK,
     &config.cloud_discovery_enabled,                   "Enable cloud host discovery",
     config_validate_bool                                                                                      },
    {          "network.cache_ssh_hosts",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_NETWORK,
     &config.cache_ssh_hosts,               "Cache SSH hosts for performance",
     config_validate_bool                                                                                      },
    {      "network.cache_timeout_minutes",    CONFIG_TYPE_INT,    CONFIG_SECTION_NETWORK,
     &config.cache_timeout_minutes,             "SSH host cache timeout in minutes",
     config_validate_int                                                                                       },
    {        "network.show_remote_context",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_NETWORK,
     &config.show_remote_context,                 "Show remote context in prompt",
     config_validate_bool                                                                                      },
    {          "network.auto_detect_cloud",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_NETWORK,
     &config.auto_detect_cloud,                 "Auto-detect cloud environment",
     config_validate_bool                                                                                      },
    {       "network.max_completion_hosts",    CONFIG_TYPE_INT,    CONFIG_SECTION_NETWORK,
     &config.max_completion_hosts,           "Maximum hosts to show in completion",
     config_validate_int                                                                                       },

    // Display system settings
    // v1.3.0: Layered display is now the exclusive system - no configuration needed
    // display.system_mode and display.layered_display options removed
    {    "display.performance_monitoring",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_DISPLAY,
     &config.display_performance_monitoring, "Enable display performance monitoring",
     config_validate_bool                                                                                      },
    {    "display.optimization_level",       CONFIG_TYPE_INT,    CONFIG_SECTION_DISPLAY,
     &config.display_optimization_level,     "Display optimization level (0-4)",
     config_validate_optimization_level                                                                        },

    // v1.3.0: Legacy enhanced display mode option removed
    // behavior.enhanced_display_mode option removed

    // Script execution control
    {               "scripts.execution",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SCRIPTS,
     &config.script_execution,                       "Enable script execution",         config_validate_bool   },

    // Line editor selection
    {                   "editor.use_lle",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_BEHAVIOR,
     &config.use_lle,                  "Use LLE instead of GNU readline (requires restart)",   config_validate_bool   },

    // Shell options integration - all 24 POSIX options with shell.* namespace
    // These map directly to existing shell_opts flags for perfect compatibility
    {            "shell.errexit",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Exit on command failure (set -e)",    config_validate_shell_option },
    {            "shell.xtrace",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Trace command execution (set -x)",    config_validate_shell_option },
    {            "shell.noexec",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Syntax check only (set -n)",          config_validate_shell_option },
    {            "shell.nounset",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Error on unset variables (set -u)",   config_validate_shell_option },
    {            "shell.verbose",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Print input lines (set -v)",          config_validate_shell_option },
    {            "shell.noglob",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Disable pathname expansion (set -f)", config_validate_shell_option },
    {            "shell.hashall",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Command hashing (set -h)",            config_validate_shell_option },
    {            "shell.monitor",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Job control (set -m)",                config_validate_shell_option },
    {            "shell.allexport",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Auto export variables (set -a)",      config_validate_shell_option },
    {            "shell.noclobber",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Prevent file overwrite (set -C)",     config_validate_shell_option },
    {            "shell.onecmd",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Exit after one command (set -t)",     config_validate_shell_option },
    {            "shell.notify",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Async job notification (set -b)",     config_validate_shell_option },
    {            "shell.ignoreeof",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Prevent exit on EOF (set -o ignoreeof)", config_validate_shell_option },
    {            "shell.nolog",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Prevent function history logging (set -o nolog)", config_validate_shell_option },
    {            "shell.emacs",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Emacs-style editing (set -o emacs)",  config_validate_shell_option },
    {            "shell.vi",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Vi-style editing (set -o vi)",        config_validate_shell_option },
    {            "shell.posix",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Strict POSIX compliance (set -o posix)", config_validate_shell_option },
    {            "shell.pipefail",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Pipeline failure detection (set -o pipefail)", config_validate_shell_option },
    {            "shell.histexpand",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "History expansion (set -o histexpand)", config_validate_shell_option },
    {            "shell.history",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Command history recording (set -o history)", config_validate_shell_option },
    {            "shell.interactive-comments",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Interactive comments (set -o interactive-comments)", config_validate_shell_option },
    {            "shell.physical",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Physical directory paths (set -o physical)", config_validate_shell_option },
    {            "shell.privileged",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_SHELL,
     NULL,                                   "Restricted shell security (set -o privileged)", config_validate_shell_option },
};

static const int num_config_options =
    sizeof(config_options) / sizeof(config_option_t);

/**
 * Handle legacy configuration keys that were removed or renamed.
 * This prevents warnings for deprecated configuration options in existing .lusushrc files.
 *
 * @param key   The legacy configuration key
 * @param value The value being set
 * @return true if key was handled, false if unknown
 */
static bool config_handle_legacy_key(const char *key, const char *value __attribute__((unused))) {
    if (!key) {
        return false;
    }

    // Legacy display configuration keys (removed for v1.3.0 stability)
    if (strcmp(key, "display.syntax_highlighting") == 0) {
        // Legacy key - syntax highlighting requires LLE for safe implementation
        // Silently ignore to maintain compatibility with existing .lusushrc files
        return true;
    }
    
    if (strcmp(key, "display.autosuggestions") == 0) {
        // Legacy key - autosuggestions require LLE for safe implementation
        // Silently ignore to maintain compatibility with existing .lusushrc files
        return true;
    }
    
    if (strcmp(key, "behavior.enhanced_display_mode") == 0) {
        // Legacy key - replaced by layered display system
        // Silently ignore to maintain compatibility with existing .lusushrc files
        return true;
    }
    
    // Handle legacy display.* configuration keys
    if (strncmp(key, "display.", 8) == 0) {
        // Unknown display.* keys - likely legacy, handle gracefully
        const char* display_key = key + 8;  // Skip "display." prefix
        
        if (strcmp(display_key, "system_mode") == 0 ||
            strcmp(display_key, "layered_display") == 0) {
            // Legacy display mode options - layered display is now exclusive
            // Silently ignore to maintain compatibility with existing .lusushrc files
            return true;
        }
        
        if (strcmp(display_key, "performance_monitoring") == 0 ||
            strcmp(display_key, "optimization_level") == 0) {
            // These are valid current keys that should be handled by config system
            // Return false to let normal processing handle them
            return false;
        }
        
        // Other display.* keys are likely legacy - handle gracefully
        return true;
    }

    return false;
}

// Legacy option name mapping for backward compatibility
typedef struct {
    const char *old_name;
    const char *new_name;
} legacy_option_mapping_t;

static legacy_option_mapping_t legacy_mappings[] = {
    // History options
    {"history_enabled", "history.enabled"},
    {"history_size", "history.size"},
    {"history_no_dups", "history.no_dups"},
    {"history_timestamps", "history.timestamps"},
    {"history_file", "history.file"},
    
    // Completion options
    {"completion_enabled", "completion.enabled"},
    {"fuzzy_completion", "completion.fuzzy"},
    {"completion_threshold", "completion.threshold"},
    {"completion_case_sensitive", "completion.case_sensitive"},
    {"completion_show_all", "completion.show_all"},
    {"hints_enabled", "completion.hints"},
    
    // Prompt options
    {"prompt_style", "prompt.style"},
    {"prompt_theme", "prompt.theme"},
    {"git_prompt_enabled", "prompt.git_enabled"},
    {"git_cache_timeout", "prompt.git_cache_timeout"},
    {"prompt_format", "prompt.format"},
    {"theme_name", "prompt.theme_name"},
    {"theme_auto_detect_colors", "prompt.theme_auto_detect_colors"},
    {"theme_fallback_basic", "prompt.theme_fallback_basic"},
    {"theme_corporate_company", "prompt.theme_corporate_company"},
    {"theme_corporate_department", "prompt.theme_corporate_department"},
    {"theme_corporate_project", "prompt.theme_corporate_project"},
    {"theme_corporate_environment", "prompt.theme_corporate_environment"},
    {"theme_show_company", "prompt.theme_show_company"},
    {"theme_show_department", "prompt.theme_show_department"},
    {"theme_show_right_prompt", "prompt.theme_show_right_prompt"},
    {"theme_enable_animations", "prompt.theme_enable_animations"},
    {"theme_enable_icons", "prompt.theme_enable_icons"},
    {"theme_color_support_override", "prompt.theme_color_support_override"},
    
    // Behavior options
    {"auto_cd", "behavior.auto_cd"},
    {"spell_correction", "behavior.spell_correction"},
    {"autocorrect_max_suggestions", "behavior.autocorrect_max_suggestions"},
    {"autocorrect_threshold", "behavior.autocorrect_threshold"},
    {"autocorrect_interactive", "behavior.autocorrect_interactive"},
    {"autocorrect_learn_history", "behavior.autocorrect_learn_history"},
    {"autocorrect_builtins", "behavior.autocorrect_builtins"},
    {"autocorrect_external", "behavior.autocorrect_external"},
    {"autocorrect_case_sensitive", "behavior.autocorrect_case_sensitive"},
    {"confirm_exit", "behavior.confirm_exit"},
    {"tab_width", "behavior.tab_width"},
    {"no_word_expand", "behavior.no_word_expand"},
    {"multiline_mode", "behavior.multiline_mode"},
    {"color_scheme", "behavior.color_scheme"},
    {"colors_enabled", "behavior.colors_enabled"},
    {"verbose_errors", "behavior.verbose_errors"},
    {"debug_mode", "behavior.debug_mode"},
    {"display_performance", "display.performance_monitoring"},
    {"display_optimization", "display.optimization_level"},
    
    // Network options
    {"ssh_completion_enabled", "network.ssh_completion"},
    {"cloud_discovery_enabled", "network.cloud_discovery"},
    {"cache_ssh_hosts", "network.cache_ssh_hosts"},
    {"cache_timeout_minutes", "network.cache_timeout_minutes"},
    {"show_remote_context", "network.show_remote_context"},
    {"auto_detect_cloud", "network.auto_detect_cloud"},
    {"max_completion_hosts", "network.max_completion_hosts"},
    
    // Scripts options
    {"script_execution", "scripts.execution"},
    
    {NULL, NULL}
};

// Find new name for legacy option
static const char *find_new_name_for_legacy(const char *old_name) {
    for (int i = 0; legacy_mappings[i].old_name; i++) {
        if (strcmp(legacy_mappings[i].old_name, old_name) == 0) {
            return legacy_mappings[i].new_name;
        }
    }
    return NULL;
}

// Shell option integration functions - use existing POSIX infrastructure
/**
 * config_validate_shell_option:
 *      Validate shell option values (true/false).
 */
bool config_validate_shell_option(const char *value) {
    return (strcmp(value, "true") == 0 || strcmp(value, "false") == 0 ||
            strcmp(value, "1") == 0 || strcmp(value, "0") == 0 ||
            strcmp(value, "on") == 0 || strcmp(value, "off") == 0);
}

/**
 * config_set_shell_option:
 *      Set shell option using existing POSIX infrastructure.
 *      Maps config system calls to existing set/unset option logic.
 */
void config_set_shell_option(const char *option_name, bool value) {
    // Remove "shell." prefix to get the actual option name
    const char *opt_name = option_name + 6; // Skip "shell."
    
    // Use existing POSIX option infrastructure - same logic as builtin_set
    extern shell_options_t shell_opts; // From lusush.h
    
    // Map to existing shell option flags using the same names as builtin_set
    if (strcmp(opt_name, "errexit") == 0) {
        shell_opts.exit_on_error = value;
    } else if (strcmp(opt_name, "xtrace") == 0) {
        shell_opts.trace_execution = value;
    } else if (strcmp(opt_name, "noexec") == 0) {
        shell_opts.syntax_check = value;
    } else if (strcmp(opt_name, "nounset") == 0) {
        shell_opts.unset_error = value;
    } else if (strcmp(opt_name, "verbose") == 0) {
        shell_opts.verbose = value;
    } else if (strcmp(opt_name, "noglob") == 0) {
        shell_opts.no_globbing = value;
    } else if (strcmp(opt_name, "hashall") == 0) {
        shell_opts.hash_commands = value;
    } else if (strcmp(opt_name, "monitor") == 0) {
        shell_opts.job_control = value;
    } else if (strcmp(opt_name, "allexport") == 0) {
        shell_opts.allexport = value;
    } else if (strcmp(opt_name, "noclobber") == 0) {
        shell_opts.noclobber = value;
    } else if (strcmp(opt_name, "onecmd") == 0) {
        shell_opts.onecmd = value;
    } else if (strcmp(opt_name, "notify") == 0) {
        shell_opts.notify = value;
    } else if (strcmp(opt_name, "ignoreeof") == 0) {
        shell_opts.ignoreeof = value;
    } else if (strcmp(opt_name, "nolog") == 0) {
        shell_opts.nolog = value;
    } else if (strcmp(opt_name, "emacs") == 0) {
        shell_opts.emacs_mode = value;
        if (value) {
            shell_opts.vi_mode = false; // Mutually exclusive
            // Update readline editing mode if function exists
            extern void lusush_update_editing_mode(void);
            lusush_update_editing_mode();
        }
    } else if (strcmp(opt_name, "vi") == 0) {
        shell_opts.vi_mode = value;
        if (value) {
            shell_opts.emacs_mode = false; // Mutually exclusive
            // Update readline editing mode if function exists
            extern void lusush_update_editing_mode(void);
            lusush_update_editing_mode();
        }
    } else if (strcmp(opt_name, "posix") == 0) {
        shell_opts.posix_mode = value;
    } else if (strcmp(opt_name, "pipefail") == 0) {
        shell_opts.pipefail_mode = value;
    } else if (strcmp(opt_name, "histexpand") == 0) {
        shell_opts.histexpand_mode = value;
    } else if (strcmp(opt_name, "history") == 0) {
        shell_opts.history_mode = value;
    } else if (strcmp(opt_name, "interactive-comments") == 0) {
        shell_opts.interactive_comments_mode = value;
    } else if (strcmp(opt_name, "physical") == 0) {
        shell_opts.physical_mode = value;
    } else if (strcmp(opt_name, "privileged") == 0) {
        shell_opts.privileged_mode = value;
    }
}

/**
 * config_get_shell_option:
 *      Get shell option value using existing POSIX infrastructure.
 */
bool config_get_shell_option(const char *option_name) {
    // Remove "shell." prefix to get the actual option name
    const char *opt_name = option_name + 6; // Skip "shell."
    
    // Use existing shell option flags - same logic as builtin_set status display
    extern shell_options_t shell_opts; // From lusush.h
    
    if (strcmp(opt_name, "errexit") == 0) {
        return shell_opts.exit_on_error;
    } else if (strcmp(opt_name, "xtrace") == 0) {
        return shell_opts.trace_execution;
    } else if (strcmp(opt_name, "noexec") == 0) {
        return shell_opts.syntax_check;
    } else if (strcmp(opt_name, "nounset") == 0) {
        return shell_opts.unset_error;
    } else if (strcmp(opt_name, "verbose") == 0) {
        return shell_opts.verbose;
    } else if (strcmp(opt_name, "noglob") == 0) {
        return shell_opts.no_globbing;
    } else if (strcmp(opt_name, "hashall") == 0) {
        return shell_opts.hash_commands;
    } else if (strcmp(opt_name, "monitor") == 0) {
        return shell_opts.job_control;
    } else if (strcmp(opt_name, "allexport") == 0) {
        return shell_opts.allexport;
    } else if (strcmp(opt_name, "noclobber") == 0) {
        return shell_opts.noclobber;
    } else if (strcmp(opt_name, "onecmd") == 0) {
        return shell_opts.onecmd;
    } else if (strcmp(opt_name, "notify") == 0) {
        return shell_opts.notify;
    } else if (strcmp(opt_name, "ignoreeof") == 0) {
        return shell_opts.ignoreeof;
    } else if (strcmp(opt_name, "nolog") == 0) {
        return shell_opts.nolog;
    } else if (strcmp(opt_name, "emacs") == 0) {
        return shell_opts.emacs_mode;
    } else if (strcmp(opt_name, "vi") == 0) {
        return shell_opts.vi_mode;
    } else if (strcmp(opt_name, "posix") == 0) {
        return shell_opts.posix_mode;
    } else if (strcmp(opt_name, "pipefail") == 0) {
        return shell_opts.pipefail_mode;
    } else if (strcmp(opt_name, "histexpand") == 0) {
        return shell_opts.histexpand_mode;
    } else if (strcmp(opt_name, "history") == 0) {
        return shell_opts.history_mode;
    } else if (strcmp(opt_name, "interactive-comments") == 0) {
        return shell_opts.interactive_comments_mode;
    } else if (strcmp(opt_name, "physical") == 0) {
        return shell_opts.physical_mode;
    } else if (strcmp(opt_name, "privileged") == 0) {
        return shell_opts.privileged_mode;
    }
    
    return false; // Unknown option
}

// Script execution support for traditional shell compatibility

// Traditional shell script file paths
#define PROFILE_SCRIPT_FILE ".profile"
#define LOGIN_SCRIPT_FILE ".lusush_login"
#define RC_SCRIPT_FILE ".lusushrc.sh"
#define LOGOUT_SCRIPT_FILE ".lusush_logout"

/**
 * config_should_execute_scripts:
 *      Check if script execution is enabled.
 */
bool config_should_execute_scripts(void) { return config.script_execution; }

/**
 * config_set_script_execution:
 *      Enable or disable script execution.
 */
void config_set_script_execution(bool enabled) {
    config.script_execution = enabled;
}

/**
 * config_get_profile_script_path:
 *      Get the path to the profile script file.
 */
char *config_get_profile_script_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char *path = malloc(strlen(home) + strlen(PROFILE_SCRIPT_FILE) + 2);
    if (!path) {
        return NULL;
    }

    sprintf(path, "%s/%s", home, PROFILE_SCRIPT_FILE);
    return path;
}

/**
 * config_get_login_script_path:
 *      Get the path to the login script file.
 */
char *config_get_login_script_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char *path = malloc(strlen(home) + strlen(LOGIN_SCRIPT_FILE) + 2);
    if (!path) {
        return NULL;
    }

    sprintf(path, "%s/%s", home, LOGIN_SCRIPT_FILE);
    return path;
}

/**
 * config_get_rc_script_path:
 *      Get the path to the RC script file.
 */
char *config_get_rc_script_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char *path = malloc(strlen(home) + strlen(RC_SCRIPT_FILE) + 2);
    if (!path) {
        return NULL;
    }

    sprintf(path, "%s/%s", home, RC_SCRIPT_FILE);
    return path;
}

/**
 * config_get_logout_script_path:
 *      Get the path to the logout script file.
 */
char *config_get_logout_script_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char *path = malloc(strlen(home) + strlen(LOGOUT_SCRIPT_FILE) + 2);
    if (!path) {
        return NULL;
    }

    sprintf(path, "%s/%s", home, LOGOUT_SCRIPT_FILE);
    return path;
}

/**
 * config_script_exists:
 *      Check if a script file exists and is readable.
 */
bool config_script_exists(const char *path) {
    if (!path) {
        return false;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }

    return S_ISREG(st.st_mode) && (access(path, R_OK) == 0);
}

/**
 * config_execute_script_file:
 *      Execute a shell script file.
 */
int config_execute_script_file(const char *path) {
    if (!path || !config_script_exists(path)) {
        return -1;
    }

    // Use the same approach as bin_source builtin
    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int result = 0;

    while ((read = getline(&line, &len, file)) != -1) {
        // Remove newline
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // Parse and execute the line
        extern int parse_and_execute(const char *command);
        int line_result = parse_and_execute(line);
        if (line_result != 0) {
            result = line_result;
        }
    }

    free(line);
    fclose(file);
    return result;
}

/**
 * config_execute_startup_scripts:
 *      Execute startup scripts for interactive shells.
 */
int config_execute_startup_scripts(void) {
    if (!config_should_execute_scripts()) {
        return 0;
    }

    int result = 0;

    // Execute .lusushrc.sh if it exists (lusush-specific RC script)
    char *rc_path = config_get_rc_script_path();
    if (rc_path && config_script_exists(rc_path)) {
        if (config_execute_script_file(rc_path) != 0) {
            result = -1;
        }
    }
    free(rc_path);

    return result;
}

/**
 * config_execute_login_scripts:
 *      Execute login scripts for login shells.
 */
int config_execute_login_scripts(void) {
    if (!config_should_execute_scripts()) {
        return 0;
    }

    int result = 0;

    // Execute .profile if it exists (POSIX standard)
    char *profile_path = config_get_profile_script_path();
    if (profile_path && config_script_exists(profile_path)) {
        if (config_execute_script_file(profile_path) != 0) {
            result = -1;
        }
    }
    free(profile_path);

    // Execute .lusush_login if it exists (lusush-specific login script)
    char *login_path = config_get_login_script_path();
    if (login_path && config_script_exists(login_path)) {
        if (config_execute_script_file(login_path) != 0) {
            result = -1;
        }
    }
    free(login_path);

    return result;
}

/**
 * config_execute_logout_scripts:
 *      Execute logout scripts when shell exits.
 */
int config_execute_logout_scripts(void) {
    if (!config_should_execute_scripts()) {
        return 0;
    }

    int result = 0;

    // Execute .lusush_logout if it exists
    char *logout_path = config_get_logout_script_path();
    if (logout_path && config_script_exists(logout_path)) {
        if (config_execute_script_file(logout_path) != 0) {
            result = -1;
        }
    }
    free(logout_path);

    return result;
}

// Configuration file template
const char *CONFIG_FILE_TEMPLATE =
    "# LUSUSH Configuration File\n"
    "# This file configures the behavior of the lusush shell\n"
    "# Lines starting with # are comments\n"
    "\n"
    "[history]\n"
    "history_enabled = true\n"
    "history_size = 1000\n"
    "history_no_dups = true\n"
    "history_timestamps = false\n"
    "# history_file = ~/.lusush_history\n"
    "\n"
    "# LLE History Configuration\n"
    "# Arrow key behavior: context-aware, classic, always-history, multiline-first\n"
    "lle_arrow_key_mode = context-aware\n"
    "lle_enable_multiline_navigation = true\n"
    "lle_wrap_history_navigation = false\n"
    "lle_save_line_on_history_nav = true\n"
    "lle_preserve_multiline_structure = true\n"
    "lle_enable_multiline_editing = true\n"
    "lle_show_multiline_indicators = true\n"
    "lle_enable_interactive_search = true\n"
    "lle_search_fuzzy_matching = false\n"
    "lle_search_case_sensitive = false\n"
    "# Storage mode: lle-only, bash-only, dual, readline-compat\n"
    "lle_storage_mode = dual\n"
    "# lle_history_file = ~/.lle_history\n"
    "lle_sync_with_readline = true\n"
    "lle_export_to_bash_history = true\n"
    "lle_enable_forensic_tracking = true\n"
    "lle_enable_deduplication = true\n"
    "# Dedup scope: none, session, recent, global\n"
    "lle_dedup_scope = session\n"
    "lle_enable_history_cache = true\n"
    "lle_cache_size = 100\n"
    "lle_readline_compatible_mode = false\n"
    "\n"
    "[completion]\n"
    "completion_enabled = true\n"
    "fuzzy_completion = true\n"
    "completion_threshold = 60\n"
    "completion_case_sensitive = false\n"
    "completion_show_all = false\n"
    "hints_enabled = false\n"
    "\n"
    "[prompt]\n"
    "prompt_style = git\n"
    "prompt_theme = default\n"
    "git_prompt_enabled = true\n"
    "git_cache_timeout = 5\n"
    "# prompt_format = \"%u@%h in %d%g %% \"\n"
    "theme_name = corporate\n"
    "theme_auto_detect_colors = true\n"
    "theme_fallback_basic = true\n"
    "# theme_corporate_company = \"\"\n"
    "# theme_corporate_department = \"\"\n"
    "# theme_corporate_project = \"\"\n"
    "# theme_corporate_environment = \"\"\n"
    "theme_show_company = false\n"
    "theme_show_department = false\n"
    "theme_show_right_prompt = true\n"

    "theme_enable_animations = false\n"
    "theme_enable_icons = false\n"
    "theme_color_support_override = 0\n"
    "\n"
    "[behavior]\n"
    "auto_cd = false\n"
    "spell_correction = false\n"
    "autocorrect_max_suggestions = 3\n"
    "autocorrect_threshold = 40\n"
    "autocorrect_interactive = true\n"
    "autocorrect_learn_history = true\n"
    "autocorrect_builtins = true\n"
    "autocorrect_external = true\n"
    "autocorrect_case_sensitive = false\n"
    "confirm_exit = false\n"
    "tab_width = 4\n"
    "color_scheme = default\n"
    "colors_enabled = true\n"
    "verbose_errors = false\n"
    "debug_mode = false\n"
    "\n"
    "[aliases]\n"
    "# Define custom aliases here\n"
    "# ll = ls -l\n"
    "# la = ls -la\n"
    "\n"
    "[keys]\n"
    "# Custom key bindings (future feature)\n"
    "# ctrl-r = reverse-search\n";

/**
 * config_init:
 *      Initialize the configuration system.
 */
int config_init(void) {
    // Set default values
    config_set_defaults();

    // Initialize context
    memset(&config_ctx, 0, sizeof(config_ctx));

    // Try to get config paths, but don't fail if we can't
    config_ctx.user_config_path = config_get_user_config_path();
    config_ctx.system_config_path = config_get_system_config_path();

    // Only proceed if we have valid paths
    if (config_ctx.user_config_path && config_ctx.system_config_path) {
        // Check if config files exist
        struct stat st;
        config_ctx.user_config_exists =
            (stat(config_ctx.user_config_path, &st) == 0);
        config_ctx.system_config_exists =
            (stat(config_ctx.system_config_path, &st) == 0);

        // Load system config first, then user config (ignore errors)
        if (config_ctx.system_config_exists) {
            config_load_system();
        }

        if (config_ctx.user_config_exists) {
            config_load_user();
        }
    }

    // Apply loaded settings (always safe to call with defaults)
    config_apply_settings();
    
    // AFTER loading config files, check environment variable override
    // This allows LLE_ENABLED=1 to work even if config file says false
    const char *lle_env = getenv("LLE_ENABLED");
    if (lle_env && strcmp(lle_env, "1") == 0) {
        config.use_lle = true;
    }

    return 0;
}

/**
 * config_set_defaults:
 *      Set default configuration values.
 */
void config_set_defaults(void) {
    // History defaults
    config.history_enabled = true;
    config.history_size = 1000;
    config.history_no_dups = true;
    config.history_timestamps = false;
    config.history_file = NULL;

    // LLE History defaults
    config.lle_arrow_key_mode = LLE_ARROW_MODE_CONTEXT_AWARE;
    config.lle_enable_multiline_navigation = true;
    config.lle_wrap_history_navigation = false;
    config.lle_save_line_on_history_nav = true;
    config.lle_preserve_multiline_structure = true;
    config.lle_enable_multiline_editing = true;
    config.lle_show_multiline_indicators = true;
    config.lle_enable_interactive_search = true;
    config.lle_search_fuzzy_matching = false;
    config.lle_search_case_sensitive = false;
    config.lle_storage_mode = LLE_STORAGE_MODE_DUAL;
    config.lle_history_file = NULL;  // Will default to ~/.lle_history
    config.lle_sync_with_readline = true;
    config.lle_export_to_bash_history = true;
    config.lle_enable_forensic_tracking = true;
    config.lle_enable_deduplication = true;
    config.lle_dedup_scope = LLE_DEDUP_SCOPE_SESSION;
    config.lle_enable_history_cache = true;
    config.lle_cache_size = 100;
    config.lle_readline_compatible_mode = false;

    // Completion defaults
    config.completion_enabled = true;
    config.fuzzy_completion = true;
    config.completion_threshold = 60;
    config.completion_case_sensitive = false;
    config.completion_show_all = false;
    config.hints_enabled = false;

    // Prompt defaults
    config.prompt_style = strdup("git");
    config.prompt_theme = strdup("default");
    config.git_prompt_enabled = true;
    config.git_cache_timeout = 5;
    config.prompt_format = NULL;

    // Theme defaults (Phase 3 Target 2)
    config.theme_name = strdup("corporate");
    config.theme_auto_detect_colors = true;
    config.theme_fallback_basic = true;
    config.theme_corporate_company = NULL;
    config.theme_corporate_department = NULL;
    config.theme_corporate_project = NULL;
    config.theme_corporate_environment = NULL;
    config.theme_show_company = false;
    config.theme_show_department = false;
    config.theme_show_right_prompt = true;

    config.theme_enable_animations = false;
    config.theme_enable_icons = false;
    config.theme_color_support_override = 0;

    // Behavior defaults
    config.auto_cd = false;
    config.spell_correction = true;
    config.confirm_exit = false;
    config.tab_width = 4;
    config.no_word_expand = false;
    config.multiline_mode = true;

    // Auto-correction defaults
    config.autocorrect_max_suggestions = 3;
    config.autocorrect_threshold = 40;
    config.autocorrect_interactive = true;
    config.autocorrect_learn_history = true;
    config.autocorrect_builtins = true;
    config.autocorrect_external = true;
    config.autocorrect_case_sensitive = false;

    // Color defaults
    config.color_scheme = strdup("default");
    config.colors_enabled = true;

    // Advanced defaults
    config.verbose_errors = false;
    config.debug_mode = false;

    // Network defaults
    config.ssh_completion_enabled = true;
    config.cloud_discovery_enabled = false;
    config.cache_ssh_hosts = true;
    config.cache_timeout_minutes = 5;
    config.show_remote_context = true;
    config.auto_detect_cloud = true;
    config.max_completion_hosts = 50;

    // Display defaults
    // v1.3.0: Layered display is now the exclusive system - no configuration needed
    config.display_performance_monitoring = false;
    config.display_optimization_level = 0;

    // Script execution defaults
    config.script_execution = true;
    
    // Line editor defaults - GNU readline by default, LLE opt-in
    // Check environment variable for testing purposes
    const char *lle_env = getenv("LLE_ENABLED");
    config.use_lle = (lle_env && strcmp(lle_env, "1") == 0);
}

/**
 * config_get_user_config_path:
 *      Get the path to the user's configuration file.
 */
char *config_get_user_config_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        } else {
            home = "/tmp";
        }
    }

    size_t path_len = strlen(home) + strlen(USER_CONFIG_FILE) + 2;
    char *path = malloc(path_len);
    if (!path) {
        return NULL;
    }

    snprintf(path, path_len, "%s/%s", home, USER_CONFIG_FILE);
    return path;
}

/**
 * config_get_system_config_path:
 *      Get the path to the system configuration file.
 */
char *config_get_system_config_path(void) { return strdup(SYSTEM_CONFIG_FILE); }

/**
 * config_load_user:
 *      Load user configuration file.
 */
int config_load_user(void) {
    return config_load_file(config_ctx.user_config_path);
}

/**
 * config_load_system:
 *      Load system configuration file.
 */
int config_load_system(void) {
    return config_load_file(config_ctx.system_config_path);
}

/**
 * config_save_user:
 *      Save user configuration to file.
 */
int config_save_user(void) {
    if (!config_ctx.user_config_path) {
        return -1;
    }
    return config_save_file(config_ctx.user_config_path);
}

/**
 * config_save_file:
 *      Save configuration to a file in INI format.
 */
int config_save_file(const char *path) {
    FILE *file = fopen(path, "w");
    if (!file) {
        return -1;
    }

    // Write header comment
    fprintf(file, "# Lusush Configuration File\n");
    fprintf(file, "# This file is automatically generated by 'config save'\n");
    fprintf(file, "# Lines starting with # are comments\n");
    fprintf(file, "# This file uses dotted notation (e.g. history.enabled = true)\n\n");

    // Write all options using full dotted names (no sections)
    for (int i = 0; i < num_config_options; i++) {
        config_option_t *opt = &config_options[i];

        // Handle shell options specially
        if (opt->section == CONFIG_SECTION_SHELL) {
            bool value = config_get_shell_option(opt->name);
            fprintf(file, "%s = %s\n", opt->name, value ? "true" : "false");
        } else {
            // Handle regular options
            switch (opt->type) {
            case CONFIG_TYPE_BOOL:
                fprintf(file, "%s = %s\n", opt->name,
                        *(bool *)opt->value_ptr ? "true" : "false");
                break;
            case CONFIG_TYPE_INT:
                fprintf(file, "%s = %d\n", opt->name, *(int *)opt->value_ptr);
                break;
            case CONFIG_TYPE_STRING:
            case CONFIG_TYPE_COLOR:
                if (*(char **)opt->value_ptr && strlen(*(char **)opt->value_ptr) > 0) {
                    fprintf(file, "%s = %s\n", opt->name, *(char **)opt->value_ptr);
                }
                // Skip NULL or empty strings entirely to avoid parsing issues
                break;
            }
        }
    }

    fclose(file);
    return 0;
}

/**
 * config_load_file:
 *      Load configuration from a file.
 */
int config_load_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    config_ctx.current_file = path;
    config_ctx.line_number = 0;
    current_section = CONFIG_SECTION_NONE;

    char line[MAX_CONFIG_LINE];
    while (fgets(line, sizeof(line), file)) {
        config_ctx.line_number++;

        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (config_parse_line(line, config_ctx.line_number, path) != 0) {
            config_warning("Error parsing line %d in %s",
                           config_ctx.line_number, path);
        }
    }

    fclose(file);
    return 0;
}

/**
 * config_parse_line:
 *      Parse a single configuration line.
 */
int config_parse_line(const char *line, int line_num, const char *filename) {
    // Skip empty lines and comments
    const char *trimmed = line;
    while (isspace(*trimmed)) {
        trimmed++;
    }

    if (*trimmed == '\0' || *trimmed == '#') {
        return 0;
    }

    // Check for section header
    if (*trimmed == '[') {
        const char *end = strchr(trimmed, ']');
        if (!end) {
            config_error("Invalid section header at line %d in %s", line_num,
                         filename);
            return -1;
        }

        char section_name[64];
        size_t section_len = end - trimmed - 1;
        if (section_len >= sizeof(section_name)) {
            config_error("Section name too long at line %d in %s", line_num,
                         filename);
            return -1;
        }

        strncpy(section_name, trimmed + 1, section_len);
        section_name[section_len] = '\0';

        return config_parse_section(section_name);
    }

    // Parse key=value pair
    char *equals = strchr(trimmed, '=');
    if (!equals) {
        config_error("Invalid configuration line at %d in %s", line_num,
                     filename);
        return -1;
    }

    // Extract key
    char key[128];
    size_t key_len = equals - trimmed;
    if (key_len >= sizeof(key)) {
        config_error("Configuration key too long at line %d in %s", line_num,
                     filename);
        return -1;
    }

    strncpy(key, trimmed, key_len);
    key[key_len] = '\0';

    // Trim whitespace from key
    char *key_end = key + strlen(key) - 1;
    while (key_end > key && isspace(*key_end)) {
        *key_end-- = '\0';
    }

    // Extract value
    const char *value = equals + 1;
    while (isspace(*value)) {
        value++;
    }

    return config_parse_option(key, value);
}

/**
 * config_parse_section:
 *      Parse a configuration section header.
 */
int config_parse_section(const char *section_name) {
    if (strcmp(section_name, "history") == 0) {
        current_section = CONFIG_SECTION_HISTORY;
    } else if (strcmp(section_name, "completion") == 0) {
        current_section = CONFIG_SECTION_COMPLETION;
    } else if (strcmp(section_name, "prompt") == 0) {
        current_section = CONFIG_SECTION_PROMPT;
    } else if (strcmp(section_name, "behavior") == 0) {
        current_section = CONFIG_SECTION_BEHAVIOR;
    } else if (strcmp(section_name, "aliases") == 0) {
        current_section = CONFIG_SECTION_ALIASES;
    } else if (strcmp(section_name, "keys") == 0) {
        current_section = CONFIG_SECTION_KEYS;
    } else if (strcmp(section_name, "network") == 0) {
        current_section = CONFIG_SECTION_NETWORK;
    } else if (strcmp(section_name, "scripts") == 0) {
        current_section = CONFIG_SECTION_SCRIPTS;
    } else {
        config_warning("Unknown configuration section: %s", section_name);
        current_section = CONFIG_SECTION_NONE;
        return -1;
    }

    return 0;
}

/**
 * config_parse_option:
 *      Parse a configuration option.
 */
int config_parse_option(const char *key, const char *value) {
    // Handle aliases specially
    if (current_section == CONFIG_SECTION_ALIASES) {
        // Add alias: key = value
        char *alias_cmd = malloc(strlen(value) + 1);
        if (alias_cmd) {
            strcpy(alias_cmd, value);
            set_alias(key, alias_cmd);
        }
        return 0;
    }

    // Handle regular options
    for (int i = 0; i < num_config_options; i++) {
        config_option_t *opt = &config_options[i];

        if (strcmp(opt->name, key) == 0) {
            // Handle shell options specially - they use integration functions
            if (strncmp(key, "shell.", 6) == 0) {
                if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || 
                    strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
                    config_set_shell_option(key, true);
                } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 || 
                           strcmp(value, "off") == 0 || strcmp(value, "no") == 0) {
                    config_set_shell_option(key, false);
                } else {
                    config_error("Invalid boolean value '%s' for shell option '%s'", value, key);
                    return -1;
                }
                return 0;
            }

            // Validate value if validator exists
            if (opt->validator && !opt->validator(value)) {
                config_error("Invalid value '%s' for option '%s'", value, key);
                return -1;
            }

            // Set value based on type
            switch (opt->type) {
            case CONFIG_TYPE_BOOL: {
                bool bool_val =
                    (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
                     strcmp(value, "yes") == 0 || strcmp(value, "on") == 0);
                *(bool *)opt->value_ptr = bool_val;
                break;
            }
            case CONFIG_TYPE_INT: {
                int int_val = atoi(value);
                *(int *)opt->value_ptr = int_val;
                break;
            }
            case CONFIG_TYPE_STRING: {
                char **str_ptr = (char **)opt->value_ptr;
                if (*str_ptr) {
                    free(*str_ptr);
                }
                *str_ptr = strdup(value);
                break;
            }
            case CONFIG_TYPE_COLOR:
                // Color handling would go here
                break;
            }

            return 0;
        }
    }

    // Handle legacy configuration keys gracefully to eliminate warnings
    if (config_handle_legacy_key(key, value)) {
        return 0;
    }

    config_warning("Unknown configuration option: %s", key);
    return -1;
}

/**
 * config_apply_settings:
 *      Apply loaded configuration settings to the shell.
 */
void config_apply_settings(void) {
    // Apply settings safely - only set basic variables for now
    // More complex integrations will be added after basic functionality works

    // Basic symbol table settings
    symtable_set_global_int("CONFIG_LOADED", 1);
    symtable_set_global_int("HISTORY_NO_DUPS", config.history_no_dups ? 1 : 0);
    symtable_set_global_int("FUZZY_COMPLETION",
                            config.fuzzy_completion ? 1 : 0);
    symtable_set_global_int("COMPLETION_THRESHOLD",
                            config.completion_threshold);

    // Apply prompt settings (handled by theme system)

    // Apply behavior settings
    // Use multiline mode with fixed line consumption issues
    lusush_multiline_set_enabled(config.multiline_mode);

    // Apply history settings
    // History deduplication is handled automatically by readline integration

    // Apply hints system settings
    if (config.hints_enabled) {
        // Hints are handled differently in readline integration
        // TODO: Implement hints system for readline if needed
    } else {
        // Hints disabled - no action needed for readline integration
    }

    // Apply other settings as needed
    symtable_set_global_int("AUTO_CD", config.auto_cd);
    symtable_set_global_int("SPELL_CORRECTION", config.spell_correction);
    symtable_set_global_int("CONFIRM_EXIT", config.confirm_exit);
    symtable_set_global_int("COLORS_ENABLED", config.colors_enabled);
    symtable_set_global_int("NO_WORD_EXPAND", config.no_word_expand);
    symtable_set_global_int("MULTILINE_MODE", config.multiline_mode);

    // Update autocorrect configuration when spell correction settings change
    autocorrect_config_t autocorrect_cfg;
    autocorrect_get_default_config(&autocorrect_cfg);
    autocorrect_cfg.enabled = config.spell_correction;
    autocorrect_cfg.max_suggestions = config.autocorrect_max_suggestions;
    autocorrect_cfg.similarity_threshold = config.autocorrect_threshold;
    autocorrect_cfg.interactive_prompts = config.autocorrect_interactive;
    autocorrect_cfg.learn_from_history = config.autocorrect_learn_history;
    autocorrect_cfg.correct_builtins = config.autocorrect_builtins;
    autocorrect_cfg.correct_external = config.autocorrect_external;
    autocorrect_cfg.case_sensitive = config.autocorrect_case_sensitive;
    autocorrect_load_config(&autocorrect_cfg);

    // Update readline debug mode
    lusush_readline_set_debug(config.debug_mode);

    // v1.3.0: Autosuggestions disabled for stability
    // lusush_autosuggestions_sync_config() call removed
}

/**
 * config_create_user_config:
 *      Create a user configuration file with default values.
 */
int config_create_user_config(void) {
    FILE *file = fopen(config_ctx.user_config_path, "w");
    if (!file) {
        return -1;
    }

    fprintf(file, "%s", CONFIG_FILE_TEMPLATE);
    fclose(file);

    return 0;
}

/**
 * Validation functions
 */
bool config_validate_bool(const char *value) {
    return (strcmp(value, "true") == 0 || strcmp(value, "false") == 0 ||
            strcmp(value, "1") == 0 || strcmp(value, "0") == 0 ||
            strcmp(value, "yes") == 0 || strcmp(value, "no") == 0 ||
            strcmp(value, "on") == 0 || strcmp(value, "off") == 0);
}

bool config_validate_int(const char *value) {
    char *endptr;
    strtol(value, &endptr, 10);
    return (*endptr == '\0');
}

bool config_validate_string(const char *value) {
    return (value != NULL && strlen(value) > 0);
}

bool config_validate_color(const char *value) {
    // Basic color validation - could be enhanced
    return config_validate_string(value);
}

bool config_validate_prompt_style(const char *value) {
    return (strcmp(value, "normal") == 0 || strcmp(value, "color") == 0 ||
            strcmp(value, "fancy") == 0 || strcmp(value, "pro") == 0 ||
            strcmp(value, "git") == 0);
}

bool config_validate_color_scheme(const char *value) {
    return (strcmp(value, "default") == 0 || strcmp(value, "dark") == 0 ||
            strcmp(value, "light") == 0 || strcmp(value, "solarized") == 0);
}

bool config_validate_float(const char *value) {
    char *endptr;
    strtod(value, &endptr);
    return (*endptr == '\0');
}

bool config_validate_path(const char *value) {
    return config_validate_string(value);
}





bool config_validate_optimization_level(const char *value) {
    char *endptr;
    long level = strtol(value, &endptr, 10);
    return (*endptr == '\0' && level >= 0 && level <= 4);
}

bool config_validate_lle_arrow_mode(const char *value) {
    return (strcmp(value, "context-aware") == 0 ||
            strcmp(value, "classic") == 0 ||
            strcmp(value, "always-history") == 0 ||
            strcmp(value, "multiline-first") == 0);
}

bool config_validate_lle_storage_mode(const char *value) {
    return (strcmp(value, "lle-only") == 0 ||
            strcmp(value, "bash-only") == 0 ||
            strcmp(value, "dual") == 0 ||
            strcmp(value, "readline-compat") == 0);
}

bool config_validate_lle_dedup_scope(const char *value) {
    return (strcmp(value, "none") == 0 ||
            strcmp(value, "session") == 0 ||
            strcmp(value, "recent") == 0 ||
            strcmp(value, "global") == 0);
}

/**
 * Error handling functions
 */
void config_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(last_error, sizeof(last_error), format, args);
    va_end(args);

    fprintf(stderr, "Config Error: %s\n", last_error);
}

void config_warning(const char *format, ...) {
    char warning[256];
    va_list args;
    va_start(args, format);
    vsnprintf(warning, sizeof(warning), format, args);
    va_end(args);

    fprintf(stderr, "Config Warning: %s\n", warning);
}

const char *config_get_last_error(void) { return last_error; }

/**
 * Built-in command for configuration management
 */
void builtin_config(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: config [show|set|get|reload|save] [options]\n");
        printf("  show [section]     - Show configuration values\n");
        printf("  set key value      - Set configuration value\n");
        printf("  get key            - Get configuration value\n");
        printf("  reload             - Reload configuration files\n");
        printf("  save               - Save current configuration\n");
        return;
    }

    if (strcmp(argv[1], "show") == 0) {
        if (argc > 2) {
            // Show specific section
            config_section_t section = CONFIG_SECTION_NONE;
            if (strcmp(argv[2], "history") == 0) {
                section = CONFIG_SECTION_HISTORY;
            } else if (strcmp(argv[2], "completion") == 0) {
                section = CONFIG_SECTION_COMPLETION;
            } else if (strcmp(argv[2], "prompt") == 0) {
                section = CONFIG_SECTION_PROMPT;
            } else if (strcmp(argv[2], "behavior") == 0) {
                section = CONFIG_SECTION_BEHAVIOR;
            } else if (strcmp(argv[2], "display") == 0) {
                section = CONFIG_SECTION_DISPLAY;
            } else if (strcmp(argv[2], "network") == 0) {
                section = CONFIG_SECTION_NETWORK;
            } else if (strcmp(argv[2], "scripts") == 0) {
                section = CONFIG_SECTION_SCRIPTS;
            } else if (strcmp(argv[2], "shell") == 0) {
                section = CONFIG_SECTION_SHELL;
            }

            if (section != CONFIG_SECTION_NONE) {
                config_show_section(section);
            } else {
                printf("Unknown section: %s\n", argv[2]);
            }
        } else {
            config_show_all();
        }
    } else if (strcmp(argv[1], "get") == 0) {
        if (argc < 3) {
            printf("Usage: config get <key>\n");
            return;
        }
        config_get_value(argv[2]);
    } else if (strcmp(argv[1], "set") == 0) {
        if (argc < 4) {
            printf("Usage: config set <key> <value>\n");
            return;
        }
        config_set_value(argv[2], argv[3]);
    } else if (strcmp(argv[1], "reload") == 0) {
        config_init();
        printf("Configuration reloaded.\n");
    } else if (strcmp(argv[1], "save") == 0) {
        if (config_save_user() == 0) {
            printf("Configuration saved to %s\n", 
                   config_ctx.user_config_path ? config_ctx.user_config_path : "~/.lusushrc");
        } else {
            printf("Error: Failed to save configuration\n");
        }
    } else {
        printf("Unknown config command: %s\n", argv[1]);
    }
}

/**
 * config_get_value:
 *      Get a single configuration value.
 */
void config_get_value(const char *key) {
    // First try the exact key
    for (int i = 0; i < num_config_options; i++) {
        config_option_t *opt = &config_options[i];

        if (strcmp(opt->name, key) == 0) {
            // Handle shell options specially - they use integration functions
            if (strncmp(key, "shell.", 6) == 0) {
                printf("%s\n", config_get_shell_option(key) ? "true" : "false");
                return;
            }
            
            switch (opt->type) {
            case CONFIG_TYPE_BOOL:
                printf("%s\n", *(bool *)opt->value_ptr ? "true" : "false");
                break;
            case CONFIG_TYPE_INT:
                printf("%d\n", *(int *)opt->value_ptr);
                break;
            case CONFIG_TYPE_STRING:
                printf("%s\n", *(char **)opt->value_ptr
                                   ? *(char **)opt->value_ptr
                                   : "");
                break;
            case CONFIG_TYPE_COLOR:
                printf("%s\n", *(char **)opt->value_ptr
                                   ? *(char **)opt->value_ptr
                                   : "");
                break;
            }
            return;
        }
    }
    
    // Check for legacy option name
    const char *new_name = find_new_name_for_legacy(key);
    if (new_name) {
        printf("Warning: '%s' is deprecated, use '%s' instead\n", key, new_name);
        config_get_value(new_name);  // Recursive call with new name
        return;
    }
    
    printf("Unknown configuration key: %s\n", key);
}

/**
 * config_set_value:
 *      Set a single configuration value.
 */
void config_set_value(const char *key, const char *value) {
    // First try the exact key
    for (int i = 0; i < num_config_options; i++) {
        config_option_t *opt = &config_options[i];

        if (strcmp(opt->name, key) == 0) {
            // Handle shell options specially - they use integration functions
            if (strncmp(key, "shell.", 6) == 0) {
                if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "on") == 0) {
                    config_set_shell_option(key, true);
                } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 || strcmp(value, "off") == 0) {
                    config_set_shell_option(key, false);
                } else {
                    printf("Invalid boolean value: %s (use true/false/on/off)\n", value);
                    return;
                }
                printf("Set %s = %s\n", key, value);
                return;
            }
            
            switch (opt->type) {
            case CONFIG_TYPE_BOOL:
                if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                    *(bool *)opt->value_ptr = true;
                } else if (strcmp(value, "false") == 0 ||
                           strcmp(value, "0") == 0) {
                    *(bool *)opt->value_ptr = false;
                } else {
                    printf("Invalid boolean value: %s (use true/false)\n",
                           value);
                    return;
                }
                break;
            case CONFIG_TYPE_INT:
                *(int *)opt->value_ptr = atoi(value);
                break;
            case CONFIG_TYPE_STRING:
                if (*(char **)opt->value_ptr) {
                    free(*(char **)opt->value_ptr);
                }
                *(char **)opt->value_ptr = strdup(value);
                break;
            case CONFIG_TYPE_COLOR:
                if (config_validate_color(value)) {
                    if (*(char **)opt->value_ptr) {
                        free(*(char **)opt->value_ptr);
                    }
                    *(char **)opt->value_ptr = strdup(value);
                } else {
                    printf("Invalid color value: %s\n", value);
                    return;
                }
                break;
            }
            printf("Set %s = %s\n", key, value);

            // Apply the setting immediately
            config_apply_settings();
            return;
        }
    }
    
    // Check for legacy option name
    const char *new_name = find_new_name_for_legacy(key);
    if (new_name) {
        printf("Warning: '%s' is deprecated, use '%s' instead\n", key, new_name);
        config_set_value(new_name, value);  // Recursive call with new name
        return;
    }
    
    // Check for common typos and provide helpful suggestions
    if (strcmp(key, "hints_enable") == 0) {
        printf("Unknown configuration key: %s\n", key);
        printf("Did you mean 'completion.hints'?\n");
        printf("Try: config set completion.hints %s\n", value);
        return;
    }
    
    printf("Unknown configuration key: %s\n", key);
}

/**
 * config_show_all:
 *      Show all configuration values.
 */
void config_show_all(void) {
    printf("LUSUSH Configuration:\n\n");

    printf("[history]\n");
    config_show_section(CONFIG_SECTION_HISTORY);

    printf("\n[completion]\n");
    config_show_section(CONFIG_SECTION_COMPLETION);

    printf("\n[prompt]\n");
    config_show_section(CONFIG_SECTION_PROMPT);

    printf("\n[behavior]\n");
    config_show_section(CONFIG_SECTION_BEHAVIOR);

    printf("\n[display]\n");
    config_show_section(CONFIG_SECTION_DISPLAY);

    printf("\n[network]\n");
    config_show_section(CONFIG_SECTION_NETWORK);

    printf("\n[scripts]\n");
    config_show_section(CONFIG_SECTION_SCRIPTS);

    printf("\n[shell]\n");
    config_show_section(CONFIG_SECTION_SHELL);
}

/**
 * config_show_section:
 *      Show configuration values for a specific section.
 */
void config_show_section(config_section_t section) {
    for (int i = 0; i < num_config_options; i++) {
        config_option_t *opt = &config_options[i];

        if (opt->section == section) {
            printf("  %s = ", opt->name);

            // Handle shell options specially - they use integration functions
            if (strncmp(opt->name, "shell.", 6) == 0) {
                printf("%s", config_get_shell_option(opt->name) ? "true" : "false");
            } else {
                switch (opt->type) {
                case CONFIG_TYPE_BOOL:
                    printf("%s", *(bool *)opt->value_ptr ? "true" : "false");
                    break;
            case CONFIG_TYPE_INT:
                printf("%d", *(int *)opt->value_ptr);
                break;
            case CONFIG_TYPE_STRING: {
                char *str_val = *(char **)opt->value_ptr;
                printf("%s", str_val ? str_val : "(null)");
                break;
            }
            case CONFIG_TYPE_COLOR:
                printf("(color)");
                break;
            }
        }

        printf("  # %s\n", opt->description);
        }
    }
}

/**
 * config_cleanup:
 *      Clean up configuration resources.
 */
void config_cleanup(void) {
    if (config_ctx.user_config_path) {
        free(config_ctx.user_config_path);
    }
    if (config_ctx.system_config_path) {
        free(config_ctx.system_config_path);
    }
    if (config.prompt_style) {
        free(config.prompt_style);
    }
    if (config.prompt_theme) {
        free(config.prompt_theme);
    }
    if (config.prompt_format) {
        free(config.prompt_format);
    }

    // Theme cleanup (Phase 3 Target 2)
    if (config.theme_name) {
        free(config.theme_name);
    }
    if (config.theme_corporate_company) {
        free(config.theme_corporate_company);
    }
    if (config.theme_corporate_department) {
        free(config.theme_corporate_department);
    }
    if (config.theme_corporate_project) {
        free(config.theme_corporate_project);
    }
    if (config.theme_corporate_environment) {
        free(config.theme_corporate_environment);
    }
    if (config.history_file) {
        free(config.history_file);
    }
    if (config.color_scheme) {
        free(config.color_scheme);
    }
}
