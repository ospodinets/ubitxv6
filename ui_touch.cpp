#include "ui_touch.h"

#include "button_timing.h"
#include "events.h"
#include "press_detector.h"
#include "touch.h"

static PressDetector touchDetector(TOUCH_MIN_PRESS_MS,
                                   LONG_PRESS_TIME_MS,
                                   TOUCH_RELEASE_DEBOUNCE_MS);
static Point lastTouchPoint = {0, 0};
static bool touchSequenceActive = true;//Keep reading until the screen is seen released

void touchPoll(const uint32_t nowMs)
{
#if TOUCH_USE_IRQ
  //Don't touch the SPI bus while idle unless the interrupt reported a touch.
  //The flag is consumed unconditionally so an edge can't be left over from an earlier sequence.
  if(!touchIrqConsume() && !touchSequenceActive){
    return;
  }
#endif

  Point point;
  const bool touched = readTouch(&point);
  if(touched){
    lastTouchPoint = point;
  }

  const ButtonPress_e press = touchDetector.update(touched, nowMs);
  touchSequenceActive = touched || !touchDetector.idle();
  if(ButtonPress_e::NotPressed != press){
    Event event = {};
    event.type = EventType::TouchPress;
    event.press = press;
    event.point = lastTouchPoint;
    scaleTouch(&event.point);
    event.timestampMs = nowMs;
    eventPost(event);
  }
}

void touchReset()
{
  touchDetector.reset();
  touchSequenceActive = true;
}
