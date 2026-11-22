#pragma once

#include <stdint.h>

void getBandString(const uint32_t frequency,
                   char* band_string_out,
                   const uint16_t max_string_length);

//Returns the frequency unchanged if it is inside a band, otherwise the edge of the nearest band
uint32_t clampToNearestBand(const uint32_t frequency);

uint32_t getFreqInBand(const uint32_t frequency,
                       const uint8_t target_band);

bool isFreqInBand(const uint32_t frequency,
                  const uint8_t band);

uint32_t getNextBandFreq(const uint32_t frequency, bool forward);