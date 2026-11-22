#pragma once

#include <stdint.h>

bool IsButtonPressed();

//Configures the pin and enables its edge interrupt
void initTunerButton();

//Blocking; only for use in startup/modal routines where the main loop is not running
void WaitForTunerButtonRelease();

//Samples the button and posts a TunerPress event when a press completes. Never blocks.
void tunerButtonPoll(const uint32_t nowMs);

//Ignores the button until it is released, and forgets any partial press
void tunerButtonReset();
