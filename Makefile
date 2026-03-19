UNAME := $(shell uname)

CC     = cc
CFLAGS = -Wall -Wextra -std=c23

BUILD_DIR = build
SRC_DIR   = src
TARGET    = $(BUILD_DIR)/game

ifeq ($(UNAME), Darwin)
    PLATFORM_SRC    = $(SRC_DIR)/macos.m
    PLATFORM_OBJ    = $(BUILD_DIR)/macos.o
    PLATFORM_CFLAGS = -Wall -Wextra
    LDFLAGS         = -framework Cocoa
else
    PLATFORM_SRC    = $(SRC_DIR)/linux.c
    PLATFORM_OBJ    = $(BUILD_DIR)/linux.o
    PLATFORM_CFLAGS = $(CFLAGS)
    LDFLAGS         = -lX11
endif

OBJS = $(BUILD_DIR)/main.o $(PLATFORM_OBJ)

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(PLATFORM_OBJ): $(PLATFORM_SRC)
	$(CC) $(PLATFORM_CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
