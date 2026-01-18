/**
 * @file async_worker.c
 * @brief LLE Async Worker Thread Pool Implementation
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 *
 * Implements a pthread-based worker thread for async operations.
 *
 * Specification: docs/lle_specification/25_prompt_theme_system_complete.md
 * Section: 7 - Async Operations
 */

#include "lle/async_worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * INTERNAL DECLARATIONS
 * ============================================================================
 */

/**
 * @brief Worker thread main function
 * @param arg Worker context (lle_async_worker_t *)
 * @return Always NULL
 */
static void *lle_async_worker_thread(void *arg);

/**
 * @brief Get git repository status
 * @param cwd Working directory
 * @param timeout_ms Timeout in milliseconds
 * @param status Output status structure
 * @return Result code
 */
static lle_result_t lle_async_get_git_status(const char *cwd,
                                             uint32_t timeout_ms,
                                             lle_git_status_data_t *status);

/* ============================================================================
 * WORKER LIFECYCLE
 * ============================================================================
 */

lle_result_t lle_async_worker_init(lle_async_worker_t **worker,
                                   lle_async_completion_fn on_complete,
                                   void *user_data) {
    if (!worker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_async_worker_t *w = calloc(1, sizeof(*w));
    if (!w) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    if (pthread_mutex_init(&w->queue_mutex, NULL) != 0) {
        free(w);
        return LLE_ERROR_SYSTEM_CALL;
    }

    if (pthread_cond_init(&w->queue_cond, NULL) != 0) {
        pthread_mutex_destroy(&w->queue_mutex);
        free(w);
        return LLE_ERROR_SYSTEM_CALL;
    }

    w->on_complete = on_complete;
    w->callback_user_data = user_data;
    w->running = false;
    w->shutdown_requested = false;
    w->next_request_id = 1;
    w->queue_head = NULL;
    w->queue_tail = NULL;
    w->queue_size = 0;
    w->total_requests = 0;
    w->total_completed = 0;
    w->total_timeouts = 0;

    *worker = w;
    return LLE_SUCCESS;
}

lle_result_t lle_async_worker_start(lle_async_worker_t *worker) {
    if (!worker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&worker->queue_mutex);
    if (worker->running) {
        pthread_mutex_unlock(&worker->queue_mutex);
        return LLE_ERROR_INVALID_PARAMETER;
    }

    worker->running = true;
    worker->shutdown_requested = false;
    pthread_mutex_unlock(&worker->queue_mutex);

    if (pthread_create(&worker->thread, NULL, lle_async_worker_thread,
                       worker) != 0) {
        pthread_mutex_lock(&worker->queue_mutex);
        worker->running = false;
        pthread_mutex_unlock(&worker->queue_mutex);
        return LLE_ERROR_SYSTEM_CALL;
    }

    return LLE_SUCCESS;
}

lle_result_t lle_async_worker_shutdown(lle_async_worker_t *worker) {
    if (!worker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&worker->queue_mutex);
    worker->shutdown_requested = true;
    pthread_cond_signal(&worker->queue_cond);
    pthread_mutex_unlock(&worker->queue_mutex);

    return LLE_SUCCESS;
}

lle_result_t lle_async_worker_wait(lle_async_worker_t *worker) {
    if (!worker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&worker->queue_mutex);
    bool was_running = worker->running;
    pthread_mutex_unlock(&worker->queue_mutex);

    if (was_running) {
        pthread_join(worker->thread, NULL);

        pthread_mutex_lock(&worker->queue_mutex);
        worker->running = false;
        pthread_mutex_unlock(&worker->queue_mutex);
    }

    return LLE_SUCCESS;
}

lle_result_t lle_async_worker_destroy(lle_async_worker_t *worker) {
    if (!worker) {
        return LLE_SUCCESS;
    }

    /* Free any pending requests */
    pthread_mutex_lock(&worker->queue_mutex);
    lle_async_request_t *req = worker->queue_head;
    while (req) {
        lle_async_request_t *next = req->next;
        free(req);
        req = next;
    }
    worker->queue_head = NULL;
    worker->queue_tail = NULL;
    worker->queue_size = 0;
    pthread_mutex_unlock(&worker->queue_mutex);

    pthread_mutex_destroy(&worker->queue_mutex);
    pthread_cond_destroy(&worker->queue_cond);

    free(worker);
    return LLE_SUCCESS;
}

/* ============================================================================
 * REQUEST MANAGEMENT
 * ============================================================================
 */

lle_async_request_t *lle_async_request_create(lle_async_request_type_t type) {
    lle_async_request_t *req = calloc(1, sizeof(*req));
    if (!req) {
        return NULL;
    }

    req->type = type;
    req->timeout_ms = LLE_ASYNC_DEFAULT_TIMEOUT_MS;
    req->id = 0; /* Assigned by worker on submit */
    req->next = NULL;
    req->user_data = NULL;
    req->cwd[0] = '\0';

    return req;
}

void lle_async_request_free(lle_async_request_t *request) { free(request); }

lle_result_t lle_async_worker_submit(lle_async_worker_t *worker,
                                     lle_async_request_t *request) {
    if (!worker || !request) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&worker->queue_mutex);

    if (!worker->running || worker->shutdown_requested) {
        pthread_mutex_unlock(&worker->queue_mutex);
        return LLE_ERROR_INVALID_STATE;
    }

    if (worker->queue_size >= LLE_ASYNC_MAX_QUEUE_SIZE) {
        pthread_mutex_unlock(&worker->queue_mutex);
        return LLE_ERROR_RESOURCE_EXHAUSTED;
    }

    /* Assign request ID */
    request->id = worker->next_request_id++;
    request->next = NULL;

    /* Enqueue */
    if (worker->queue_tail) {
        worker->queue_tail->next = request;
    } else {
        worker->queue_head = request;
    }
    worker->queue_tail = request;
    worker->queue_size++;
    worker->total_requests++;

    pthread_cond_signal(&worker->queue_cond);
    pthread_mutex_unlock(&worker->queue_mutex);

    return LLE_SUCCESS;
}

/* ============================================================================
 * QUERY FUNCTIONS
 * ============================================================================
 */

bool lle_async_worker_is_running(const lle_async_worker_t *worker) {
    if (!worker) {
        return false;
    }

    /* Note: This is a simple check without locking for performance.
     * The running flag is set atomically by the worker lifecycle functions. */
    return worker->running && !worker->shutdown_requested;
}

size_t lle_async_worker_pending_count(const lle_async_worker_t *worker) {
    if (!worker) {
        return 0;
    }

    /* Note: queue_size is updated under mutex, but this read is safe
     * for informational purposes. */
    return worker->queue_size;
}

lle_result_t lle_async_worker_get_stats(const lle_async_worker_t *worker,
                                        uint64_t *total_requests,
                                        uint64_t *total_completed,
                                        uint64_t *total_timeouts) {
    if (!worker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (total_requests) {
        *total_requests = worker->total_requests;
    }
    if (total_completed) {
        *total_completed = worker->total_completed;
    }
    if (total_timeouts) {
        *total_timeouts = worker->total_timeouts;
    }

    return LLE_SUCCESS;
}

/* ============================================================================
 * WORKER THREAD
 * ============================================================================
 */

/**
 * @brief Worker thread main function
 *
 * Main loop for the async worker thread. Waits for requests on the queue,
 * processes them, and invokes the completion callback with results.
 *
 * @param arg Worker context pointer (lle_async_worker_t *)
 * @return Always returns NULL
 */
static void *lle_async_worker_thread(void *arg) {
    lle_async_worker_t *worker = arg;

    while (1) {
        lle_async_request_t *request = NULL;

        /* Wait for work */
        pthread_mutex_lock(&worker->queue_mutex);
        while (worker->queue_head == NULL && !worker->shutdown_requested) {
            pthread_cond_wait(&worker->queue_cond, &worker->queue_mutex);
        }

        if (worker->shutdown_requested && worker->queue_head == NULL) {
            pthread_mutex_unlock(&worker->queue_mutex);
            break;
        }

        /* Dequeue request */
        request = worker->queue_head;
        if (request) {
            worker->queue_head = request->next;
            if (worker->queue_head == NULL) {
                worker->queue_tail = NULL;
            }
            worker->queue_size--;
        }
        pthread_mutex_unlock(&worker->queue_mutex);

        if (!request) {
            continue;
        }

        /* Process request */
        lle_async_response_t response;
        memset(&response, 0, sizeof(response));
        response.id = request->id;

        switch (request->type) {
        case LLE_ASYNC_GIT_STATUS:
            response.result = lle_async_get_git_status(
                request->cwd, request->timeout_ms, &response.data.git_status);
            break;

        case LLE_ASYNC_CUSTOM:
            /* Custom requests not yet implemented */
            response.result = LLE_ERROR_FEATURE_NOT_AVAILABLE;
            break;

        default:
            response.result = LLE_ERROR_INVALID_PARAMETER;
            break;
        }

        /* Update stats before callback so they're visible when callback signals */
        pthread_mutex_lock(&worker->queue_mutex);
        worker->total_completed++;
        pthread_mutex_unlock(&worker->queue_mutex);

        /* Notify completion */
        if (worker->on_complete) {
            worker->on_complete(&response, worker->callback_user_data);
        }

        free(request);
    }

    return NULL;
}

/* ============================================================================
 * GIT STATUS PROVIDER
 * ============================================================================
 */

/**
 * @brief Run a git command and capture output
 *
 * Executes a shell command using popen and captures the first line of output.
 * The command should include proper stderr redirection as needed.
 *
 * @param cmd Command string to execute
 * @param output Buffer for captured output (may be NULL)
 * @param output_size Size of output buffer
 * @return true if command succeeded (exit status 0), false otherwise
 */
static bool run_git_command(const char *cmd, char *output, size_t output_size) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return false;
    }

    if (output && output_size > 0) {
        if (fgets(output, (int)output_size, fp)) {
            /* Remove trailing newline */
            size_t len = strlen(output);
            if (len > 0 && output[len - 1] == '\n') {
                output[len - 1] = '\0';
            }
        } else {
            output[0] = '\0';
        }
    }

    /* Drain any remaining output to prevent pipe blocking */
    char drain[256];
    while (fgets(drain, sizeof(drain), fp) != NULL) {
        /* Discard */
    }

    int status = pclose(fp);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/**
 * @brief Get git repository status
 *
 * Gathers comprehensive git status information for a repository.
 * This function runs in the worker thread and may block on git commands.
 *
 * @param cwd Working directory of the repository
 * @param timeout_ms Timeout in milliseconds (currently unused)
 * @param status Output structure for git status data
 * @return LLE_SUCCESS on success
 * @return LLE_ERROR_INVALID_PARAMETER if cwd or status is NULL
 * @return LLE_ERROR_SYSTEM_CALL if directory operations fail
 */
static lle_result_t lle_async_get_git_status(const char *cwd,
                                             uint32_t timeout_ms,
                                             lle_git_status_data_t *status) {
    (void)
        timeout_ms; /* Timeout not implemented - git commands have their own */

    if (!cwd || !status) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    memset(status, 0, sizeof(*status));

    /* Save current directory */
    char old_cwd[PATH_MAX];
    if (!getcwd(old_cwd, sizeof(old_cwd))) {
        return LLE_ERROR_SYSTEM_CALL;
    }

    /* Change to target directory */
    if (chdir(cwd) != 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Check if in git repo */
    if (!run_git_command("git rev-parse --git-dir 2>/dev/null", NULL, 0)) {
        chdir(old_cwd);
        status->is_git_repo = false;
        return LLE_SUCCESS;
    }
    status->is_git_repo = true;

    /* Get branch name */
    if (run_git_command("git branch --show-current 2>/dev/null", status->branch,
                        sizeof(status->branch))) {
        /* branch is set */
    } else {
        /* Might be detached HEAD */
        status->is_detached = true;
        run_git_command("git rev-parse --short HEAD 2>/dev/null",
                        status->commit, sizeof(status->commit));
    }

    /* Check for detached HEAD explicitly */
    char head_ref[256] = {0};
    if (run_git_command("git symbolic-ref HEAD 2>/dev/null", head_ref,
                        sizeof(head_ref))) {
        status->is_detached = false;
    } else {
        status->is_detached = true;
    }

    /* Get short commit hash */
    if (status->commit[0] == '\0') {
        run_git_command("git rev-parse --short HEAD 2>/dev/null",
                        status->commit, sizeof(status->commit));
    }

    /* Get status counts using git status --porcelain (same as sync version) */
    status->staged_count = 0;
    status->unstaged_count = 0;
    status->untracked_count = 0;

    FILE *fp = popen("git status --porcelain 2>/dev/null", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (line[0] == '?') {
                status->untracked_count++;
            } else {
                if (line[0] != ' ' && line[0] != '?') {
                    status->staged_count++;
                }
                if (line[1] != ' ' && line[1] != '?') {
                    status->unstaged_count++;
                }
            }
        }
        pclose(fp);
    }

    /* Check ahead/behind counts */
    char ahead_behind[64] = {0};
    if (run_git_command("git rev-list --left-right --count HEAD...@{upstream} "
                        "2>/dev/null",
                        ahead_behind, sizeof(ahead_behind))) {
        sscanf(ahead_behind, "%d %d", &status->ahead, &status->behind);
    }

    /* Check for merge in progress */
    char merge_head[8] = {0};
    if (run_git_command("test -f .git/MERGE_HEAD && echo 1", merge_head,
                        sizeof(merge_head))) {
        status->is_merging = (merge_head[0] == '1');
    }

    /* Check for rebase in progress */
    char rebase_dir[8] = {0};
    if (run_git_command("test -d .git/rebase-merge -o -d .git/rebase-apply && "
                        "echo 1",
                        rebase_dir, sizeof(rebase_dir))) {
        status->is_rebasing = (rebase_dir[0] == '1');
    }

    /* Restore original directory */
    chdir(old_cwd);

    return LLE_SUCCESS;
}
