CC = gcc
CFLAGS = -Wall -Wextra -std=c11
SRC = src/lsv1.0.0.c
TARGET = ls
BIN_DIR = bin

all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

run: all
	./$(BIN_DIR)/$(TARGET)
