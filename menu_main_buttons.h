#pragma once

#include <stdint.h>

#include "button.h"

extern const Button* const mainMenuButtons[];
extern const uint8_t MAIN_MENU_NUM_BUTTONS;

void updateSplitButton();
void updateCWButton();
void updateRitButton();
void updateSidebandButton();

void drawTx();
void drawVBat();
void drawCallsign();
void drawVfo();
void invalidateVfo();
void drawBand();

void drawSMeter();
void updateSMeterIncremental(const int16_t smeter_value, const int16_t smeter_squelch);
void updateSMeterFromInput();
