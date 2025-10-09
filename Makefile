# Compiler and flags
CC = gcc
CFLAGS = -fno-omit-frame-pointer -O2 -g -Wall

# Paths
SRC_DIR = src
BUILD_DIR = build
EXT_DIR = $(SRC_DIR)/ext
UTIL_DIR = $(SRC_DIR)/util
CHANDL_DIR = $(SRC_DIR)/chandl
PU_DIR = $(SRC_DIR)/projectutil
CORE_DIR = $(SRC_DIR)/core
ENGINE_DIR = $(SRC_DIR)/engine
RSLIB_DIR = target/release

# Source files
SRC_MAIN = $(SRC_DIR)/main.c
SRC_UTIL = $(UTIL_DIR)/util.c
SRC_CHANDL = $(CHANDL_DIR)/chandl.c
SRC_CMDH = $(CHANDL_DIR)/cmd_handle.c
SRC_PU = $(PU_DIR)/projectutil.c
SRC_CORE = $(CORE_DIR)/core.c
SRC_ENGINE = $(ENGINE_DIR)/engine.c

# Object files
OBJ_MAIN = $(BUILD_DIR)/main.o
OBJ_UTIL = $(BUILD_DIR)/util.o
OBJ_CHANDL = $(BUILD_DIR)/chandl.o
OBJ_CMDH = $(BUILD_DIR)/cmd_handle.o
OBJ_PU = $(BUILD_DIR)/projectutil.o
OBJ_CORE = $(BUILD_DIR)/core.o
OBJ_ENGINE = $(BUILD_DIR)/engine.o
OBJECTS = $(OBJ_MAIN) $(OBJ_UTIL) $(OBJ_CHANDL) $(OBJ_CMDH) $(OBJ_PU) $(OBJ_CORE) $(OBJ_ENGINE)

# Output binary
BIN = $(BUILD_DIR)/midorix

# Release Build
release:
	cargo build --release
	$(MAKE) $(BIN)

# Make build smaller
tiny-build: $(BIN)
	strip $(BIN)
	upx --best --lzma $(BIN)

# Debug Build
debug:
	cargo build
	$(MAKE) $(BIN) RSLIB_DIR=target/debug CFLAGS="-fsanitize=address -fno-omit-frame-pointer -g -O0"

# Clean Build
clean:
	cargo clean
	rm build/*

# Link step
$(BIN): $(OBJECTS)
	@echo ">> Linking $@"
	$(CC) -o $@ $^ \
		$(CFLAGS) \
		-Wl,-Bstatic -L $(RSLIB_DIR) -lflag_parsr -lrscmd_handle \
		-Wl,-Bdynamic -llinenoise -llua -lm -ldl -lcjson

# Compile rules
$(OBJ_MAIN): $(SRC_MAIN)
	@echo ">> Compiling $<"
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_UTIL): $(SRC_UTIL)
	@echo ">> Compiling $<"
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_CHANDL): $(SRC_CHANDL)
	@echo ">> Compiling $<"
	$(CC) -c $< -o $@ $(CFLAGS) -lcjson

$(OBJ_CMDH): $(SRC_CMDH)
	@echo ">> Compiling $<"
	$(CC) -c $< -o $@ $(CFLAGS) -llua -lm -ldl -lcjson

$(OBJ_PU): $(SRC_PU) $(SRC_UTIL)
	@echo ">> Compiling $<"
	$(CC) -c $< -o $@ $(CFLAGS) -llua -lm -ldl -lcjson

$(OBJ_CORE): $(SRC_CORE) $(OBJ_UTIL)
	@echo ">> Compiling $<"
	$(CC) -c $< -o $@ $(CFLAGS) -llua -lm -ldl -lcjson

$(OBJ_ENGINE): $(SRC_ENGINE) $(OBJ_UTIL) $(OBJ_CORE)
	@echo ">> Compiling $<"
	$(CC) -c $< -o $@ $(CFLAGS) -llinenoise -llua -lm -ldl -lcjson

# Run target
run: $(BIN)
	@echo ">> Running Midorix..."
	./$(BIN)

# Install Midorix
install: $(BIN)
	@echo "Installing Midorix..."
	
	@echo "Setting up Configuration."
	@mkdir -p ~/.config/midorix
	@cp -r assets/default_config/* ~/.config/midorix
	
	@echo "Installing Midorix..."
	@sudo cp $(BIN) /usr/local/bin
	
	@echo "Done installing Midorix!"

