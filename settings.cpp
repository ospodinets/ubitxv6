#include <string.h>//memset
#include <stdint.h>
#include <EEPROM.h>
#include <Arduino.h>//only needed for debugging's Serial.print stuff
#include "nano_gui.h"//redrawVFOs() function
#include "settings.h"
#include "settings_journal.h"
#include "si5351.h"

/**
 * These are the "magic" indices where these user changable settinngs are stored in the EEPROM
 */
static const uint16_t EEPROM_ADDR_MASTER_CAL = 0;//int32_t
//4 is currently unused, but may have been LSB_CAL on other versions
static const uint16_t EEPROM_ADDR_USB_CAL = 8;//uint32_t
//12 is currently unused, but may have been CW_SIDETONE on other versions?
static const uint16_t EEPROM_ADDR_VFO_A_FREQ  = 16;//uint32_t
static const uint16_t EEPROM_ADDR_VFO_B_FREQ  = 20;//uint32_t
static const uint16_t EEPROM_ADDR_CW_SIDETONE  = 24;//uint32_t
static const uint16_t EEPROM_ADDR_CW_DIT_TIME  = 28;//uint32_t
static const uint16_t EEPROM_ADDR_TOUCH_SLOPE_X  = 32;//int16_t
static const uint16_t EEPROM_ADDR_TOUCH_SLOPE_Y  = 36;//int16_t
static const uint16_t EEPROM_ADDR_TOUCH_OFFSET_X  = 40;//int16_t
static const uint16_t EEPROM_ADDR_TOUCH_OFFSET_Y  = 44;//int16_t
static const uint16_t EEPROM_ADDR_CW_DELAYTIME  = 48;//uint16_t
static const uint16_t EEPROM_ADDR_VFO_A_MODE = 256;//uint8_t
static const uint16_t EEPROM_ADDR_VFO_B_MODE = 257;//uint8_t
static const uint16_t EEPROM_ADDR_CW_KEY_TYPE = 358;//uint8_t
static const uint16_t EEPROM_ADDR_QUICKLIST_FREQ = 630;//uint32_t array of size NUM_QUICKLIST_SETTINGS
static const uint16_t EEPROM_ADDR_QUICKLIST_MODE = 710;//uint8_t array of size NUM_QUICKLIST_SETTINGS

static uint8_t persistedSettings[SettingsJournal::IMAGE_SIZE];
static bool settingsImageLoaded = false;

static void LoadSettingsImage()
{
  if(settingsImageLoaded){
    return;
  }
  bool invalidRecords = false;
  if(!SettingsJournal::load(persistedSettings, invalidRecords)){
    eeprom_buffer_fill();
    for(size_t i = 0; i < sizeof(persistedSettings); ++i){
      persistedSettings[i] = eeprom_buffered_read_byte(i);
    }
    Serial.println(F("Settings: using legacy EEPROM image"));
  }
  if(invalidRecords){
    Serial.println(F("Settings: WARNING, incomplete or corrupt journal record"));
  }
  settingsImageLoaded = true;
}

template<typename T>
static void ReadSetting(uint16_t address, T& value)
{
  memcpy(&value, persistedSettings + address, sizeof(value));
}

template<typename T>
bool LoadSane(T& dest,uint16_t addr, T min, T max)
{
    T read_value;
    ReadSetting(addr,read_value);
    if((min <= read_value) && (read_value <= max)){
      dest = read_value;
      //Serial.print(addr);
      //Serial.print(F(":"));
      //Serial.println(dest);
      return true;
    }
    //Serial.print(addr);
    //Serial.print(F(": Not valid: "));
    //Serial.print(read_value);
    //Serial.print(F(" Leaving value at "));
    //Serial.println(dest);
    return false;
}

//This is the non-extern version that actually gets built
SettingsRam globalSettings;

void LoadDefaultSettings()
{
  memset(&globalSettings,0x00,sizeof(globalSettings));

  globalSettings.oscillatorCal = 0L;
  globalSettings.usbCarrierFreq = 11052000UL;

  globalSettings.activeVfo = Vfo_e::VFO_A;
  globalSettings.vfoA.frequency = 7150000UL;
  globalSettings.vfoA.mode = VFO_MODE_LSB;
  globalSettings.vfoB.frequency = 14150000UL;
  globalSettings.vfoB.mode = VFO_MODE_USB;

  for(uint8_t i = 0; i < NUM_QUICKLIST_SETTINGS; ++i){
    globalSettings.quickList[i].frequency = i*1000000;
    globalSettings.quickList[i].mode = VfoMode_e::VFO_MODE_LSB;
  }

  globalSettings.keyerMode = KEYER_STRAIGHT;
  globalSettings.cwSideToneFreq = 800;
  globalSettings.cwDitDurationMs = 100;
  globalSettings.cwActiveTimeoutMs = 500;//Within the 100-1000 ms range allowed by the settings screen

  globalSettings.touchSlopeX = 104;
  globalSettings.touchSlopeY = 137;
  globalSettings.touchOffsetX = 28;
  globalSettings.touchOffsetY = 29;

  globalSettings.ritOn = false;
  globalSettings.ritFrequency = globalSettings.vfoA.frequency;

  globalSettings.tuningMode = TuningMode_e::TUNE_SSB;

  globalSettings.splitOn = false;

  globalSettings.sMeterValue = 0;
}

void LoadSettingsFromEeprom()
{
  LoadSettingsImage();
  LoadSane(globalSettings.usbCarrierFreq,EEPROM_ADDR_USB_CAL,static_cast<uint32_t>(11048000),static_cast<uint32_t>(11060000));
  LoadSane(globalSettings.vfoA.frequency,EEPROM_ADDR_VFO_A_FREQ,SI5351_MIN_FREQUENCY_HZ,SI5351_MAX_FREQUENCY_HZ);
  LoadSane(globalSettings.vfoB.frequency,EEPROM_ADDR_VFO_B_FREQ,SI5351_MIN_FREQUENCY_HZ,SI5351_MAX_FREQUENCY_HZ);
  LoadSane(globalSettings.cwSideToneFreq,EEPROM_ADDR_CW_SIDETONE,static_cast<uint32_t>(100),static_cast<uint32_t>(2000));
  LoadSane(globalSettings.cwDitDurationMs,EEPROM_ADDR_CW_DIT_TIME,static_cast<uint16_t>(10),static_cast<uint16_t>(1000));
  if(LoadSane(globalSettings.cwActiveTimeoutMs,EEPROM_ADDR_CW_DELAYTIME,static_cast<uint16_t>(10) ,static_cast<uint16_t>(100))){
    globalSettings.cwActiveTimeoutMs *= 10;//scale by 10 for legacy reasons
  }
  LoadSane(globalSettings.vfoA.mode,EEPROM_ADDR_VFO_A_MODE,VFO_MODE_LSB,VFO_MODE_USB);
  LoadSane(globalSettings.vfoB.mode,EEPROM_ADDR_VFO_B_MODE,VFO_MODE_LSB,VFO_MODE_USB);
  LoadSane(globalSettings.keyerMode,EEPROM_ADDR_CW_KEY_TYPE,KEYER_STRAIGHT,KEYER_IAMBIC_B);

  for(uint8_t i = 0; i < NUM_QUICKLIST_SETTINGS; ++i){
    LoadSane(globalSettings.quickList[i].frequency,EEPROM_ADDR_QUICKLIST_FREQ+(sizeof(uint32_t)*i),SI5351_MIN_FREQUENCY_HZ,SI5351_MAX_FREQUENCY_HZ);
    LoadSane(globalSettings.quickList[i].mode,EEPROM_ADDR_QUICKLIST_MODE+(sizeof(uint8_t)*i),VFO_MODE_LSB,VFO_MODE_USB);
  }

  ReadSetting(EEPROM_ADDR_MASTER_CAL,globalSettings.oscillatorCal);
  int16_t slopeX, slopeY, offsetX, offsetY;
  ReadSetting(EEPROM_ADDR_TOUCH_SLOPE_X,slopeX);
  ReadSetting(EEPROM_ADDR_TOUCH_SLOPE_Y,slopeY);
  ReadSetting(EEPROM_ADDR_TOUCH_OFFSET_X,offsetX);
  ReadSetting(EEPROM_ADDR_TOUCH_OFFSET_Y,offsetY);
  const bool erased = slopeX == -1 && slopeY == -1 && offsetX == -1 && offsetY == -1;
  if(slopeX != 0 && slopeY != 0 && !erased){
    globalSettings.touchSlopeX = slopeX;
    globalSettings.touchSlopeY = slopeY;
    globalSettings.touchOffsetX = offsetX;
    globalSettings.touchOffsetY = offsetY;
  }
  else{
    Serial.println(F("TOUCH: invalid EEPROM calibration; retaining defaults"));
  }
}

struct EepromSettingsBatch
{
  uint8_t* image;
  uint16_t changedBytes = 0;

  template<typename T>
  void put(uint16_t address, const T& value)
  {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    for(size_t i = 0; i < sizeof(value); ++i){
      if(image[address + i] != bytes[i]){
        ++changedBytes;
        image[address + i] = bytes[i];
      }
    }
  }
};

static void PutAllSettings(EepromSettingsBatch& batch)
{
  batch.put(EEPROM_ADDR_MASTER_CAL,globalSettings.oscillatorCal);
  batch.put(EEPROM_ADDR_USB_CAL,globalSettings.usbCarrierFreq);
  batch.put(EEPROM_ADDR_VFO_A_FREQ,globalSettings.vfoA.frequency);
  batch.put(EEPROM_ADDR_VFO_B_FREQ,globalSettings.vfoB.frequency);
  batch.put(EEPROM_ADDR_CW_SIDETONE,globalSettings.cwSideToneFreq);
  batch.put(EEPROM_ADDR_CW_DIT_TIME,globalSettings.cwDitDurationMs);
  batch.put(EEPROM_ADDR_TOUCH_SLOPE_X,globalSettings.touchSlopeX);
  batch.put(EEPROM_ADDR_TOUCH_SLOPE_Y,globalSettings.touchSlopeY);
  batch.put(EEPROM_ADDR_TOUCH_OFFSET_X,globalSettings.touchOffsetX);
  batch.put(EEPROM_ADDR_TOUCH_OFFSET_Y,globalSettings.touchOffsetY);
  batch.put(EEPROM_ADDR_CW_DELAYTIME,globalSettings.cwActiveTimeoutMs/10);//scale by 10 for legacy reasons
  batch.put(EEPROM_ADDR_VFO_A_MODE,globalSettings.vfoA.mode);
  batch.put(EEPROM_ADDR_VFO_B_MODE,globalSettings.vfoB.mode);
  batch.put(EEPROM_ADDR_CW_KEY_TYPE,globalSettings.keyerMode);

  for(uint8_t i = 0; i < NUM_QUICKLIST_SETTINGS; ++i){
    batch.put(EEPROM_ADDR_QUICKLIST_FREQ+(sizeof(uint32_t)*i),globalSettings.quickList[i].frequency);
    batch.put(EEPROM_ADDR_QUICKLIST_MODE+(sizeof(uint8_t)*i),globalSettings.quickList[i].mode);
  }
}

bool SaveSettingsToEeprom()
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  const bool timerAvailable = (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) == 0;
  if(timerAvailable){
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  }
  Serial.println(F("EEPROM save: start"));

  const uint32_t start = DWT->CYCCNT;
  LoadSettingsImage();
  uint8_t image[SettingsJournal::IMAGE_SIZE];
  memcpy(image, persistedSettings, sizeof(image));
  EepromSettingsBatch batch{image};
  PutAllSettings(batch);

  uint32_t writeCycles = 0;
  bool erasedSector = false;
  bool saved = true;
  if(batch.changedBytes != 0){
    const uint32_t writeStart = DWT->CYCCNT;
    saved = SettingsJournal::save(image, erasedSector);
    writeCycles = DWT->CYCCNT - writeStart;
    if(saved){
      memcpy(persistedSettings, image, sizeof(image));
    }
  }
  const uint32_t elapsedCycles = DWT->CYCCNT - start;

  if(!saved){
    Serial.println(F("EEPROM save: ERROR, journal write or verification failed"));
  }
  Serial.print(F("EEPROM save: changed bytes="));
  Serial.print(batch.changedBytes);
  Serial.print(F(" sector erased="));
  Serial.print(erasedSector ? 1 : 0);
  if(timerAvailable && SystemCoreClock >= 1000){
    const uint32_t cyclesPerMs = SystemCoreClock / 1000;
    Serial.print(F(" total ms="));
    Serial.print(elapsedCycles / cyclesPerMs);
    Serial.print(F(" journal ms="));
    Serial.println(writeCycles / cyclesPerMs);
  }
  else{
    Serial.println(F(" timing unavailable"));
  }
  return saved;
}

uint32_t GetActiveVfoFreq()
{
  if(VFO_A == globalSettings.activeVfo){
    return globalSettings.vfoA.frequency;
  }
  else{
    return globalSettings.vfoB.frequency;
  }
}

void SetActiveVfoFreq(uint32_t frequency)
{
  if(VFO_A == globalSettings.activeVfo)
  {
    globalSettings.vfoA.frequency = frequency;
  }
  else{
    globalSettings.vfoB.frequency = frequency;
  }
}

VfoMode_e GetActiveVfoMode()
{
  if(VFO_A == globalSettings.activeVfo){
    return globalSettings.vfoA.mode;
  }
  else{
    return globalSettings.vfoB.mode;
  }
}

void SetActiveVfoMode(VfoMode_e mode)
{
  if(VFO_A == globalSettings.activeVfo)
  {
    globalSettings.vfoA.mode = mode;
  }
  else{
    globalSettings.vfoB.mode = mode;
  }
}
