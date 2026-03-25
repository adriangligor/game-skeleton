ifdef OS
    # Windows – MSVC
    CC              = cl
    CFLAGS          = /W4 /nologo /std:c17
    BUILD_DIR       = build
    SRC_DIR         = src
    TARGET          = $(BUILD_DIR)/game.exe
    PLATFORM_SRC    = $(SRC_DIR)/windows.c
    PLATFORM_OBJ    = $(BUILD_DIR)/windows.obj
    PLATFORM_CFLAGS = $(CFLAGS)
    LDFLAGS         = user32.lib d3d11.lib dxgi.lib d3dcompiler.lib /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
    MAIN_OBJ        = $(BUILD_DIR)/main.obj
    GAME_OBJ        = $(BUILD_DIR)/game.obj
    SURFACE_OBJ     = $(BUILD_DIR)/surface.obj
    CC_COMPILE      = /c
    CC_OBJ_OUT      = /Fo
    CC_EXE_OUT      = /Fe
    MKDIR           = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    CLEAN           = if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
else
    UNAME := $(shell uname)
    CC     = cc
    CFLAGS = -Wall -Wextra -std=c23
    BUILD_DIR   = build
    SRC_DIR     = src
    TARGET      = $(BUILD_DIR)/game
    MAIN_OBJ    = $(BUILD_DIR)/main.o
    GAME_OBJ    = $(BUILD_DIR)/game.o
    SURFACE_OBJ = $(BUILD_DIR)/surface.o
    CC_COMPILE  = -c
    CC_OBJ_OUT  = -o
    CC_EXE_OUT  = -o
    MKDIR = mkdir -p $(BUILD_DIR)
    CLEAN = rm -rf $(BUILD_DIR)

    ifeq ($(UNAME), Darwin)
        PLATFORM_SRC    = $(SRC_DIR)/macos.m
        PLATFORM_OBJ    = $(BUILD_DIR)/macos.o
        PLATFORM_CFLAGS = -Wall -Wextra
        LDFLAGS         = -framework Cocoa -framework Metal -framework QuartzCore
    else
        PLATFORM_SRC    = $(SRC_DIR)/linux.c
        PLATFORM_OBJ    = $(BUILD_DIR)/linux.o
        PLATFORM_CFLAGS = $(CFLAGS)
        LDFLAGS         = -lX11 -lGL
    endif
endif

OBJS = $(MAIN_OBJ) $(GAME_OBJ) $(SURFACE_OBJ) $(PLATFORM_OBJ)

FMT_SRCS = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(SRC_DIR)/*.m)

.PHONY: all clean fmt

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	$(MKDIR)

$(MAIN_OBJ): $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(CC_COMPILE) $(CC_OBJ_OUT)$@ $<

$(GAME_OBJ): $(SRC_DIR)/game.c
	$(CC) $(CFLAGS) $(CC_COMPILE) $(CC_OBJ_OUT)$@ $<

$(SURFACE_OBJ): $(SRC_DIR)/surface.c
	$(CC) $(CFLAGS) $(CC_COMPILE) $(CC_OBJ_OUT)$@ $<

$(PLATFORM_OBJ): $(PLATFORM_SRC)
	$(CC) $(PLATFORM_CFLAGS) $(CC_COMPILE) $(CC_OBJ_OUT)$@ $<

$(TARGET): $(OBJS)
	$(CC) $(CC_EXE_OUT)$@ $^ $(LDFLAGS)

clean:
	$(CLEAN)

fmt:
	uncrustify -c .uncrustify.cfg --no-backup $(FMT_SRCS)
