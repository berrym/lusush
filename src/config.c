#include "../include/config.h"

#include "../include/alias.h"
#include "../include/errors.h"
#include "../include/history.h"
#include "../include/linenoise/linenoise.h"
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
    {          "history_enabled",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_HISTORY,
     &config.history_enabled,"Enable command history",config_validate_bool                                                                                  },
    {             "history_size",    CONFIG_TYPE_INT,    CONFIG_SECTION_HISTORY,
     &config.history_size,                       "Maximum history entries",          config_validate_int       },
    {          "history_no_dups",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_HISTORY,
     &config.history_no_dups,              "Remove duplicate history entries",
     config_validate_bool                                                                                      },
    {       "history_timestamps",   CONFIG_TYPE_BOOL,    CONFIG_SECTION_HISTORY,
     &config.history_timestamps,                     "Add timestamps to history",
     config_validate_bool                                                                                      },
    {             "history_file", CONFIG_TYPE_STRING,    CONFIG_SECTION_HISTORY,
     &config.history_file,                             "History file path",       config_validate_string       },

    // Completion settings
    {       "completion_enabled",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.completion_enabled,                         "Enable tab completion",         config_validate_bool },
    {         "fuzzy_completion",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.fuzzy_completion,           "Enable fuzzy matching in completion",
     config_validate_bool                                                                                      },
    {     "completion_threshold",    CONFIG_TYPE_INT, CONFIG_SECTION_COMPLETION,
     &config.completion_threshold,              "Fuzzy matching threshold (0-100)",
     config_validate_int                                                                                       },
    {"completion_case_sensitive",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.completion_case_sensitive,                     "Case sensitive completion",
     config_validate_bool                                                                                      },
    {      "completion_show_all",   CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
     &config.completion_show_all,                          "Show all completions",         config_validate_bool},

    // Prompt settings
    {             "prompt_style", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.prompt_style, "Prompt style (normal, color, fancy, pro, git)",
     config_validate_prompt_style                                                                              },
    {             "prompt_theme", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.prompt_theme,                            "Prompt color theme",       config_validate_string       },
    {       "git_prompt_enabled",   CONFIG_TYPE_BOOL,     CONFIG_SECTION_PROMPT,
     &config.git_prompt_enabled,                      "Enable git-aware prompts",
     config_validate_bool                                                                                      },
    {        "git_cache_timeout",    CONFIG_TYPE_INT,     CONFIG_SECTION_PROMPT,
     &config.git_cache_timeout,           "Git status cache timeout in seconds",
     config_validate_int                                                                                       },
    {            "prompt_format", CONFIG_TYPE_STRING,     CONFIG_SECTION_PROMPT,
     &config.prompt_format,                   "Custom prompt format string",
     config_validate_string                                                                                    },

    // Behavior settings
    {           "multiline_edit",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.multiline_edit,                      "Enable multiline editing",         config_validate_bool     },
    {                  "auto_cd",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,   &config.auto_cd,
     "Auto-cd to directories",         config_validate_bool                                                    },
    {         "spell_correction",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.spell_correction,               "Enable command spell correction",
     config_validate_bool                                                                                      },
    {             "confirm_exit",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.confirm_exit,                        "Confirm before exiting",         config_validate_bool       },
    {                "tab_width",    CONFIG_TYPE_INT,   CONFIG_SECTION_BEHAVIOR, &config.tab_width,
     "Tab width for display",          config_validate_int                                                     },

    // Color settings
    {             "color_scheme", CONFIG_TYPE_STRING,   CONFIG_SECTION_BEHAVIOR,
     &config.color_scheme,                             "Color scheme name", config_validate_color_scheme       },
    {           "colors_enabled",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.colors_enabled,                           "Enable color output",         config_validate_bool     },

    // Advanced settings
    {           "verbose_errors",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.verbose_errors,                   "Show verbose error messages",
     config_validate_bool                                                                                      },
    {               "debug_mode",   CONFIG_TYPE_BOOL,   CONFIG_SECTION_BEHAVIOR,
     &config.debug_mode,                             "Enable debug mode",         config_validate_bool         },
};

static const int num_config_options =
    sizeof(config_options) / sizeof(config_option_t);

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
    "[completion]\n"
    "completion_enabled = true\n"
    "fuzzy_completion = true\n"
    "completion_threshold = 60\n"
    "completion_case_sensitive = false\n"
    "completion_show_all = false\n"
    "\n"
    "[prompt]\n"
    "prompt_style = git\n"
    "prompt_theme = default\n"
    "git_prompt_enabled = true\n"
    "git_cache_timeout = 5\n"
    "# prompt_format = \"%u@%h in %d%g %% \"\n"
    "\n"
    "[behavior]\n"
    "multiline_edit = true\n"
    "auto_cd = false\n"
    "spell_correction = false\n"
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

    // Completion defaults
    config.completion_enabled = true;
    config.fuzzy_completion = true;
    config.completion_threshold = 60;
    config.completion_case_sensitive = false;
    config.completion_show_all = false;

    // Prompt defaults
    config.prompt_style = strdup("git");
    config.prompt_theme = strdup("default");
    config.git_prompt_enabled = true;
    config.git_cache_timeout = 5;
    config.prompt_format = NULL;

    // Behavior defaults
    config.multiline_edit = true;
    config.auto_cd = false;
    config.spell_correction = false;
    config.confirm_exit = false;
    config.tab_width = 4;

    // Color defaults
    config.color_scheme = strdup("default");
    config.colors_enabled = true;

    // Advanced defaults
    config.verbose_errors = false;
    config.debug_mode = false;
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

    // Apply prompt settings (basic application) - skip for now to avoid crashes
    // if (config.prompt_style) {
    //     // Set prompt style via setprompt command
    //     char *prompt_args[] = {"setprompt", "-s", config.prompt_style, NULL};
    //     set_prompt(3, prompt_args);
    // }

    // Apply behavior settings
    symtable_set_global_int("MULTILINE_EDIT", config.multiline_edit);
    linenoiseSetMultiLine(config.multiline_edit);

    // Apply other settings as needed
    symtable_set_global_int("AUTO_CD", config.auto_cd);
    symtable_set_global_int("SPELL_CORRECTION", config.spell_correction);
    symtable_set_global_int("CONFIRM_EXIT", config.confirm_exit);
    symtable_set_global_int("COLORS_ENABLED", config.colors_enabled);
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
            }

            if (section != CONFIG_SECTION_NONE) {
                config_show_section(section);
            } else {
                printf("Unknown section: %s\n", argv[2]);
            }
        } else {
            config_show_all();
        }
    } else if (strcmp(argv[1], "reload") == 0) {
        config_init();
        printf("Configuration reloaded.\n");
    } else {
        printf("Unknown config command: %s\n", argv[1]);
    }
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
    if (config.history_file) {
        free(config.history_file);
    }
    if (config.color_scheme) {
        free(config.color_scheme);
    }
}
