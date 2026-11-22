#pragma once

#include <stdint.h>

#include "button_press_e.h"
#include "point.h"

enum class EventType : uint8_t {
  TunerPress,//press holds the ButtonPress_e
  TouchPress,//press holds the ButtonPress_e, point holds the scaled touch position
  PttChanged //pressed holds the PTT state sampled in the interrupt
};

struct Event {
  EventType type;
  ButtonPress_e press;
  bool pressed;
  Point point;
  uint32_t timestampMs;
};

/*
 * Fixed-size FIFO shared between event producers (main loop pollers now,
 * interrupt handlers later) and the main loop consumer.
 * All functions are safe to call from interrupt context.
 */

//Returns false (and counts the drop) if the queue is full
bool eventPost(const Event& event);

//Returns false if the queue is empty
bool eventPop(Event* const event_out);

void eventClearAll();

//Number of events dropped because the queue was full
uint16_t eventDroppedCount();
