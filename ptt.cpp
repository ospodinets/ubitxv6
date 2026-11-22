#include "ptt.h"

#include <Arduino.h>

#include "events.h"
#include "pin_definitions.h"

bool IsPttPressed()
{
  return LOW == digitalRead(PIN_PTT);
}

//The level is sampled here rather than inferred from the edge, so handling an event is
//idempotent and the last event of a burst always carries the final state
static void pttIsr()
{
  Event event = {};
  event.type = EventType::PttChanged;
  event.pressed = IsPttPressed();
  event.timestampMs = millis();
  eventPost(event);
}

void initPtt()
{
  pinMode(PIN_PTT, INPUT);//External pull-up and hardware debounce
}

void enablePttEvents()
{
  attachInterrupt(digitalPinToInterrupt(PIN_PTT), pttIsr, CHANGE);
}
