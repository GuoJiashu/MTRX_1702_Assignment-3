# Makefile for pcb_check
# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -Iinclude

# Source files and object files
SRC_DIR = src
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_DIR = obj
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Targets
TARGET = pcb_check

# Phony targets
.PHONY: all clean

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up object files and the executable
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Define dependencies for object files
$(OBJ_DIR)/bitmap.o: $(SRC_DIR)/bitmap.c include/bitmap.h
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c
