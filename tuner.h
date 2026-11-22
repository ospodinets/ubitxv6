#pragma once

#include "settings.h"

void saveVFOs();
void setFrequency(const unsigned long freq, const bool transmit = false);
void ritEnable(unsigned long f);
void ritDisable();
void cwKeyerUpdate(const uint32_t nowMs);//Non-blocking; call on every pass of the main loop
bool cwKeyerCanBlockFor(const uint32_t nowMs, const uint32_t durationMs);//True if the keyer timing tolerates the loop being busy that long
void startCwPractice(uint16_t frequency, uint16_t ditDurationMs);
void updateCwPractice();
void stopCwPractice();
void switchVFO(Vfo_e vfoSelect);

bool autoSelectSidebandChanged(const uint32_t old_frequency); //if the current frequency defaults to a different sideband mode, updates to that sideband mode and returns true. Else, returns false
