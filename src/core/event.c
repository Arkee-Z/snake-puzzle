#include "snake_puzzle/core/event.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void event_queue_init(EventQueue *queue)
{
    memset(queue, 0, sizeof(*queue));
}

void event_queue_clear(EventQueue *queue)
{
    queue->head = 0;
    queue->count = 0;
}

void event_push(EventQueue *queue, EventType type, int x, int y, int score,
                const char *format, ...)
{
    Event *event;

    if (queue == NULL) {
        return;
    }
    if (queue->count >= SP_MAX_EVENTS) {
        queue->head = (queue->head + 1) % SP_MAX_EVENTS;
        queue->count -= 1;
    }
    event = &queue->events[(queue->head + queue->count) % SP_MAX_EVENTS];
    memset(event, 0, sizeof(*event));
    event->type = type;
    event->x = x;
    event->y = y;
    event->score = score;
    if (format != NULL && format[0] != '\0') {
        va_list args;
        va_start(args, format);
        vsnprintf(event->message, sizeof(event->message), format, args);
        va_end(args);
    }
    queue->count += 1;
}

bool event_poll(EventQueue *queue, Event *out)
{
    if (queue == NULL || out == NULL || queue->count == 0) {
        return false;
    }
    *out = queue->events[queue->head];
    queue->head = (queue->head + 1) % SP_MAX_EVENTS;
    queue->count -= 1;
    return true;
}

const Event *event_last(const EventQueue *queue)
{
    if (queue == NULL || queue->count == 0) {
        return NULL;
    }
    return &queue->events[(queue->head + queue->count - 1) % SP_MAX_EVENTS];
}

const char *event_type_name(EventType type)
{
    switch (type) {
    case EVENT_MOVE:
        return "move";
    case EVENT_EAT_FOOD:
        return "eat";
    case EVENT_PICKUP_ITEM:
        return "item";
    case EVENT_PUSH_BLOCK:
        return "push";
    case EVENT_PLATE_PRESSED:
        return "plate";
    case EVENT_GATE_OPENED:
        return "gate";
    case EVENT_TELEPORT:
        return "teleport";
    case EVENT_BREAK_FLOOR:
        return "collapse";
    case EVENT_SEAL_COLLECTED:
        return "seal";
    case EVENT_SPECIAL_UNLOCKED:
        return "unlocked";
    case EVENT_ENTER_SPECIAL:
        return "chamber";
    case EVENT_SPECIAL_FINISHED:
        return "chamber-clear";
    case EVENT_ORB_COLLECTED:
        return "orb";
    case EVENT_MINI_UNLOCKED:
        return "exit-open";
    case EVENT_SHIELD_SAVED:
        return "shield";
    case EVENT_DEATH:
        return "death";
    case EVENT_SCORE:
        return "score";
    case EVENT_INFO:
        return "info";
    case EVENT_NONE:
    default:
        return "none";
    }
}
