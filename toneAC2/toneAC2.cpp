#include "toneAC2.h"

static const uint8_t NO_ACTIVE_PIN = 0xFF;
static uint8_t activePin = NO_ACTIVE_PIN;

static void stopTone(bool destruct)
{
  if (activePin == NO_ACTIVE_PIN) {
    return;
  }

  noTone(activePin, destruct);
  digitalWrite(activePin, LOW);
  activePin = NO_ACTIVE_PIN;
}

void toneAC2(uint8_t pin, unsigned int frequency, unsigned long length, uint8_t background)
{
  if (frequency == 0) {
    stopTone(false);
    return;
  }

  if (activePin != NO_ACTIVE_PIN && activePin != pin) {
    stopTone(true);
  }

  activePin = pin;
  if (length > 0 && !background) {
    tone(pin, frequency);
    delay(length);
    noToneAC2();
  } else {
    tone(pin, frequency, length);
  }
}

void noToneAC2()
{
  stopTone(false);
}
