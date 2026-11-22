#pragma once

#include <stdint.h>

static const uint16_t LONG_PRESS_TIME_MS = 3000;
//The tuner button and PTT have a hardware RC debouncer, so no software debounce is needed for them
static const uint8_t BUTTON_RELEASE_DEBOUNCE_MS = 0;
static const uint8_t BUTTON_MIN_PRESS_MS = 0;
static const uint8_t TOUCH_RELEASE_DEBOUNCE_MS = 20;
static const uint8_t TOUCH_MIN_PRESS_MS = 50;