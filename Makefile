CC      = cc
CFLAGS  = -Wall -Wextra -std=c23

SRC_DIR   = src
BUILD_DIR = build
TARGET    = $(BUILD_DIR)/game

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)
