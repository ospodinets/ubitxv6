#include <Arduino.h>
#include "button_timing.h"
#include "events.h"
#include "pin_definitions.h"
#include "press_detector.h"
#include "push_button.h"

static PressDetector tunerButtonDetector(BUTTON_MIN_PRESS_MS,
                                         LONG_PRESS_TIME_MS,
                                         BUTTON_RELEASE_DEBOUNCE_MS);
static volatile bool tunerButtonWakePending = true;//Start true so a button held at boot is seen
static bool tunerButtonSequenceActive = true;//Keep reading until the button is seen released

//Only flags the edge; the press timing and debouncing are done in tunerButtonPoll()
static void tunerButtonIsr()
{
  tunerButtonWakePending = true;
}

void initTunerButton()
{
  pinMode(PIN_ENC_PUSH_BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_PUSH_BUTTON), tunerButtonIsr, CHANGE);
}

bool IsButtonPressed()
{
  //Button has a pullup, so it reads high normally,
  //and reads low when pressed down
  return !digitalRead(PIN_ENC_PUSH_BUTTON);
}

void WaitForTunerButtonRelease()
{
  while(IsButtonPressed()){
    delay(1);
  }
}

void tunerButtonPoll(const uint32_t nowMs)
{
  //Don't sample the pin while idle unless an edge was reported.
  //The flag is consumed unconditionally so an edge can't be left over from an earlier sequence.
  const bool wake = tunerButtonWakePending;
  tunerButtonWakePending = false;
  if(!wake && !tunerButtonSequenceActive){
    return;
  }

  const bool down = IsButtonPressed();
  const ButtonPress_e press = tunerButtonDetector.update(down, nowMs);
  tunerButtonSequenceActive = down || !tunerButtonDetector.idle();
  if(ButtonPress_e::NotPressed != press){
    Event event = {};
    event.type = EventType::TunerPress;
    event.press = press;
    event.timestampMs = nowMs;
    eventPost(event);
  }
}

void tunerButtonReset()
{
  tunerButtonDetector.reset();
  tunerButtonSequenceActive = true;
}
