CC = gcc
CFLAGS = -O2 -g -Wall

# Source files
MAIN_SRC = src/main.c
UTIL_SRC = src/util/util.c
INIH_SRC = src/inih/ini.c
CHANDL_SRC = src/chandl/chandl.c
CMDH_SRC = src/chandl/cmd_handle.c

# Header
UTIL_H = src/util/util.h
CHANDL_H = src/chandl/chandl.h
CMDH_H = src/chandl/cmd_handle.h

# Object files
MAIN_BIN = build/midorix
MAIN_OBJ = build/main.o
UTIL_OBJ = build/util.o
INIH_OBJ = build/ini.o
CHANDL_OBJ = build/chandl.o
CMDH_OBJ = build/cmd_handle.o
ALL_OBJ = $(MAIN_BIN) $(MAIN_OBJ) $(UTIL_OBJ) $(INIH_OBJ) $(CHANDL_OBJ) $(CMDH_OBJ)

# Default target
all: clean build run

# Clean target
clean:
	rm -rf $(ALL_OBJ)

# Build target
build: $(ALL_OBJ)

# Build target for util.o
$(UTIL_OBJ): $(UTIL_SRC) $(UTIL_H)
	$(CC) -c -o $@ $(UTIL_SRC) $(CFLAGS)

# Build target for ini.o
$(INIH_OBJ): $(INIH_SRC)
	$(CC) -c -o $@ $(INIH_SRC) $(CFLAGS)

# Build target for chandl.o
$(CHANDL_OBJ): $(CHANDL_SRC)
	$(CC) -c -o $@ $(CHANDL_SRC) $(CFLAGS)

# Build target for the command handler
$(CMDH_OBJ): $(CMDH_SRC)
	$(CC) -c -o $@ $(CMDH_SRC) $(CFLAGS)

# Build target for the main executable
$(MAIN_OBJ): $(MAIN_SRC)
	$(CC) -c -o $@ $(MAIN_SRC)

# Link
$(MAIN_BIN): $(ALL_OBJ)
	$(CC) -o $@ build/*

run:
	./$(MAIN_BIN)

