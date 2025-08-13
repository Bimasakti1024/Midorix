# Compiler and flags
CC = gcc
CFLAGS = -O2 -g -Wall

# Paths
SRC_DIR = src
BUILD_DIR = build
EXT_DIR = $(SRC_DIR)/ext
UTIL_DIR = $(SRC_DIR)/util
CHANDL_DIR = $(SRC_DIR)/chandl

# Source files
SRC_MAIN = $(SRC_DIR)/main.c
SRC_UTIL = $(UTIL_DIR)/util.c
SRC_CHANDL = $(CHANDL_DIR)/chandl.c
SRC_CMDH = $(CHANDL_DIR)/cmd_handle.c
SRC_CJSON = $(EXT_DIR)/cJSON/cJSON.c
SRC_LINENOISE = $(EXT_DIR)/linenoise/linenoise.c

# Header files (only needed if dependencies tracked)
HDR_UTIL = $(UTIL_DIR)/util.h
HDR_CHANDL = $(CHANDL_DIR)/chandl.h
HDR_CMDH = $(CHANDL_DIR)/cmd_handle.h

# Object files
OBJ_MAIN = $(BUILD_DIR)/main.o
OBJ_UTIL = $(BUILD_DIR)/util.o
OBJ_CHANDL = $(BUILD_DIR)/chandl.o
OBJ_CMDH = $(BUILD_DIR)/cmd_handle.o
OBJ_CJSON = $(BUILD_DIR)/cJSON.o
OBJ_LINENOISE = $(BUILD_DIR)/linenoise.o
OBJECTS = $(OBJ_MAIN) $(OBJ_UTIL) $(OBJ_CHANDL) $(OBJ_CMDH) $(OBJ_CJSON) $(OBJ_LINENOISE)

# Output binary
BIN = $(BUILD_DIR)/midorix

# Default target
all: $(BIN)

# Link step
$(BIN): $(OBJECTS)
	$(CC) -o $@ $^ -lreadline -llua -lm -ldl

# Compile rules
$(OBJ_MAIN): $(SRC_MAIN)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_UTIL): $(SRC_UTIL) $(HDR_UTIL)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_CHANDL): $(SRC_CHANDL)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_CMDH): $(SRC_CMDH)
	$(CC) -c $< -o $@ $(CFLAGS) -llua -lm -ldl

$(OBJ_CJSON): $(SRC_CJSON)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_LINENOISE): $(SRC_LINENOISE)
	$(CC) -c $< -o $@ $(CFLAGS)

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
	find . -type d -wholename './src/ext' -prune -o -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +

.PHONY: all run clean memcheck clang-format

