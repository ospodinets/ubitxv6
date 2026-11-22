#include "press_detector.h"

PressDetector::PressDetector(const uint16_t minPressMs,
                             const uint16_t longPressMs,
                             const uint16_t releaseDebounceMs)
    : m_minPressMs(minPressMs),
      m_longPressMs(longPressMs),
      m_releaseDebounceMs(releaseDebounceMs),
      m_state(State::WaitRelease),
      m_downSinceMs(0),
      m_upSinceMs(0),
      m_isUp(false)
{
}

void PressDetector::reset()
{
  m_state = State::WaitRelease;
  m_isUp = false;
}

bool PressDetector::idle() const
{
  return State::Idle == m_state;
}

ButtonPress_e PressDetector::update(const bool down, const uint32_t nowMs)
{
  if (down) {
    m_isUp = false;
  } else if (!m_isUp) {
    m_isUp = true;
    m_upSinceMs = nowMs;
  }
  const bool releaseStable = m_isUp && (nowMs - m_upSinceMs >= m_releaseDebounceMs);

  switch (m_state) {
    case State::Idle:
      if (down) {
        m_state = State::Down;
        m_downSinceMs = nowMs;
      }
      break;

    case State::Down:
      if (down) {
        if (nowMs - m_downSinceMs >= m_longPressMs) {
          m_state = State::WaitRelease;
          return ButtonPress_e::LongPress;
        }
      } else if (releaseStable) {
        m_state = State::Idle;
        //The release is only stable after the debounce time, so exclude it from the held time
        if (m_upSinceMs - m_downSinceMs >= m_minPressMs) {
          return ButtonPress_e::ShortPress;
        }
      }
      break;

    case State::WaitRelease:
      if (releaseStable) {
        m_state = State::Idle;
      }
      break;
  }
  return ButtonPress_e::NotPressed;
}
