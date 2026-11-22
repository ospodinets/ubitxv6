#include "events.h"

#include <Arduino.h>

static const uint8_t EVENT_QUEUE_SIZE = 16;//Power of two

static Event queue[EVENT_QUEUE_SIZE];
static volatile uint8_t head = 0;//Next slot to write
static volatile uint8_t tail = 0;//Next slot to read
static volatile uint16_t droppedCount = 0;

//Saves and restores PRIMASK so these are safe from both thread and interrupt context
class CriticalSection {
 public:
  CriticalSection() : m_primask(__get_PRIMASK()) { __disable_irq(); }
  ~CriticalSection() { __set_PRIMASK(m_primask); }
 private:
  uint32_t m_primask;
};

bool eventPost(const Event& event)
{
  CriticalSection cs;
  const uint8_t next = (head + 1) & (EVENT_QUEUE_SIZE - 1);
  if (next == tail) {
    ++droppedCount;
    return false;
  }
  queue[head] = event;
  head = next;
  return true;
}

bool eventPop(Event* const event_out)
{
  CriticalSection cs;
  if (head == tail) {
    return false;
  }
  *event_out = queue[tail];
  tail = (tail + 1) & (EVENT_QUEUE_SIZE - 1);
  return true;
}

void eventClearAll()
{
  CriticalSection cs;
  tail = head;
}

uint16_t eventDroppedCount()
{
  CriticalSection cs;
  return droppedCount;
}
