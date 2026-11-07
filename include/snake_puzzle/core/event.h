#ifndef SNAKE_PUZZLE_CORE_EVENT_H
#define SNAKE_PUZZLE_CORE_EVENT_H

#include "snake_puzzle/common.h"

typedef enum EventType {
    EVENT_NONE = 0,
    EVENT_MOVE,
    EVENT_EAT_FOOD,
    EVENT_PICKUP_ITEM,
    EVENT_PUSH_BLOCK,
    EVENT_PLATE_PRESSED,
    EVENT_GATE_OPENED,
    EVENT_TELEPORT,
    EVENT_BREAK_FLOOR,
    EVENT_SEAL_COLLECTED,
    EVENT_SPECIAL_UNLOCKED,
    EVENT_ENTER_SPECIAL,
    EVENT_SPECIAL_FINISHED,
    EVENT_ORB_COLLECTED,
    EVENT_MINI_UNLOCKED,
    EVENT_SHIELD_SAVED,
    EVENT_DEATH,
    EVENT_SCORE,
    EVENT_INFO
} EventType;

typedef struct Event {
    EventType type;
    int x;
    int y;
    int score;
    char message[SP_MESSAGE_MAX];
} Event;

typedef struct EventQueue {
    Event events[SP_MAX_EVENTS];
    int head;
    int count;
} EventQueue;

void event_queue_init(EventQueue *queue);
void event_queue_clear(EventQueue *queue);
void event_push(EventQueue *queue, EventType type, int x, int y, int score,
                const char *format, ...);
bool event_poll(EventQueue *queue, Event *out);
const Event *event_last(const EventQueue *queue);
const char *event_type_name(EventType type);

#endif
