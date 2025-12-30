/**
 * @file theme_parser.c
 * @brief LLE Theme File Parser - TOML-subset Parser Implementation
 *
 * Specification: Issue #21 - Theme File Loading System
 * Version: 1.0.0
 *
 * Custom TOML-subset parser for parsing theme configuration files.
 * Designed to be dependency-free and focused on the subset of TOML
 * needed for theme files.
 */

#include "lle/prompt/theme_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Forward declaration for portability (see ht_fnv1a.c) */
int strcasecmp(const char *s1, const char *s2);

/* ============================================================================
 * Internal Helper Macros
 * ============================================================================
 */

#define PARSER_EOF(p) ((p)->pos >= (p)->input_len)
#define PARSER_PEEK(p) (PARSER_EOF(p) ? '\0' : (p)->input[(p)->pos])
#define PARSER_ADVANCE(p)                                                      \
    do {                                                                       \
        if (!PARSER_EOF(p)) {                                                  \
            if ((p)->input[(p)->pos] == '\n') {                                \
                (p)->line++;                                                   \
                (p)->column = 1;                                               \
            } else {                                                           \
                (p)->column++;                                                 \
            }                                                                  \
            (p)->pos++;                                                        \
        }                                                                      \
    } while (0)

/* ============================================================================
 * Forward Declarations
 * ============================================================================
 */

static void parser_skip_whitespace(lle_theme_parser_t *parser);
static void parser_skip_whitespace_and_newlines(lle_theme_parser_t *parser);
static void parser_skip_comment(lle_theme_parser_t *parser);
static void parser_skip_line(lle_theme_parser_t *parser);
static lle_result_t parser_set_error(lle_theme_parser_t *parser,
                                     const char *message);

static lle_result_t parser_parse_section(lle_theme_parser_t *parser);
static lle_result_t parser_parse_key(lle_theme_parser_t *parser, char *key,
                                     size_t key_size);
static lle_result_t parser_parse_value(lle_theme_parser_t *parser,
                                       lle_theme_value_t *value);
static lle_result_t parser_parse_string(lle_theme_parser_t *parser, char *out,
                                        size_t out_size);
static lle_result_t parser_parse_integer(lle_theme_parser_t *parser,
                                         int64_t *out);
static lle_result_t parser_parse_boolean(lle_theme_parser_t *parser, bool *out);
static lle_result_t parser_parse_array(lle_theme_parser_t *parser,
                                       lle_theme_value_t *value);
static lle_result_t parser_parse_inline_table(lle_theme_parser_t *parser,
                                              lle_theme_value_t *value);

/* Theme construction callback */
static lle_result_t theme_builder_callback(const char *section, const char *key,
                                           const lle_theme_value_t *value,
                                           void *user_data);

/* ============================================================================
 * Parser Core API
 * ============================================================================
 */

/**
 * @brief Initialize a theme parser
 */
lle_result_t lle_theme_parser_init(lle_theme_parser_t *parser,
                                   const char *input) {
    if (!parser || !input) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(parser, 0, sizeof(*parser));
    parser->input = input;
    parser->input_len = strlen(input);
    parser->pos = 0;
    parser->line = 1;
    parser->column = 1;
    parser->current_section[0] = '\0';
    parser->error_msg[0] = '\0';

    return LLE_SUCCESS;
}

/**
 * @brief Reset parser to beginning of input
 */
void lle_theme_parser_reset(lle_theme_parser_t *parser) {
    if (!parser) {
        return;
    }

    parser->pos = 0;
    parser->line = 1;
    parser->column = 1;
    parser->current_section[0] = '\0';
    parser->error_msg[0] = '\0';
    parser->error_line = 0;
    parser->error_column = 0;
    parser->keys_parsed = 0;
    parser->sections_parsed = 0;
}

/**
 * @brief Parse input and call callback for each key-value pair
 */
lle_result_t lle_theme_parser_parse(lle_theme_parser_t *parser,
                                    lle_theme_parser_callback_t callback,
                                    void *user_data) {
    if (!parser || !callback) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    while (!PARSER_EOF(parser)) {
        parser_skip_whitespace_and_newlines(parser);

        if (PARSER_EOF(parser)) {
            break;
        }

        char c = PARSER_PEEK(parser);

        /* Skip comments */
        if (c == '#') {
            parser_skip_line(parser);
            continue;
        }

        /* Parse section header */
        if (c == '[') {
            lle_result_t result = parser_parse_section(parser);
            if (result != LLE_SUCCESS) {
                return result;
            }
            parser->sections_parsed++;
            continue;
        }

        /* Parse key-value pair */
        if (isalpha((unsigned char)c) || c == '_') {
            char key[LLE_THEME_PARSER_KEY_MAX];
            lle_result_t result = parser_parse_key(parser, key, sizeof(key));
            if (result != LLE_SUCCESS) {
                return result;
            }

            parser_skip_whitespace(parser);

            /* Expect '=' */
            if (PARSER_PEEK(parser) != '=') {
                return parser_set_error(parser, "Expected '=' after key");
            }
            PARSER_ADVANCE(parser); /* Skip '=' */

            parser_skip_whitespace(parser);

            /* Parse value */
            lle_theme_value_t value;
            memset(&value, 0, sizeof(value));
            result = parser_parse_value(parser, &value);
            if (result != LLE_SUCCESS) {
                lle_theme_value_free(&value);
                return result;
            }

            /* Call callback */
            result = callback(parser->current_section, key, &value, user_data);
            lle_theme_value_free(&value);

            if (result != LLE_SUCCESS) {
                return result;
            }

            parser->keys_parsed++;
            continue;
        }

        /* Unknown character */
        return parser_set_error(parser, "Unexpected character");
    }

    return LLE_SUCCESS;
}

/**
 * @brief Get error message from parser
 */
const char *lle_theme_parser_error(const lle_theme_parser_t *parser) {
    if (!parser) {
        return "";
    }
    return parser->error_msg;
}

/**
 * @brief Get error line number
 */
size_t lle_theme_parser_error_line(const lle_theme_parser_t *parser) {
    if (!parser) {
        return 0;
    }
    return parser->error_line;
}

/**
 * @brief Get error column number
 */
size_t lle_theme_parser_error_column(const lle_theme_parser_t *parser) {
    if (!parser) {
        return 0;
    }
    return parser->error_column;
}

/* ============================================================================
 * Internal Parsing Helpers
 * ============================================================================
 */

/**
 * @brief Skip whitespace (space and tab only)
 */
static void parser_skip_whitespace(lle_theme_parser_t *parser) {
    while (!PARSER_EOF(parser)) {
        char c = PARSER_PEEK(parser);
        if (c == ' ' || c == '\t') {
            PARSER_ADVANCE(parser);
        } else {
            break;
        }
    }
}

/**
 * @brief Skip whitespace and newlines
 */
static void parser_skip_whitespace_and_newlines(lle_theme_parser_t *parser) {
    while (!PARSER_EOF(parser)) {
        char c = PARSER_PEEK(parser);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            PARSER_ADVANCE(parser);
        } else if (c == '#') {
            /* Skip comment lines */
            parser_skip_line(parser);
        } else {
            break;
        }
    }
}

/**
 * @brief Skip a comment (from # to end of line)
 */
static void parser_skip_comment(lle_theme_parser_t *parser) {
    if (PARSER_PEEK(parser) == '#') {
        parser_skip_line(parser);
    }
}

/**
 * @brief Skip to end of line
 */
static void parser_skip_line(lle_theme_parser_t *parser) {
    while (!PARSER_EOF(parser) && PARSER_PEEK(parser) != '\n') {
        PARSER_ADVANCE(parser);
    }
    if (PARSER_PEEK(parser) == '\n') {
        PARSER_ADVANCE(parser);
    }
}

/**
 * @brief Set parser error with message
 */
static lle_result_t parser_set_error(lle_theme_parser_t *parser,
                                     const char *message) {
    parser->error_line = parser->line;
    parser->error_column = parser->column;
    snprintf(parser->error_msg, sizeof(parser->error_msg),
             "Line %zu, column %zu: %s", parser->line, parser->column, message);
    return LLE_ERROR_INVALID_FORMAT;
}

/**
 * @brief Parse section header [section] or [section.subsection]
 */
static lle_result_t parser_parse_section(lle_theme_parser_t *parser) {
    if (PARSER_PEEK(parser) != '[') {
        return parser_set_error(parser, "Expected '['");
    }
    PARSER_ADVANCE(parser); /* Skip '[' */

    /* Read section name */
    char section[LLE_THEME_PARSER_KEY_MAX * LLE_THEME_PARSER_SECTION_DEPTH_MAX];
    size_t len = 0;

    while (!PARSER_EOF(parser) && PARSER_PEEK(parser) != ']') {
        char c = PARSER_PEEK(parser);

        if (isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.') {
            if (len >= sizeof(section) - 1) {
                return parser_set_error(parser, "Section name too long");
            }
            section[len++] = c;
            PARSER_ADVANCE(parser);
        } else if (c == ' ' || c == '\t') {
            /* Skip whitespace in section name */
            PARSER_ADVANCE(parser);
        } else {
            return parser_set_error(parser,
                                    "Invalid character in section name");
        }
    }

    if (PARSER_PEEK(parser) != ']') {
        return parser_set_error(parser, "Expected ']'");
    }
    PARSER_ADVANCE(parser); /* Skip ']' */

    section[len] = '\0';
    snprintf(parser->current_section, sizeof(parser->current_section), "%s",
             section);

    /* Skip rest of line */
    parser_skip_whitespace(parser);
    if (PARSER_PEEK(parser) == '#') {
        parser_skip_comment(parser);
    } else if (PARSER_PEEK(parser) != '\n' && !PARSER_EOF(parser)) {
        return parser_set_error(parser, "Unexpected content after section");
    }

    return LLE_SUCCESS;
}

/**
 * @brief Parse a key name (identifier)
 */
static lle_result_t parser_parse_key(lle_theme_parser_t *parser, char *key,
                                     size_t key_size) {
    size_t len = 0;

    while (!PARSER_EOF(parser)) {
        char c = PARSER_PEEK(parser);

        if (isalnum((unsigned char)c) || c == '_' || c == '-') {
            if (len >= key_size - 1) {
                return parser_set_error(parser, "Key name too long");
            }
            key[len++] = c;
            PARSER_ADVANCE(parser);
        } else {
            break;
        }
    }

    if (len == 0) {
        return parser_set_error(parser, "Empty key name");
    }

    key[len] = '\0';
    return LLE_SUCCESS;
}

/**
 * @brief Parse a value (string, integer, boolean, array, or inline table)
 */
static lle_result_t parser_parse_value(lle_theme_parser_t *parser,
                                       lle_theme_value_t *value) {
    memset(value, 0, sizeof(*value));

    char c = PARSER_PEEK(parser);

    /* String */
    if (c == '"') {
        value->type = LLE_THEME_VALUE_STRING;
        return parser_parse_string(parser, value->data.string,
                                   sizeof(value->data.string));
    }

    /* Array */
    if (c == '[') {
        return parser_parse_array(parser, value);
    }

    /* Inline table */
    if (c == '{') {
        return parser_parse_inline_table(parser, value);
    }

    /* Boolean: true or false */
    if (c == 't' || c == 'f') {
        value->type = LLE_THEME_VALUE_BOOLEAN;
        return parser_parse_boolean(parser, &value->data.boolean);
    }

    /* Integer (including negative) */
    if (isdigit((unsigned char)c) || c == '-' || c == '+') {
        value->type = LLE_THEME_VALUE_INTEGER;
        return parser_parse_integer(parser, &value->data.integer);
    }

    return parser_set_error(parser, "Invalid value");
}

/**
 * @brief Parse a quoted string with escape sequences
 */
static lle_result_t parser_parse_string(lle_theme_parser_t *parser, char *out,
                                        size_t out_size) {
    if (PARSER_PEEK(parser) != '"') {
        return parser_set_error(parser, "Expected '\"'");
    }
    PARSER_ADVANCE(parser); /* Skip opening quote */

    size_t len = 0;

    while (!PARSER_EOF(parser)) {
        char c = PARSER_PEEK(parser);

        if (c == '"') {
            PARSER_ADVANCE(parser); /* Skip closing quote */
            out[len] = '\0';
            return LLE_SUCCESS;
        }

        if (c == '\n') {
            return parser_set_error(parser, "Unterminated string");
        }

        if (c == '\\') {
            /* Escape sequence */
            PARSER_ADVANCE(parser);
            if (PARSER_EOF(parser)) {
                return parser_set_error(parser, "Unterminated escape sequence");
            }

            char escaped = PARSER_PEEK(parser);
            PARSER_ADVANCE(parser);

            switch (escaped) {
            case 'n':
                c = '\n';
                break;
            case 't':
                c = '\t';
                break;
            case 'r':
                c = '\r';
                break;
            case '\\':
                c = '\\';
                break;
            case '"':
                c = '"';
                break;
            default:
                return parser_set_error(parser, "Invalid escape sequence");
            }
        } else {
            PARSER_ADVANCE(parser);
        }

        if (len >= out_size - 1) {
            return parser_set_error(parser, "String too long");
        }
        out[len++] = c;
    }

    return parser_set_error(parser, "Unterminated string");
}

/**
 * @brief Parse an integer value
 */
static lle_result_t parser_parse_integer(lle_theme_parser_t *parser,
                                         int64_t *out) {
    char buffer[32];
    size_t len = 0;

    /* Sign */
    char c = PARSER_PEEK(parser);
    if (c == '-' || c == '+') {
        buffer[len++] = c;
        PARSER_ADVANCE(parser);
    }

    /* Digits */
    while (!PARSER_EOF(parser)) {
        c = PARSER_PEEK(parser);
        if (isdigit((unsigned char)c)) {
            if (len >= sizeof(buffer) - 1) {
                return parser_set_error(parser, "Integer too long");
            }
            buffer[len++] = c;
            PARSER_ADVANCE(parser);
        } else {
            break;
        }
    }

    if (len == 0 || (len == 1 && (buffer[0] == '-' || buffer[0] == '+'))) {
        return parser_set_error(parser, "Invalid integer");
    }

    buffer[len] = '\0';
    *out = strtoll(buffer, NULL, 10);
    return LLE_SUCCESS;
}

/**
 * @brief Parse a boolean value (true or false)
 */
static lle_result_t parser_parse_boolean(lle_theme_parser_t *parser,
                                         bool *out) {
    char word[6] = {0};
    size_t i = 0;

    while (!PARSER_EOF(parser) && i < 5) {
        char c = PARSER_PEEK(parser);
        if (isalpha((unsigned char)c)) {
            word[i++] = (char)tolower((unsigned char)c);
            PARSER_ADVANCE(parser);
        } else {
            break;
        }
    }
    word[i] = '\0';

    if (strcmp(word, "true") == 0) {
        *out = true;
        return LLE_SUCCESS;
    } else if (strcmp(word, "false") == 0) {
        *out = false;
        return LLE_SUCCESS;
    }

    return parser_set_error(parser,
                            "Invalid boolean (expected 'true' or 'false')");
}

/**
 * @brief Parse an array value ["a", "b", ...]
 */
static lle_result_t parser_parse_array(lle_theme_parser_t *parser,
                                       lle_theme_value_t *value) {
    if (PARSER_PEEK(parser) != '[') {
        return parser_set_error(parser, "Expected '['");
    }
    PARSER_ADVANCE(parser); /* Skip '[' */

    value->type = LLE_THEME_VALUE_ARRAY;
    value->data.array.items = NULL;
    value->data.array.count = 0;

    /* Allocate array storage */
    lle_theme_value_t *items =
        calloc(LLE_THEME_PARSER_ARRAY_MAX, sizeof(lle_theme_value_t));
    if (!items) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    parser_skip_whitespace_and_newlines(parser);

    while (!PARSER_EOF(parser) && PARSER_PEEK(parser) != ']') {
        if (value->data.array.count >= LLE_THEME_PARSER_ARRAY_MAX) {
            free(items);
            return parser_set_error(parser, "Array too large");
        }

        /* Parse array element */
        lle_result_t result =
            parser_parse_value(parser, &items[value->data.array.count]);
        if (result != LLE_SUCCESS) {
            /* Free already parsed items */
            for (size_t i = 0; i < value->data.array.count; i++) {
                lle_theme_value_free(&items[i]);
            }
            free(items);
            return result;
        }
        value->data.array.count++;

        parser_skip_whitespace_and_newlines(parser);

        /* Check for comma or end */
        if (PARSER_PEEK(parser) == ',') {
            PARSER_ADVANCE(parser);
            parser_skip_whitespace_and_newlines(parser);
        } else if (PARSER_PEEK(parser) != ']') {
            for (size_t i = 0; i < value->data.array.count; i++) {
                lle_theme_value_free(&items[i]);
            }
            free(items);
            return parser_set_error(parser, "Expected ',' or ']' in array");
        }
    }

    if (PARSER_PEEK(parser) != ']') {
        for (size_t i = 0; i < value->data.array.count; i++) {
            lle_theme_value_free(&items[i]);
        }
        free(items);
        return parser_set_error(parser, "Unterminated array");
    }
    PARSER_ADVANCE(parser); /* Skip ']' */

    value->data.array.items = items;
    return LLE_SUCCESS;
}

/**
 * @brief Parse an inline table { key = value, ... }
 */
static lle_result_t parser_parse_inline_table(lle_theme_parser_t *parser,
                                              lle_theme_value_t *value) {
    if (PARSER_PEEK(parser) != '{') {
        return parser_set_error(parser, "Expected '{'");
    }
    PARSER_ADVANCE(parser); /* Skip '{' */

    value->type = LLE_THEME_VALUE_TABLE;
    value->data.table.count = 0;

    parser_skip_whitespace(parser);

    while (!PARSER_EOF(parser) && PARSER_PEEK(parser) != '}') {
        if (value->data.table.count >= LLE_THEME_PARSER_TABLE_ENTRIES_MAX) {
            return parser_set_error(parser, "Inline table too large");
        }

        /* Parse key */
        lle_theme_table_entry_t *entry =
            &value->data.table.entries[value->data.table.count];
        lle_result_t result =
            parser_parse_key(parser, entry->key, sizeof(entry->key));
        if (result != LLE_SUCCESS) {
            return result;
        }

        parser_skip_whitespace(parser);

        /* Expect '=' */
        if (PARSER_PEEK(parser) != '=') {
            return parser_set_error(parser, "Expected '=' in inline table");
        }
        PARSER_ADVANCE(parser);

        parser_skip_whitespace(parser);

        /* Parse value */
        entry->value = calloc(1, sizeof(lle_theme_value_t));
        if (!entry->value) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }

        result = parser_parse_value(parser, entry->value);
        if (result != LLE_SUCCESS) {
            free(entry->value);
            entry->value = NULL;
            return result;
        }

        value->data.table.count++;
        parser_skip_whitespace(parser);

        /* Check for comma or end */
        if (PARSER_PEEK(parser) == ',') {
            PARSER_ADVANCE(parser);
            parser_skip_whitespace(parser);
        } else if (PARSER_PEEK(parser) != '}') {
            return parser_set_error(parser,
                                    "Expected ',' or '}' in inline table");
        }
    }

    if (PARSER_PEEK(parser) != '}') {
        return parser_set_error(parser, "Unterminated inline table");
    }
    PARSER_ADVANCE(parser); /* Skip '}' */

    return LLE_SUCCESS;
}

/* ============================================================================
 * Value Helpers
 * ============================================================================
 */

/**
 * @brief Create a string value
 */
lle_result_t lle_theme_value_set_string(lle_theme_value_t *value,
                                        const char *str) {
    if (!value || !str) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    value->type = LLE_THEME_VALUE_STRING;
    snprintf(value->data.string, sizeof(value->data.string), "%s", str);
    return LLE_SUCCESS;
}

/**
 * @brief Create an integer value
 */
void lle_theme_value_set_integer(lle_theme_value_t *value, int64_t integer) {
    if (!value) {
        return;
    }

    value->type = LLE_THEME_VALUE_INTEGER;
    value->data.integer = integer;
}

/**
 * @brief Create a boolean value
 */
void lle_theme_value_set_boolean(lle_theme_value_t *value, bool boolean) {
    if (!value) {
        return;
    }

    value->type = LLE_THEME_VALUE_BOOLEAN;
    value->data.boolean = boolean;
}

/**
 * @brief Free resources used by a value
 */
void lle_theme_value_free(lle_theme_value_t *value) {
    if (!value) {
        return;
    }

    if (value->type == LLE_THEME_VALUE_ARRAY && value->data.array.items) {
        for (size_t i = 0; i < value->data.array.count; i++) {
            lle_theme_value_free(&value->data.array.items[i]);
        }
        free(value->data.array.items);
        value->data.array.items = NULL;
        value->data.array.count = 0;
    }

    if (value->type == LLE_THEME_VALUE_TABLE) {
        for (size_t i = 0; i < value->data.table.count; i++) {
            if (value->data.table.entries[i].value) {
                lle_theme_value_free(value->data.table.entries[i].value);
                free(value->data.table.entries[i].value);
                value->data.table.entries[i].value = NULL;
            }
        }
        value->data.table.count = 0;
    }
}

/**
 * @brief Get a string from a table value by key
 */
lle_result_t lle_theme_value_table_get_string(const lle_theme_value_t *value,
                                              const char *key, char *out,
                                              size_t out_len) {
    if (!value || !key || !out || out_len == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (value->type != LLE_THEME_VALUE_TABLE) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    for (size_t i = 0; i < value->data.table.count; i++) {
        if (strcmp(value->data.table.entries[i].key, key) == 0) {
            const lle_theme_value_t *v = value->data.table.entries[i].value;
            if (v && v->type == LLE_THEME_VALUE_STRING) {
                snprintf(out, out_len, "%s", v->data.string);
                return LLE_SUCCESS;
            }
            return LLE_ERROR_INVALID_PARAMETER;
        }
    }

    return LLE_ERROR_NOT_FOUND;
}

/**
 * @brief Get an integer from a table value by key
 */
lle_result_t lle_theme_value_table_get_integer(const lle_theme_value_t *value,
                                               const char *key, int64_t *out) {
    if (!value || !key || !out) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (value->type != LLE_THEME_VALUE_TABLE) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    for (size_t i = 0; i < value->data.table.count; i++) {
        if (strcmp(value->data.table.entries[i].key, key) == 0) {
            const lle_theme_value_t *v = value->data.table.entries[i].value;
            if (v && v->type == LLE_THEME_VALUE_INTEGER) {
                *out = v->data.integer;
                return LLE_SUCCESS;
            }
            return LLE_ERROR_INVALID_PARAMETER;
        }
    }

    return LLE_ERROR_NOT_FOUND;
}

/**
 * @brief Get a boolean from a table value by key
 */
lle_result_t lle_theme_value_table_get_boolean(const lle_theme_value_t *value,
                                               const char *key, bool *out) {
    if (!value || !key || !out) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (value->type != LLE_THEME_VALUE_TABLE) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    for (size_t i = 0; i < value->data.table.count; i++) {
        if (strcmp(value->data.table.entries[i].key, key) == 0) {
            const lle_theme_value_t *v = value->data.table.entries[i].value;
            if (v && v->type == LLE_THEME_VALUE_BOOLEAN) {
                *out = v->data.boolean;
                return LLE_SUCCESS;
            }
            return LLE_ERROR_INVALID_PARAMETER;
        }
    }

    return LLE_ERROR_NOT_FOUND;
}

/* ============================================================================
 * Color Parsing
 * ============================================================================
 */

/* Basic ANSI color name lookup table */
static const struct {
    const char *name;
    lle_basic_color_t color;
} color_names[] = {{"black", LLE_COLOR_BLACK},
                   {"red", LLE_COLOR_RED},
                   {"green", LLE_COLOR_GREEN},
                   {"yellow", LLE_COLOR_YELLOW},
                   {"blue", LLE_COLOR_BLUE},
                   {"magenta", LLE_COLOR_MAGENTA},
                   {"cyan", LLE_COLOR_CYAN},
                   {"white", LLE_COLOR_WHITE},
                   {NULL, 0}};

/**
 * @brief Parse hex digit to integer
 */
static int hex_digit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

/**
 * @brief Parse a color specification string into lle_color_t
 */
lle_result_t lle_parse_color_spec(const char *spec, lle_color_t *color) {
    if (!spec || !color) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(color, 0, sizeof(*color));

    /* Skip leading whitespace */
    while (*spec == ' ' || *spec == '\t') {
        spec++;
    }

    size_t len = strlen(spec);
    if (len == 0) {
        return LLE_ERROR_INVALID_FORMAT;
    }

    /* Check for hex color: #RGB or #RRGGBB */
    if (spec[0] == '#') {
        if (len == 4) {
            /* #RGB -> expand to #RRGGBB */
            int r = hex_digit(spec[1]);
            int g = hex_digit(spec[2]);
            int b = hex_digit(spec[3]);
            if (r < 0 || g < 0 || b < 0) {
                return LLE_ERROR_INVALID_FORMAT;
            }
            color->mode = LLE_COLOR_MODE_TRUE;
            color->value.rgb.r = (uint8_t)(r * 17); /* 0xF -> 0xFF */
            color->value.rgb.g = (uint8_t)(g * 17);
            color->value.rgb.b = (uint8_t)(b * 17);
            return LLE_SUCCESS;
        } else if (len == 7) {
            /* #RRGGBB */
            int r1 = hex_digit(spec[1]);
            int r2 = hex_digit(spec[2]);
            int g1 = hex_digit(spec[3]);
            int g2 = hex_digit(spec[4]);
            int b1 = hex_digit(spec[5]);
            int b2 = hex_digit(spec[6]);
            if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0) {
                return LLE_ERROR_INVALID_FORMAT;
            }
            color->mode = LLE_COLOR_MODE_TRUE;
            color->value.rgb.r = (uint8_t)((r1 << 4) | r2);
            color->value.rgb.g = (uint8_t)((g1 << 4) | g2);
            color->value.rgb.b = (uint8_t)((b1 << 4) | b2);
            return LLE_SUCCESS;
        }
        return LLE_ERROR_INVALID_FORMAT;
    }

    /* Check for rgb(r, g, b) function */
    if (strncmp(spec, "rgb(", 4) == 0) {
        int r, g, b;
        if (sscanf(spec, "rgb(%d,%d,%d)", &r, &g, &b) == 3 ||
            sscanf(spec, "rgb( %d , %d , %d )", &r, &g, &b) == 3) {
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                return LLE_ERROR_INVALID_FORMAT;
            }
            color->mode = LLE_COLOR_MODE_TRUE;
            color->value.rgb.r = (uint8_t)r;
            color->value.rgb.g = (uint8_t)g;
            color->value.rgb.b = (uint8_t)b;
            return LLE_SUCCESS;
        }
        return LLE_ERROR_INVALID_FORMAT;
    }

    /* Check for basic color name */
    for (int i = 0; color_names[i].name != NULL; i++) {
        if (strcasecmp(spec, color_names[i].name) == 0) {
            color->mode = LLE_COLOR_MODE_BASIC;
            color->value.basic = (uint8_t)color_names[i].color;
            return LLE_SUCCESS;
        }
    }

    /* Check for numeric 256-color index */
    char *endptr;
    long index = strtol(spec, &endptr, 10);
    if (*endptr == '\0' && index >= 0 && index <= 255) {
        color->mode = LLE_COLOR_MODE_256;
        color->value.palette = (uint8_t)index;
        return LLE_SUCCESS;
    }

    return LLE_ERROR_INVALID_FORMAT;
}

/**
 * @brief Parse an inline table into a color with attributes
 */
lle_result_t lle_parse_color_table(const lle_theme_value_t *value,
                                   lle_color_t *color) {
    if (!value || !color) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (value->type != LLE_THEME_VALUE_TABLE) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(color, 0, sizeof(*color));

    /* Look for foreground color specification */
    char fg_spec[64] = {0};
    int64_t fg_int = -1;

    if (lle_theme_value_table_get_string(value, "fg", fg_spec,
                                         sizeof(fg_spec)) == LLE_SUCCESS) {
        lle_result_t result = lle_parse_color_spec(fg_spec, color);
        if (result != LLE_SUCCESS) {
            return result;
        }
    } else if (lle_theme_value_table_get_integer(value, "fg", &fg_int) ==
               LLE_SUCCESS) {
        if (fg_int >= 0 && fg_int <= 255) {
            color->mode = LLE_COLOR_MODE_256;
            color->value.palette = (uint8_t)fg_int;
        } else {
            return LLE_ERROR_INVALID_FORMAT;
        }
    }

    /* Parse attributes */
    bool attr;
    if (lle_theme_value_table_get_boolean(value, "bold", &attr) ==
        LLE_SUCCESS) {
        color->bold = attr;
    }
    if (lle_theme_value_table_get_boolean(value, "italic", &attr) ==
        LLE_SUCCESS) {
        color->italic = attr;
    }
    if (lle_theme_value_table_get_boolean(value, "underline", &attr) ==
        LLE_SUCCESS) {
        color->underline = attr;
    }
    if (lle_theme_value_table_get_boolean(value, "dim", &attr) == LLE_SUCCESS) {
        color->dim = attr;
    }

    return LLE_SUCCESS;
}

/* ============================================================================
 * Theme Construction
 * ============================================================================
 */

/**
 * @brief Theme builder context for callback
 */
typedef struct {
    lle_theme_t *theme;
    char error_msg[256];
    lle_result_t error;
} theme_builder_ctx_t;

/**
 * @brief Map category string to enum
 */
static lle_theme_category_t parse_category(const char *str) {
    if (strcasecmp(str, "minimal") == 0)
        return LLE_THEME_CATEGORY_MINIMAL;
    if (strcasecmp(str, "classic") == 0)
        return LLE_THEME_CATEGORY_CLASSIC;
    if (strcasecmp(str, "modern") == 0)
        return LLE_THEME_CATEGORY_MODERN;
    if (strcasecmp(str, "powerline") == 0)
        return LLE_THEME_CATEGORY_POWERLINE;
    if (strcasecmp(str, "professional") == 0)
        return LLE_THEME_CATEGORY_PROFESSIONAL;
    if (strcasecmp(str, "creative") == 0)
        return LLE_THEME_CATEGORY_CREATIVE;
    return LLE_THEME_CATEGORY_CUSTOM;
}

/**
 * @brief Apply a color value to a color field
 */
static lle_result_t apply_color(const lle_theme_value_t *value,
                                lle_color_t *color) {
    if (value->type == LLE_THEME_VALUE_STRING) {
        return lle_parse_color_spec(value->data.string, color);
    } else if (value->type == LLE_THEME_VALUE_INTEGER) {
        if (value->data.integer >= 0 && value->data.integer <= 255) {
            color->mode = LLE_COLOR_MODE_256;
            color->value.palette = (uint8_t)value->data.integer;
            return LLE_SUCCESS;
        }
        return LLE_ERROR_INVALID_FORMAT;
    } else if (value->type == LLE_THEME_VALUE_TABLE) {
        return lle_parse_color_table(value, color);
    }
    return LLE_ERROR_INVALID_FORMAT;
}

/**
 * @brief Theme builder callback - populates theme from parsed values
 */
static lle_result_t theme_builder_callback(const char *section, const char *key,
                                           const lle_theme_value_t *value,
                                           void *user_data) {
    theme_builder_ctx_t *ctx = (theme_builder_ctx_t *)user_data;
    lle_theme_t *theme = ctx->theme;

    /* [theme] section - metadata */
    if (strcmp(section, "theme") == 0) {
        if (strcmp(key, "name") == 0 && value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->name, sizeof(theme->name), "%.63s",
                     value->data.string);
        } else if (strcmp(key, "description") == 0 &&
                   value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->description, sizeof(theme->description), "%.255s",
                     value->data.string);
        } else if (strcmp(key, "author") == 0 &&
                   value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->author, sizeof(theme->author), "%.63s",
                     value->data.string);
        } else if (strcmp(key, "version") == 0 &&
                   value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->version, sizeof(theme->version), "%.15s",
                     value->data.string);
        } else if (strcmp(key, "category") == 0 &&
                   value->type == LLE_THEME_VALUE_STRING) {
            theme->category = parse_category(value->data.string);
        } else if (strcmp(key, "inherits_from") == 0 &&
                   value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->inherits_from, sizeof(theme->inherits_from), "%.63s",
                     value->data.string);
        }
        return LLE_SUCCESS;
    }

    /* [capabilities] section */
    if (strcmp(section, "capabilities") == 0) {
        if (value->type == LLE_THEME_VALUE_BOOLEAN && value->data.boolean) {
            if (strcmp(key, "unicode") == 0) {
                theme->capabilities |= LLE_THEME_CAP_UNICODE;
            } else if (strcmp(key, "multiline") == 0) {
                theme->capabilities |= LLE_THEME_CAP_MULTILINE;
            } else if (strcmp(key, "transient") == 0) {
                theme->capabilities |= LLE_THEME_CAP_TRANSIENT;
            } else if (strcmp(key, "async_segments") == 0) {
                theme->capabilities |= LLE_THEME_CAP_ASYNC_SEGMENTS;
            } else if (strcmp(key, "powerline") == 0) {
                theme->capabilities |= LLE_THEME_CAP_POWERLINE;
            } else if (strcmp(key, "nerd_font") == 0) {
                theme->capabilities |= LLE_THEME_CAP_NERD_FONT;
            } else if (strcmp(key, "right_prompt") == 0) {
                theme->capabilities |= LLE_THEME_CAP_RIGHT_PROMPT;
            } else if (strcmp(key, "true_color") == 0) {
                theme->capabilities |= LLE_THEME_CAP_TRUE_COLOR;
            } else if (strcmp(key, "256_color") == 0) {
                theme->capabilities |= LLE_THEME_CAP_256_COLOR;
            } else if (strcmp(key, "ascii_fallback") == 0) {
                theme->capabilities |= LLE_THEME_CAP_ASCII_FALLBACK;
            }
        }
        return LLE_SUCCESS;
    }

    /* [layout] section */
    if (strcmp(section, "layout") == 0) {
        if (strcmp(key, "ps1") == 0 && value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->layout.ps1_format, sizeof(theme->layout.ps1_format),
                     "%s", value->data.string);
        } else if (strcmp(key, "ps2") == 0 &&
                   value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->layout.ps2_format, sizeof(theme->layout.ps2_format),
                     "%s", value->data.string);
        } else if (strcmp(key, "rprompt") == 0 &&
                   value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->layout.rps1_format,
                     sizeof(theme->layout.rps1_format), "%s",
                     value->data.string);
            theme->layout.enable_right_prompt = true;
        } else if (strcmp(key, "transient") == 0 &&
                   value->type == LLE_THEME_VALUE_STRING) {
            snprintf(theme->layout.transient_format,
                     sizeof(theme->layout.transient_format), "%s",
                     value->data.string);
            theme->layout.enable_transient = true;
        } else if (strcmp(key, "newline_before") == 0 &&
                   value->type == LLE_THEME_VALUE_BOOLEAN) {
            theme->layout.newline_before = value->data.boolean ? 1 : 0;
        } else if (strcmp(key, "newline_after") == 0 &&
                   value->type == LLE_THEME_VALUE_BOOLEAN) {
            theme->layout.newline_after = value->data.boolean ? 1 : 0;
        } else if (strcmp(key, "multiline") == 0 &&
                   value->type == LLE_THEME_VALUE_BOOLEAN) {
            theme->layout.enable_multiline = value->data.boolean;
        } else if (strcmp(key, "compact") == 0 &&
                   value->type == LLE_THEME_VALUE_BOOLEAN) {
            theme->layout.compact_mode = value->data.boolean;
        }
        return LLE_SUCCESS;
    }

    /* [segments] section */
    if (strcmp(section, "segments") == 0) {
        if (strcmp(key, "enabled") == 0 &&
            value->type == LLE_THEME_VALUE_ARRAY) {
            theme->enabled_segment_count = 0;
            for (size_t i = 0;
                 i < value->data.array.count && i < LLE_THEME_MAX_SEGMENTS;
                 i++) {
                if (value->data.array.items[i].type == LLE_THEME_VALUE_STRING) {
                    snprintf(theme->enabled_segments[i],
                             sizeof(theme->enabled_segments[i]), "%.31s",
                             value->data.array.items[i].data.string);
                    theme->enabled_segment_count++;
                }
            }
        }
        return LLE_SUCCESS;
    }

    /* [colors] section */
    if (strcmp(section, "colors") == 0) {
        lle_color_t *target = NULL;

        /* Map key to color field */
        if (strcmp(key, "primary") == 0)
            target = &theme->colors.primary;
        else if (strcmp(key, "secondary") == 0)
            target = &theme->colors.secondary;
        else if (strcmp(key, "success") == 0)
            target = &theme->colors.success;
        else if (strcmp(key, "warning") == 0)
            target = &theme->colors.warning;
        else if (strcmp(key, "error") == 0)
            target = &theme->colors.error;
        else if (strcmp(key, "info") == 0)
            target = &theme->colors.info;
        else if (strcmp(key, "text") == 0)
            target = &theme->colors.text;
        else if (strcmp(key, "text_dim") == 0)
            target = &theme->colors.text_dim;
        else if (strcmp(key, "text_bright") == 0)
            target = &theme->colors.text_bright;
        else if (strcmp(key, "border") == 0)
            target = &theme->colors.border;
        else if (strcmp(key, "background") == 0)
            target = &theme->colors.background;
        else if (strcmp(key, "highlight") == 0)
            target = &theme->colors.highlight;
        else if (strcmp(key, "git_clean") == 0)
            target = &theme->colors.git_clean;
        else if (strcmp(key, "git_dirty") == 0)
            target = &theme->colors.git_dirty;
        else if (strcmp(key, "git_staged") == 0)
            target = &theme->colors.git_staged;
        else if (strcmp(key, "git_untracked") == 0)
            target = &theme->colors.git_untracked;
        else if (strcmp(key, "git_branch") == 0)
            target = &theme->colors.git_branch;
        else if (strcmp(key, "git_ahead") == 0)
            target = &theme->colors.git_ahead;
        else if (strcmp(key, "git_behind") == 0)
            target = &theme->colors.git_behind;
        else if (strcmp(key, "path_home") == 0)
            target = &theme->colors.path_home;
        else if (strcmp(key, "path_root") == 0)
            target = &theme->colors.path_root;
        else if (strcmp(key, "path_normal") == 0)
            target = &theme->colors.path_normal;
        else if (strcmp(key, "path_separator") == 0)
            target = &theme->colors.path_separator;
        else if (strcmp(key, "status_ok") == 0)
            target = &theme->colors.status_ok;
        else if (strcmp(key, "status_error") == 0)
            target = &theme->colors.status_error;
        else if (strcmp(key, "status_running") == 0)
            target = &theme->colors.status_running;

        if (target) {
            lle_result_t result = apply_color(value, target);
            if (result != LLE_SUCCESS) {
                snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                         "Invalid color value for '%s'", key);
                ctx->error = result;
                return result;
            }
        }
        return LLE_SUCCESS;
    }

    /* [symbols] section */
    if (strcmp(section, "symbols") == 0 &&
        value->type == LLE_THEME_VALUE_STRING) {
        char *target = NULL;
        size_t target_size = LLE_SYMBOL_MAX;

        if (strcmp(key, "prompt") == 0)
            target = theme->symbols.prompt;
        else if (strcmp(key, "prompt_root") == 0)
            target = theme->symbols.prompt_root;
        else if (strcmp(key, "continuation") == 0)
            target = theme->symbols.continuation;
        else if (strcmp(key, "separator_left") == 0)
            target = theme->symbols.separator_left;
        else if (strcmp(key, "separator_right") == 0)
            target = theme->symbols.separator_right;
        else if (strcmp(key, "branch") == 0)
            target = theme->symbols.branch;
        else if (strcmp(key, "staged") == 0)
            target = theme->symbols.staged;
        else if (strcmp(key, "unstaged") == 0)
            target = theme->symbols.unstaged;
        else if (strcmp(key, "untracked") == 0)
            target = theme->symbols.untracked;
        else if (strcmp(key, "ahead") == 0)
            target = theme->symbols.ahead;
        else if (strcmp(key, "behind") == 0)
            target = theme->symbols.behind;
        else if (strcmp(key, "stash") == 0)
            target = theme->symbols.stash;
        else if (strcmp(key, "conflict") == 0)
            target = theme->symbols.conflict;
        else if (strcmp(key, "directory") == 0)
            target = theme->symbols.directory;
        else if (strcmp(key, "home") == 0)
            target = theme->symbols.home;
        else if (strcmp(key, "error") == 0)
            target = theme->symbols.error;
        else if (strcmp(key, "success") == 0)
            target = theme->symbols.success;
        else if (strcmp(key, "time") == 0)
            target = theme->symbols.time;
        else if (strcmp(key, "jobs") == 0)
            target = theme->symbols.jobs;

        if (target) {
            /* Limit to LLE_SYMBOL_MAX-1 chars to prevent truncation warnings */
            snprintf(target, target_size, "%.15s", value->data.string);
        }
        return LLE_SUCCESS;
    }

    /* [symbols.ascii] section - ASCII fallback symbols */
    if (strcmp(section, "symbols.ascii") == 0 &&
        value->type == LLE_THEME_VALUE_STRING) {
        /* Note: ASCII fallbacks are stored in the same symbol set when
         * the theme is used in ASCII mode. For now, we just ignore these
         * during parsing and let the theme loader handle mode selection. */
        return LLE_SUCCESS;
    }

    /* Unknown section/key - ignore (allows future extensions) */
    return LLE_SUCCESS;
}

/**
 * @brief Parse input directly into a theme structure
 */
lle_result_t lle_theme_parser_parse_to_theme(lle_theme_parser_t *parser,
                                             lle_theme_t *theme) {
    if (!parser || !theme) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    theme_builder_ctx_t ctx = {
        .theme = theme, .error_msg = {0}, .error = LLE_SUCCESS};

    lle_result_t result =
        lle_theme_parser_parse(parser, theme_builder_callback, &ctx);

    if (result != LLE_SUCCESS) {
        return result;
    }

    if (ctx.error != LLE_SUCCESS) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), "%s",
                 ctx.error_msg);
        return ctx.error;
    }

    return LLE_SUCCESS;
}

/* ============================================================================
 * Theme Validation
 * ============================================================================
 */

/**
 * @brief Validate a parsed theme structure
 */
lle_result_t lle_theme_parser_validate(const lle_theme_t *theme,
                                       char *error_buf, size_t error_buf_len) {
    if (!theme) {
        if (error_buf && error_buf_len > 0) {
            snprintf(error_buf, error_buf_len, "Theme is NULL");
        }
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Theme name is required */
    if (strlen(theme->name) == 0) {
        if (error_buf && error_buf_len > 0) {
            snprintf(error_buf, error_buf_len, "Theme name is required");
        }
        return LLE_ERROR_INVALID_FORMAT;
    }

    /* Validate theme name characters */
    for (size_t i = 0; theme->name[i] != '\0'; i++) {
        char c = theme->name[i];
        if (!isalnum((unsigned char)c) && c != '_' && c != '-') {
            if (error_buf && error_buf_len > 0) {
                snprintf(error_buf, error_buf_len,
                         "Theme name contains invalid character '%c'", c);
            }
            return LLE_ERROR_INVALID_FORMAT;
        }
    }

    /* PS1 format is recommended but not required (will inherit from parent) */

    return LLE_SUCCESS;
}
