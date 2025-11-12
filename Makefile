# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Files
TARGET = $(BIN_DIR)/app
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Default target
all: $(TARGET)

# Link object files into final executable
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Compile each .c into a .o file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run the program
run: all
	./$(TARGET)
