#include <Arduino.h>

#include "settings_journal.h"

#if !defined(STM32F411xE)
#error "The settings journal flash layout requires an STM32F411CE"
#endif

namespace SettingsJournal {

uint32_t readWord(uint32_t address)
{
  return *reinterpret_cast<const volatile uint32_t*>(address);
}

bool unlock()
{
  if(HAL_FLASH_Unlock() != HAL_OK){
    return false;
  }
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                         FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
  return true;
}

bool lock()
{
  const bool locked = HAL_FLASH_Lock() == HAL_OK;
  FLASH_FlushCaches();
  return locked;
}

bool programWord(uint32_t address, uint32_t value)
{
  return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value) == HAL_OK;
}

bool eraseSector(unsigned int sector)
{
  FLASH_EraseInitTypeDef erase = {};
  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  erase.Sector = sector;
  erase.NbSectors = 1;
  uint32_t error = 0;
  return HAL_FLASHEx_Erase(&erase, &error) == HAL_OK;
}

}
