# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Werror
DEBUG_FLAGS = -g
RELEASE_FLAGS = -O2 -s

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Targets
DEBUG_TARGET = $(BIN_DIR)/Debug/xlang
RELEASE_TARGET = $(BIN_DIR)/Release/xlang

.PHONY: all debug release clean

all: debug release

debug: $(DEBUG_TARGET)

release: $(RELEASE_TARGET)

$(DEBUG_TARGET): $(OBJ_FILES)
    @mkdir -p $(BIN_DIR)/Debug
    $(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^ $(LDFLAGS)

$(RELEASE_TARGET): $(OBJ_FILES)
    @mkdir -p $(BIN_DIR)/Release
    $(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    @rm -rf $(OBJ_DIR) $(BIN_DIR)