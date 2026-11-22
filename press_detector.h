#pragma once

#include <stdint.h>

#include "button_press_e.h"

/*
 * Non-blocking press classifier for a debounced momentary input.
 * Feed it the raw "is down" state and the current time on every poll.
 *
 *  - LongPress is returned once, as soon as the input has been held for longPressMs
 *  - ShortPress is returned once, when the input is released after being held
 *    for at least minPressMs but less than longPressMs
 *  - After a LongPress, the release is swallowed
 *  - Releases are only accepted after the input has stayed up for releaseDebounceMs
 */
class PressDetector {
 public:
  PressDetector(const uint16_t minPressMs,
                const uint16_t longPressMs,
                const uint16_t releaseDebounceMs);

  ButtonPress_e update(const bool down, const uint32_t nowMs);

  //Ignores the input until it has been released, e.g. when it was already held at startup
  void reset();

  //True when no press is in progress and none is waiting to be released
  bool idle() const;

 private:
  enum class State : uint8_t { Idle, Down, WaitRelease };

  const uint16_t m_minPressMs;
  const uint16_t m_longPressMs;
  const uint16_t m_releaseDebounceMs;

  State m_state;
  uint32_t m_downSinceMs;
  uint32_t m_upSinceMs;
  bool m_isUp;//True once an up sample has been seen, until the next down sample
};
