/**
 * LLE Prompt Template Engine
 *
 * Spec 25 Section 6: Template Engine
 * Provides expressive format strings with segment references,
 * conditionals, color application, and property access.
 *
 * Template Syntax:
 *   ${segment}              - Render segment (e.g., ${directory}, ${git})
 *   ${segment.property}     - Access segment property (e.g., ${git.branch})
 *   ${?segment:true:false}  - Conditional: if segment visible, show true, else false
 *   ${?segment.prop:t:f}    - Conditional on property existence
 *   ${color:text}           - Apply theme color to text (e.g., ${primary:hello})
 *   \n                      - Literal newline
 *   \\                      - Escaped backslash
 *   \$                      - Escaped dollar sign
 */

#ifndef LLE_PROMPT_TEMPLATE_H
#define LLE_PROMPT_TEMPLATE_H

#include "lle/error_handling.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum sizes */
#define LLE_TEMPLATE_MAX           1024
#define LLE_TEMPLATE_TOKEN_MAX     256
#define LLE_TEMPLATE_SEGMENT_MAX   32
#define LLE_TEMPLATE_PROPERTY_MAX  32
#define LLE_TEMPLATE_LITERAL_MAX   256

/**
 * Template token types
 */
typedef enum lle_template_token_type {
    LLE_TOKEN_LITERAL,           /* Plain text */
    LLE_TOKEN_SEGMENT,           /* ${segment} */
    LLE_TOKEN_PROPERTY,          /* ${segment.property} */
    LLE_TOKEN_CONDITIONAL,       /* ${?condition:true:false} */
    LLE_TOKEN_COLOR,             /* ${color:text} */
    LLE_TOKEN_NEWLINE,           /* \n */
    LLE_TOKEN_END                /* End of template */
} lle_template_token_type_t;

/**
 * Parsed template token
 */
typedef struct lle_template_token {
    lle_template_token_type_t type;

    union {
        /* Literal text */
        struct {
            char text[LLE_TEMPLATE_LITERAL_MAX];
            size_t length;
        } literal;

        /* Segment reference */
        struct {
            char segment_name[LLE_TEMPLATE_SEGMENT_MAX];
            char property_name[LLE_TEMPLATE_PROPERTY_MAX];
            bool has_property;
        } segment;

        /* Conditional */
        struct {
            char condition_segment[LLE_TEMPLATE_SEGMENT_MAX];
            char condition_property[LLE_TEMPLATE_PROPERTY_MAX];
            char true_value[LLE_TEMPLATE_LITERAL_MAX];
            char false_value[LLE_TEMPLATE_LITERAL_MAX];
            bool check_property;
        } conditional;

        /* Color application */
        struct {
            char color_name[LLE_TEMPLATE_SEGMENT_MAX];
            char text[LLE_TEMPLATE_LITERAL_MAX];
        } color;
    } data;

    struct lle_template_token *next;
} lle_template_token_t;

/**
 * Parsed template (linked list of tokens)
 */
typedef struct lle_parsed_template {
    lle_template_token_t *head;
    lle_template_token_t *tail;
    size_t token_count;
    char original[LLE_TEMPLATE_MAX];
    bool valid;
} lle_parsed_template_t;

/**
 * Forward declarations for render context
 */
struct lle_segment_registry;
struct lle_prompt_context;
struct lle_theme;

/* ========================================================================== */
/* Template Parsing API                                                       */
/* ========================================================================== */

/**
 * Parse a template string into tokens
 *
 * @param template_str  Template string to parse
 * @param parsed        Output: parsed template structure
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_template_parse(const char *template_str,
                                 lle_parsed_template_t **parsed);

/**
 * Free a parsed template and all its tokens
 *
 * @param parsed  Parsed template to free
 */
void lle_template_free(lle_parsed_template_t *parsed);

/**
 * Validate a template string without full parsing
 *
 * @param template_str  Template string to validate
 * @return true if template appears valid, false otherwise
 */
bool lle_template_validate(const char *template_str);

/* ========================================================================== */
/* Template Rendering API                                                     */
/* ========================================================================== */

/**
 * Segment value provider callback
 *
 * Called by the template renderer to get segment output.
 * Returns the rendered content for a segment, or NULL if segment not found.
 *
 * @param segment_name  Name of the segment to render
 * @param property      Property name (NULL for full segment)
 * @param user_data     User-provided context
 * @return Rendered content (caller must free) or NULL
 */
typedef char *(*lle_template_segment_provider_t)(const char *segment_name,
                                                  const char *property,
                                                  void *user_data);

/**
 * Segment visibility check callback
 *
 * Called by the template renderer to check if a segment is visible.
 *
 * @param segment_name  Name of the segment
 * @param property      Property name (NULL for segment visibility)
 * @param user_data     User-provided context
 * @return true if segment/property is visible, false otherwise
 */
typedef bool (*lle_template_segment_visible_t)(const char *segment_name,
                                                const char *property,
                                                void *user_data);

/**
 * Color provider callback
 *
 * Called by the template renderer to get ANSI color codes.
 *
 * @param color_name  Semantic color name (e.g., "primary", "git_clean")
 * @param user_data   User-provided context
 * @return ANSI color code string (static, do not free) or empty string
 */
typedef const char *(*lle_template_color_provider_t)(const char *color_name,
                                                      void *user_data);

/**
 * Template render context
 *
 * Provides callbacks for segment rendering, visibility checking,
 * and color lookup during template evaluation.
 */
typedef struct lle_template_render_ctx {
    lle_template_segment_provider_t get_segment;
    lle_template_segment_visible_t is_visible;
    lle_template_color_provider_t get_color;
    void *user_data;
} lle_template_render_ctx_t;

/**
 * Render a parsed template to output string
 *
 * @param tmpl         Parsed template
 * @param render_ctx   Render context with callbacks
 * @param output       Output buffer
 * @param output_size  Output buffer size
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_template_render(const lle_parsed_template_t *tmpl,
                                  const lle_template_render_ctx_t *render_ctx,
                                  char *output,
                                  size_t output_size);

/**
 * Parse and render a template in one call
 *
 * Convenience function that parses, renders, and frees the template.
 *
 * @param template_str  Template string
 * @param render_ctx    Render context with callbacks
 * @param output        Output buffer
 * @param output_size   Output buffer size
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_template_evaluate(const char *template_str,
                                    const lle_template_render_ctx_t *render_ctx,
                                    char *output,
                                    size_t output_size);

/* ========================================================================== */
/* Token Creation Helpers (Internal)                                          */
/* ========================================================================== */

/**
 * Create a literal token
 */
lle_template_token_t *lle_template_token_literal(const char *text, size_t len);

/**
 * Create a segment token
 */
lle_template_token_t *lle_template_token_segment(const char *name);

/**
 * Create a property token
 */
lle_template_token_t *lle_template_token_property(const char *segment,
                                                   const char *property);

/**
 * Create a conditional token
 */
lle_template_token_t *lle_template_token_conditional(const char *segment,
                                                      const char *property,
                                                      const char *true_val,
                                                      const char *false_val);

/**
 * Create a color token
 */
lle_template_token_t *lle_template_token_color(const char *color,
                                                const char *text);

/**
 * Create a newline token
 */
lle_template_token_t *lle_template_token_newline(void);

/**
 * Create an end token
 */
lle_template_token_t *lle_template_token_end(void);

/**
 * Free a single token
 */
void lle_template_token_free(lle_template_token_t *token);

#ifdef __cplusplus
}
#endif

#endif /* LLE_PROMPT_TEMPLATE_H */
