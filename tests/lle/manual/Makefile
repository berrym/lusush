# Makefile for manual F-key test program

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I../../../include
LDFLAGS = -L../../../build
LIBS = -lle -lutil -lpthread

TARGET = test_fkey_manual
SOURCES = test_fkey_manual.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS) $(LIBS)
	@echo ""
	@echo "Manual F-key test compiled successfully!"
	@echo "Run with: ./$(TARGET)"
	@echo ""

clean:
	rm -f $(TARGET)

.PHONY: run
run: $(TARGET)
	./$(TARGET)
