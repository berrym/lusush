/**
 * LLE Input Stream Management
 *
 * Provides raw terminal input stream buffering and flow control for the
 * input parsing system.
 *
 * Specification: docs/lle_specification/06_input_parsing_complete.md
 * Phase: 1 - Input Stream Management
 */

#include "lle/error_handling.h"
#include "lle/input_parsing.h"
#include "lle/memory_management.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================
 */

/**
 * Set terminal file descriptor to non-blocking mode
 */
static lle_result_t set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return LLE_ERROR_SYSTEM_CALL;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return LLE_ERROR_SYSTEM_CALL;
    }

    return LLE_SUCCESS;
}

/**
 * Set terminal file descriptor to blocking mode
 */
static lle_result_t set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return LLE_ERROR_SYSTEM_CALL;
    }

    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        return LLE_ERROR_SYSTEM_CALL;
    }

    return LLE_SUCCESS;
}

/**
 * Compact buffer by moving unconsumed data to beginning
 */
static void compact_buffer(lle_input_stream_t *stream) {
    if (!stream || stream->buffer_pos == 0) {
        return;
    }

    size_t remaining = stream->buffer_used - stream->buffer_pos;
    if (remaining > 0) {
        memmove(stream->buffer, stream->buffer + stream->buffer_pos, remaining);
    }

    stream->buffer_used = remaining;
    stream->buffer_pos = 0;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================
 */

/**
 * Initialize input stream
 */
lle_result_t lle_input_stream_init(lle_input_stream_t **stream,
                                   lle_terminal_system_t *terminal,
                                   lle_memory_pool_t *memory_pool) {
    if (!stream || !terminal || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_result_t result = LLE_SUCCESS;
    lle_input_stream_t *new_stream = NULL;

    /* Allocate stream structure */
    new_stream = lle_pool_alloc(sizeof(lle_input_stream_t));
    if (!new_stream) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(new_stream, 0, sizeof(lle_input_stream_t));

    /* Allocate input buffer */
    new_stream->buffer = lle_pool_alloc(LLE_INPUT_BUFFER_SIZE);
    if (!new_stream->buffer) {
        lle_pool_free(new_stream);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize stream fields */
    new_stream->buffer_size = LLE_INPUT_BUFFER_SIZE;
    new_stream->buffer_used = 0;
    new_stream->buffer_pos = 0;
    new_stream->memory_pool = memory_pool;

    /* Get terminal file descriptor - assuming STDIN for now */
    new_stream->terminal_fd = STDIN_FILENO;

    /* Set non-blocking mode by default for responsive input */
    new_stream->blocking_mode = false;
    result = set_nonblocking(new_stream->terminal_fd);
    if (result != LLE_SUCCESS) {
        lle_pool_free(new_stream->buffer);
        lle_pool_free(new_stream);
        return result;
    }

    /* Initialize statistics */
    new_stream->bytes_read = 0;
    new_stream->read_operations = 0;
    new_stream->buffer_overflows = 0;

    /* Flow control disabled by default */
    new_stream->flow_control_enabled = false;

    /* Note: terminal_caps would be set from terminal system in full integration
     */
    new_stream->terminal_caps = NULL;

    *stream = new_stream;
    return LLE_SUCCESS;
}

/**
 * Destroy input stream
 */
lle_result_t lle_input_stream_destroy(lle_input_stream_t *stream) {
    if (!stream) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Restore blocking mode on terminal before cleanup */
    if (!stream->blocking_mode) {
        set_blocking(stream->terminal_fd);
    }

    /* Free buffer */
    if (stream->buffer) {
        lle_pool_free(stream->buffer);
    }

    /* Free stream structure */
    lle_pool_free(stream);

    return LLE_SUCCESS;
}

/**
 * Read data from terminal into internal buffer
 */
lle_result_t lle_input_stream_read(lle_input_stream_t *stream, char *buffer,
                                   size_t buffer_size, size_t *bytes_read) {
    if (!stream || !buffer || !bytes_read) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    *bytes_read = 0;

    /* Compact buffer if needed to make room */
    if (stream->buffer_used >= stream->buffer_size) {
        compact_buffer(stream);

        /* Check if buffer is still full after compacting */
        if (stream->buffer_used >= stream->buffer_size) {
            stream->buffer_overflows++;
            return LLE_ERROR_BUFFER_OVERFLOW;
        }
    }

    /* Read from terminal */
    size_t space_available = stream->buffer_size - stream->buffer_used;
    ssize_t n = read(stream->terminal_fd, stream->buffer + stream->buffer_used,
                     space_available);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* No data available in non-blocking mode - not an error */
            return LLE_SUCCESS;
        }
        return LLE_ERROR_IO_ERROR;
    }

    if (n == 0) {
        /* EOF - terminal closed */
        return LLE_ERROR_IO_ERROR;
    }

    /* Update buffer state */
    stream->buffer_used += (size_t)n;
    stream->bytes_read += (size_t)n;
    stream->read_operations++;

    /* Copy to output buffer if requested */
    size_t copy_size = (size_t)n < buffer_size ? (size_t)n : buffer_size;
    memcpy(buffer, stream->buffer + stream->buffer_used - (size_t)n, copy_size);
    *bytes_read = copy_size;

    return LLE_SUCCESS;
}

/**
 * Buffer incoming data (for testing or piped input)
 */
lle_result_t lle_input_stream_buffer_data(lle_input_stream_t *stream,
                                          const char *data, size_t data_len) {
    if (!stream || !data) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (data_len == 0) {
        return LLE_SUCCESS;
    }

    /* Compact buffer if needed */
    if (stream->buffer_used + data_len > stream->buffer_size) {
        compact_buffer(stream);

        /* Check if there's still not enough space */
        if (stream->buffer_used + data_len > stream->buffer_size) {
            stream->buffer_overflows++;
            return LLE_ERROR_BUFFER_OVERFLOW;
        }
    }

    /* Copy data to buffer */
    memcpy(stream->buffer + stream->buffer_used, data, data_len);
    stream->buffer_used += data_len;
    stream->bytes_read += data_len;

    return LLE_SUCCESS;
}

/**
 * Get buffered data without consuming
 */
lle_result_t lle_input_stream_get_buffered(lle_input_stream_t *stream,
                                           const char **data,
                                           size_t *data_len) {
    if (!stream || !data || !data_len) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t available = stream->buffer_used - stream->buffer_pos;

    if (available == 0) {
        *data = NULL;
        *data_len = 0;
        return LLE_SUCCESS;
    }

    *data = stream->buffer + stream->buffer_pos;
    *data_len = available;

    return LLE_SUCCESS;
}

/**
 * Consume processed data from buffer
 */
lle_result_t lle_input_stream_consume(lle_input_stream_t *stream,
                                      size_t bytes) {
    if (!stream) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t available = stream->buffer_used - stream->buffer_pos;

    if (bytes > available) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    stream->buffer_pos += bytes;

    /* Compact buffer if we've consumed a significant portion */
    if (stream->buffer_pos > stream->buffer_size / 2) {
        compact_buffer(stream);
    }

    return LLE_SUCCESS;
}

/**
 * Peek at single byte at offset without consuming
 */
lle_result_t lle_input_stream_peek(lle_input_stream_t *stream, size_t offset,
                                   char *byte) {
    if (!stream || !byte) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t available = stream->buffer_used - stream->buffer_pos;

    if (offset >= available) {
        return LLE_ERROR_BUFFER_UNDERFLOW;
    }

    *byte = stream->buffer[stream->buffer_pos + offset];

    return LLE_SUCCESS;
}

/**
 * Set blocking mode
 */
lle_result_t lle_input_stream_set_blocking(lle_input_stream_t *stream,
                                           bool blocking) {
    if (!stream) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (stream->blocking_mode == blocking) {
        return LLE_SUCCESS; /* Already in desired mode */
    }

    lle_result_t result;
    if (blocking) {
        result = set_blocking(stream->terminal_fd);
    } else {
        result = set_nonblocking(stream->terminal_fd);
    }

    if (result == LLE_SUCCESS) {
        stream->blocking_mode = blocking;
    }

    return result;
}

/**
 * Enable/disable flow control
 */
lle_result_t lle_input_stream_set_flow_control(lle_input_stream_t *stream,
                                               bool enabled) {
    if (!stream) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    stream->flow_control_enabled = enabled;
    return LLE_SUCCESS;
}

/**
 * Get stream statistics
 */
lle_result_t lle_input_stream_get_statistics(lle_input_stream_t *stream,
                                             uint64_t *bytes_read,
                                             uint64_t *read_operations,
                                             uint64_t *buffer_overflows) {
    if (!stream) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (bytes_read) {
        *bytes_read = stream->bytes_read;
    }

    if (read_operations) {
        *read_operations = stream->read_operations;
    }

    if (buffer_overflows) {
        *buffer_overflows = stream->buffer_overflows;
    }

    return LLE_SUCCESS;
}

/**
 * Reset stream state
 */
lle_result_t lle_input_stream_reset(lle_input_stream_t *stream) {
    if (!stream) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Clear buffer */
    stream->buffer_used = 0;
    stream->buffer_pos = 0;

    /* Reset statistics */
    stream->bytes_read = 0;
    stream->read_operations = 0;
    stream->buffer_overflows = 0;

    return LLE_SUCCESS;
}

/**
 * Get available bytes in buffer
 */
lle_result_t lle_input_stream_get_available(lle_input_stream_t *stream,
                                            size_t *available) {
    if (!stream || !available) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    *available = stream->buffer_used - stream->buffer_pos;

    return LLE_SUCCESS;
}
