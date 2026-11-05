#pragma once

#include <stdint.h>

#include "button.h"

extern const Button* const mainMenuButtons[];
extern const uint8_t MAIN_MENU_NUM_BUTTONS;

extern Button bRit;
extern Button bCw;
extern Button bSpl;

void updateSidebandButton();
void drawTx();
void drawVersion();
void drawCallsign();
void drawVfo();
void invalidateVfo();
void drawBand();
