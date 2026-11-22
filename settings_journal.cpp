#include "settings_journal.h"

#include <string.h>

namespace SettingsJournal {
namespace {

constexpr uint32_t SECTOR_SIZE = 128 * 1024;
constexpr uint32_t SECTORS[] = {0x08040000, 0x08060000};
constexpr uint32_t MAGIC = 0x534a5231;
constexpr uint32_t COMMITTED = 0x434f4d31;
constexpr uint32_t ERASED = 0xffffffff;

struct Record {
  uint32_t magic;
  uint32_t sequence;
  uint8_t image[IMAGE_SIZE];
  uint32_t checksum;
  uint32_t committed;
};

static_assert(sizeof(Record) == 732, "Unexpected journal record layout");
constexpr size_t SLOTS = SECTOR_SIZE / sizeof(Record);

struct Scan {
  bool found = false;
  bool invalid = false;
  uint32_t sequence = 0;
  int sector = -1;
  size_t nextSlot[2] = {0, 0};
};

uint32_t checksum(const Record& record)
{
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&record);
  uint32_t crc = 0xffffffff;
  for(size_t i = 0; i < offsetof(Record, checksum); ++i){
    crc ^= bytes[i];
    for(unsigned int bit = 0; bit < 8; ++bit){
      crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
  }
  return ~crc;
}

void readRecord(uint32_t address, Record& record)
{
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&record);
  for(size_t i = 0; i < sizeof(record); i += sizeof(uint32_t)){
    const uint32_t word = readWord(address + i);
    memcpy(bytes + i, &word, sizeof(word));
  }
}

bool newer(uint32_t a, uint32_t b)
{
  return a != b && (a - b) < 0x80000000u;
}

Scan scan(uint8_t* image)
{
  Scan result;
  for(int sector = 0; sector < 2; ++sector){
    bool foundInSector = false;
    for(size_t slot = SLOTS; slot > 0; --slot){
      const uint32_t address = SECTORS[sector] + (slot - 1) * sizeof(Record);
      bool occupied = false;
      for(size_t offset = 0; offset < sizeof(Record); offset += sizeof(uint32_t)){
        if(readWord(address + offset) != ERASED){
          occupied = true;
          break;
        }
      }
      if(!occupied){
        continue;
      }
      if(result.nextSlot[sector] == 0){
        result.nextSlot[sector] = slot;
      }
      if(foundInSector || readWord(address) != MAGIC){
        continue;
      }
      Record record;
      readRecord(address, record);
      if(record.committed != COMMITTED || record.checksum != checksum(record)){
        result.invalid = true;
        continue;
      }
      foundInSector = true;
      if(!result.found || newer(record.sequence, result.sequence)){
        result.found = true;
        result.sequence = record.sequence;
        result.sector = sector;
        if(image != nullptr){
          memcpy(image, record.image, IMAGE_SIZE);
        }
      }
    }
  }
  return result;
}

}

bool load(uint8_t* image, bool& invalidRecords)
{
  const Scan result = scan(image);
  invalidRecords = result.invalid;
  return result.found;
}

bool save(const uint8_t* image, bool& erasedSector)
{
  const Scan state = scan(nullptr);
  int sector = state.found ? state.sector : 0;
  bool erase = false;
  if(state.nextSlot[sector] >= SLOTS){
    sector = 1 - sector;
    erase = true;
  }
  else if(!state.found && state.nextSlot[sector] != 0){
    // Migration must leave the legacy EEPROM image in sector 7 intact.
    erase = true;
  }

  Record record;
  record.magic = MAGIC;
  record.sequence = state.found ? state.sequence + 1 : 0;
  memcpy(record.image, image, IMAGE_SIZE);
  record.checksum = checksum(record);
  record.committed = COMMITTED;

  erasedSector = false;
  if(!unlock()){
    return false;
  }
  bool ok = true;
  if(erase){
    ok = eraseSector(static_cast<unsigned int>(sector + 6));
    erasedSector = ok;
  }
  const size_t slot = erase ? 0 : state.nextSlot[sector];
  const uint32_t address = SECTORS[sector] + slot * sizeof(Record);
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&record);
  // The commit word is programmed last; incomplete records are ignored on boot.
  for(size_t offset = 0; ok && offset < sizeof(record); offset += sizeof(uint32_t)){
    uint32_t word;
    memcpy(&word, bytes + offset, sizeof(word));
    if(word != ERASED){
      ok = programWord(address + offset, word);
    }
  }
  if(!lock()){
    ok = false;
  }
  if(ok){
    Record verified;
    readRecord(address, verified);
    ok = memcmp(&record, &verified, sizeof(record)) == 0;
  }
  return ok;
}

}
