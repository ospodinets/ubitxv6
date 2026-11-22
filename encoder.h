#pragma once

#include <stdint.h>

void enc_setup(void);
int enc_read(void);

//Reads the encoder and posts a KnobTurned event if it moved. Never blocks.
void encoderPoll(const uint32_t nowMs);

//Forgets any rotation not yet reported, e.g. made while the polling was suspended
void encoderReset(void);
