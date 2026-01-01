/*
 * Lusush Shell - LLE Source Manager Implementation
 * Copyright (C) 2021-2026  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ============================================================================
 *
 * SOURCE MANAGER IMPLEMENTATION - Spec 12 Core
 *
 * Manages completion sources and orchestrates querying.
 */

#include "lle/completion/source_manager.h"
#include "lle/completion/completion_generator.h" /* For existing source functions */
#include "lle/completion/completion_sources.h" /* For lle_completion_source_aliases */
#include <string.h>

// ============================================================================
// SOURCE APPLICABILITY FUNCTIONS
// ============================================================================

/**
 * Builtin source: Only applicable at command position
 */
static bool builtin_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_COMMAND;
}

/**
 * Alias source: Only applicable at command position
 */
static bool alias_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_COMMAND;
}

/**
 * External command source: Only applicable at command position
 */
static bool
external_command_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_COMMAND;
}

/**
 * File source: Applicable for arguments and redirects
 */
static bool file_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_ARGUMENT ||
           context->type == LLE_CONTEXT_REDIRECT;
}

/**
 * Variable source: Only applicable for $VAR
 */
static bool variable_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_VARIABLE;
}

/**
 * History source: Always applicable as fallback
 */
static bool history_source_applicable(const lle_context_analyzer_t *context) {
    (void)context; /* Unused */
    return true;   /* Always available */
}

// ============================================================================
// SOURCE GENERATION FUNCTIONS
// ============================================================================

/**
 * Builtin command source - ONLY builtins (no external commands)
 */
static lle_result_t
builtin_source_generate(lle_memory_pool_t *pool,
                        const lle_context_analyzer_t *context,
                        const char *prefix, lle_completion_result_t *result) {
    (void)context; /* Not needed for builtins */

    /* Call ONLY the builtin source function to avoid duplicates */
    return lle_completion_source_builtins(pool, prefix, result);
}

/**
 * Alias command source - shell aliases
 */
static lle_result_t alias_source_generate(lle_memory_pool_t *pool,
                                          const lle_context_analyzer_t *context,
                                          const char *prefix,
                                          lle_completion_result_t *result) {
    (void)context; /* Not needed for aliases */

    /* Call the alias source function */
    return lle_completion_source_aliases(pool, prefix, result);
}

/**
 * External command source - ONLY PATH commands (no builtins)
 */
static lle_result_t external_command_source_generate(
    lle_memory_pool_t *pool, const lle_context_analyzer_t *context,
    const char *prefix, lle_completion_result_t *result) {
    (void)context; /* Not needed for commands */

    /* Call ONLY the external command source to avoid duplicates with builtins
     */
    return lle_completion_source_commands(pool, prefix, result);
}

/**
 * Check if command expects only directory arguments
 */
static bool is_directory_only_command(const char *command_name) {
    if (!command_name) {
        return false;
    }

    /*
     * POSIX commands that only accept directory arguments.
     * Note: pushd/popd are bash extensions, not implemented in lusush.
     */
    static const char *dir_commands[] = {"cd", "rmdir", NULL};

    for (const char **cmd = dir_commands; *cmd != NULL; cmd++) {
        if (strcmp(command_name, *cmd) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * File/directory source - command-aware
 *
 * For commands like cd, rmdir: only show directories
 * For other commands: show both files and directories
 */
static lle_result_t file_source_generate(lle_memory_pool_t *pool,
                                         const lle_context_analyzer_t *context,
                                         const char *prefix,
                                         lle_completion_result_t *result) {
    /* Check if command expects directories only */
    if (context && is_directory_only_command(context->command_name)) {
        return lle_completion_source_directories(pool, prefix, result);
    }

    /* Default: show both files and directories */
    return lle_completion_source_files(pool, prefix, result);
}

/**
 * Variable source
 */
static lle_result_t
variable_source_generate(lle_memory_pool_t *pool,
                         const lle_context_analyzer_t *context,
                         const char *prefix, lle_completion_result_t *result) {
    (void)context; /* Not needed for variables */

    /* Use existing completion function */
    return lle_completion_generate_variables(pool, prefix, result);
}

/**
 * History source
 */
static lle_result_t
history_source_generate(lle_memory_pool_t *pool,
                        const lle_context_analyzer_t *context,
                        const char *prefix, lle_completion_result_t *result) {
    (void)context; /* Not needed for history */

    /* Use existing completion function */
    return lle_completion_source_history(pool, prefix, result);
}

// ============================================================================
// PUBLIC API
// ============================================================================

lle_result_t lle_source_manager_create(lle_memory_pool_t *pool,
                                       lle_source_manager_t **out_manager) {
    if (!pool || !out_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_source_manager_t *manager = lle_pool_alloc(sizeof(*manager));
    if (!manager) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    manager->num_sources = 0;
    manager->pool = pool;

    /* Register default sources */
    lle_result_t res;

    res = lle_source_manager_register(manager, LLE_SOURCE_BUILTINS, "builtins",
                                      builtin_source_generate,
                                      builtin_source_applicable);
    if (res != LLE_SUCCESS) {
        return res;
    }

    res = lle_source_manager_register(manager, LLE_SOURCE_ALIASES, "aliases",
                                      alias_source_generate,
                                      alias_source_applicable);
    if (res != LLE_SUCCESS) {
        return res;
    }

    res = lle_source_manager_register(
        manager, LLE_SOURCE_EXTERNAL_COMMANDS, "external_commands",
        external_command_source_generate, external_command_source_applicable);
    if (res != LLE_SUCCESS) {
        return res;
    }

    res = lle_source_manager_register(manager, LLE_SOURCE_FILES, "files",
                                      file_source_generate,
                                      file_source_applicable);
    if (res != LLE_SUCCESS) {
        return res;
    }

    res = lle_source_manager_register(manager, LLE_SOURCE_VARIABLES,
                                      "variables", variable_source_generate,
                                      variable_source_applicable);
    if (res != LLE_SUCCESS) {
        return res;
    }

    res = lle_source_manager_register(manager, LLE_SOURCE_HISTORY, "history",
                                      history_source_generate,
                                      history_source_applicable);
    if (res != LLE_SUCCESS) {
        return res;
    }

    *out_manager = manager;
    return LLE_SUCCESS;
}

void lle_source_manager_free(lle_source_manager_t *manager) {
    if (!manager) {
        return;
    }

    /* Memory is pool-allocated, will be freed with pool */
    (void)manager;
}

lle_result_t
lle_source_manager_register(lle_source_manager_t *manager,
                            lle_source_type_t type, const char *name,
                            lle_source_generate_fn generate_fn,
                            lle_source_applicable_fn applicable_fn) {
    if (!manager || !name || !generate_fn) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (manager->num_sources >= MAX_COMPLETION_SOURCES) {
        return LLE_ERROR_BUFFER_OVERFLOW; /* Array full */
    }

    lle_completion_source_t *source = lle_pool_alloc(sizeof(*source));
    if (!source) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    source->type = type;
    source->name = name;
    source->generate = generate_fn;
    source->is_applicable = applicable_fn;
    source->user_data = NULL;

    manager->sources[manager->num_sources++] = source;
    return LLE_SUCCESS;
}

lle_result_t lle_source_manager_query(lle_source_manager_t *manager,
                                      const lle_context_analyzer_t *context,
                                      const char *prefix,
                                      lle_completion_result_t *result) {
    if (!manager || !context || !prefix || !result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Query all applicable sources */
    for (size_t i = 0; i < manager->num_sources; i++) {
        lle_completion_source_t *source = manager->sources[i];

        /* Check if source is applicable for this context */
        if (source->is_applicable && !source->is_applicable(context)) {
            continue; /* Skip this source */
        }

        /* Query source for completions */
        lle_result_t res =
            source->generate(manager->pool, context, prefix, result);

        /* Continue even if source fails - other sources may succeed */
        (void)res;
    }

    return LLE_SUCCESS;
}
