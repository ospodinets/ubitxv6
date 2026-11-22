#pragma once

#include <stddef.h>
#include <stdint.h>

namespace SettingsJournal {

// The image uses the existing EEPROM byte addresses through the last quick-list entry.
constexpr size_t IMAGE_SIZE = 716;

bool load(uint8_t* image, bool& invalidRecords);
bool save(const uint8_t* image, bool& erasedSector);

// Implemented by the target's flash backend.
uint32_t readWord(uint32_t address);
bool unlock();
bool lock();
bool programWord(uint32_t address, uint32_t value);
bool eraseSector(unsigned int sector);

}
