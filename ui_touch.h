#pragma once

#include <stdint.h>

//Samples the touch screen and posts a TouchPress event (with calibrated point)
//when a press completes. Never blocks.
void touchPoll(const uint32_t nowMs);

//Ignores the touch screen until it is released, and forgets any partial press
void touchReset();
