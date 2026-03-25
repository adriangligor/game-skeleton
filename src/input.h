#pragma once
#include <stdbool.h>

typedef enum {
    KEY_UNKNOWN = 0,
    KEY_ESCAPE, KEY_ENTER, KEY_SPACE, KEY_TAB,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
    KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
    KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_COUNT
} KeyCode;

typedef enum {
    EVENT_NONE = 0,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_SYSTEM_QUIT,
} EventType;

typedef struct {
    EventType type;
    KeyCode key;   // valid for EVENT_KEY_DOWN / EVENT_KEY_UP
} Event;

// Called by platform code during platform_pump_events()
void input_push_event(Event e);

// Called by game code to consume one event; returns false when queue is empty
bool input_next_event(Event *out);
