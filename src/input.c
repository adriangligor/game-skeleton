#include "input.h"

#define QUEUE_CAPACITY 64

static Event g_queue[QUEUE_CAPACITY];
static int g_head;   // next write index
static int g_tail;   // next read index

void input_push_event(Event e) {
    int next = (g_head + 1) % QUEUE_CAPACITY;
    if (next == g_tail) {
        return;                 // full — drop oldest would be complex; just drop new
    }
    g_queue[g_head] = e;
    g_head = next;
}

bool input_next_event(Event *out) {
    if (g_tail == g_head) {
        return false;
    }
    *out = g_queue[g_tail];
    g_tail = (g_tail + 1) % QUEUE_CAPACITY;

    return true;
}
