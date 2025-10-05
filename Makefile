# Makefile for lsv1.0.0

CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = ls
SRC = src/lsv1.0.0.c
BIN_DIR = bin

# Default target
all: $(BIN_DIR)/$(TARGET)

# Create bin directory if not exists and compile
$(BIN_DIR)/$(TARGET): $(SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Create bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean build files
clean:
	rm -rf $(BIN_DIR)

# Run the program
run: all
	./$(BIN_DIR)/$(TARGET)
