# Makefile for LLE Foundation Layer

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -O2 -g
CFLAGS += -D_POSIX_C_SOURCE=200809L

# Source files
TERMINAL_SOURCES = terminal/terminal.c terminal/capability.c
TERMINAL_OBJECTS = $(TERMINAL_SOURCES:.c=.o)

DISPLAY_SOURCES = display/display.c
DISPLAY_OBJECTS = $(DISPLAY_SOURCES:.c=.o)

ALL_OBJECTS = $(TERMINAL_OBJECTS) $(DISPLAY_OBJECTS)

# Test files
TEST_SOURCES = test/terminal_test.c test/terminal_unit_test.c test/display_test.c
TEST_BINARIES = $(TEST_SOURCES:.c=)

# Targets
.PHONY: all clean test

all: $(TEST_BINARIES)

# Terminal tests
test/terminal_test: test/terminal_test.c $(TERMINAL_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

test/terminal_unit_test: test/terminal_unit_test.c $(TERMINAL_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Display tests
test/display_test: test/display_test.c $(ALL_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Run tests
test: all
	@echo "Running terminal abstraction unit tests (no TTY required)..."
	@./test/terminal_unit_test
	@echo ""
	@echo "To run full integration tests, execute: ./test/terminal_test"
	@echo "(Requires interactive terminal)"

# Clean
clean:
	rm -f $(ALL_OBJECTS) $(TEST_BINARIES)
	rm -f terminal/*.o display/*.o test/*.o

# Dependencies
terminal/terminal.o: terminal/terminal.h
terminal/capability.o: terminal/terminal.h
test/terminal_test.o: terminal/terminal.h
