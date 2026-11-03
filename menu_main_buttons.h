#pragma once

#include <stdint.h>

#include "button.h"

extern const Button* const mainMenuButtons[];
extern const uint8_t MAIN_MENU_NUM_BUTTONS;

extern const Button bRit;
extern const Button bCw;
extern const Button bSpl;

void updateSidebandButton();
void drawTx();
void drawVersion();
void drawCallsign();
void drawVfo();
void drawBand();
