# Makefile for LLE PTY Tests

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE
INCLUDES = -I../../../include

# Source files
HARNESS_SRC = pty_test_harness.c
HARNESS_OBJ = pty_test_harness.o

# Test executables
TESTS = test_pty_basic_display test_pty_cursor_diagnostic

# Library dependencies (for linking against full system on some platforms)
LDFLAGS = -lutil

.PHONY: all clean run

all: $(TESTS)

# Build harness
$(HARNESS_OBJ): $(HARNESS_SRC) pty_test_harness.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(HARNESS_SRC) -o $(HARNESS_OBJ)

# Build basic display test
test_pty_basic_display: test_pty_basic_display.c $(HARNESS_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) test_pty_basic_display.c $(HARNESS_OBJ) -o test_pty_basic_display $(LDFLAGS)

# Build cursor diagnostic test
test_pty_cursor_diagnostic: test_pty_cursor_diagnostic.c $(HARNESS_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) test_pty_cursor_diagnostic.c $(HARNESS_OBJ) -o test_pty_cursor_diagnostic $(LDFLAGS)

# Run tests
run: all
	@echo "=========================================="
	@echo "Running PTY Tests"
	@echo "=========================================="
	@echo ""
	@./test_pty_basic_display

# Clean build artifacts
clean:
	rm -f $(HARNESS_OBJ) $(TESTS)
	@echo "Cleaned PTY test build artifacts"
