CC := gcc

CFLAGS := -Wall -Wextra -Iinclude -g

LDFLAGS := -lncurses -lmenu

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

TARGET := $(BIN_DIR)/FileTransfer

all: $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ_FILES)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)