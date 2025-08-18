# Compiler and flags
CC = gcc
CFLAGS = -fsanitize=address -fno-omit-frame-pointer -O2 -g -Wall

# Paths
SRC_DIR = src
BUILD_DIR = build
EXT_DIR = $(SRC_DIR)/ext
UTIL_DIR = $(SRC_DIR)/util
CHANDL_DIR = $(SRC_DIR)/chandl
PU_DIR = $(SRC_DIR)/projectutil

# Source files
SRC_MAIN = $(SRC_DIR)/main.c
SRC_UTIL = $(UTIL_DIR)/util.c
SRC_CHANDL = $(CHANDL_DIR)/chandl.c
SRC_CMDH = $(CHANDL_DIR)/cmd_handle.c
SRC_PU = $(PU_DIR)/projectutil.c

# Object files
OBJ_MAIN = $(BUILD_DIR)/main.o
OBJ_UTIL = $(BUILD_DIR)/util.o
OBJ_CHANDL = $(BUILD_DIR)/chandl.o
OBJ_CMDH = $(BUILD_DIR)/cmd_handle.o
OBJ_PU = $(BUILD_DIR)/projectutil.o
OBJECTS = $(OBJ_MAIN) $(OBJ_UTIL) $(OBJ_CHANDL) $(OBJ_CMDH) $(OBJ_PU)

# Output binary
BIN = $(BUILD_DIR)/midorix

# Default target
all: $(BIN)

# Link step
$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS) -llinenoise -llua -lm -ldl -lcjson

# Compile rules
$(OBJ_MAIN): $(SRC_MAIN)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_UTIL): $(SRC_UTIL)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_CHANDL): $(SRC_CHANDL)
	$(CC) -c $< -o $@ $(CFLAGS) -lcjson

$(OBJ_CMDH): $(SRC_CMDH)
	$(CC) -c $< -o $@ $(CFLAGS) -llua -lm -ldl -lcjson

$(OBJ_PU): $(SRC_PU) $(SRC_UTIL)
	$(CC) -c $< -o $@ $(CFLAGS) -llua -lm -ldl -lcjson

# Run target
run: all
	./$(BIN)

# Clean
clean:
	rm -rf $(BUILD_DIR)/*.o $(BIN)

# Memory Check
memcheck: $(BIN)
	valgrind --leak-check=full --show-leak-kinds=all -s $<

# clang-format
clang-format:
	find . -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +

.PHONY: all run clean memcheck clang-format

