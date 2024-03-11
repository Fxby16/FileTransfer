CC := gcc

CFLAGS := -Wall -Wextra -Iinclude -g

LDFLAGS := -lncurses -lmenu

SRC_DIR := src

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, %.o, $(SRC_FILES))

TARGET := FileTransfer

all: $(TARGET)

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJ_FILES) $(TARGET)