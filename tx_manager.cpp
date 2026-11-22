#include "tx_manager.h"

#include <Arduino.h>

#include "pin_definitions.h"
#include "settings.h"
#include "tuner.h"

static TxSource owner = TxSource::None;
static uint32_t cwTailDeadlineMs = 0;

//Set when the TX state changes; the main loop redraws the TX indicator so that
//the radio control code never has to touch the display
static bool indicatorStale = false;

/**
 * Puts the uBitx in tx mode. It takes care of rit settings, sideband settings
 * Note: In cw mode, doesnt key the radio, only puts it in tx mode
 * CW offest is calculated as lower than the operating frequency when in LSB mode, and vice versa in USB mode
 */
static void enterTx(const TuningMode_e tx_mode)
{
  globalSettings.tuningMode = tx_mode;

  if (globalSettings.ritOn){
    //save the current as the rx frequency
    uint32_t rit_tx_freq = globalSettings.ritFrequency;
    globalSettings.ritFrequency = GetActiveVfoFreq();
    setFrequency(rit_tx_freq,true);
  }
  else{
    if(globalSettings.splitOn){
      if(Vfo_e::VFO_B == globalSettings.activeVfo){
        globalSettings.activeVfo = Vfo_e::VFO_A;
      }
      else{
        globalSettings.activeVfo = Vfo_e::VFO_B;
      }
    }
    setFrequency(GetActiveVfoFreq(),true);
  }

  digitalWrite(PIN_TX_RXn, 1);//turn on the tx
}

static void leaveTx()
{
  digitalWrite(PIN_TX_RXn, 0);//turn off the tx

  if(globalSettings.ritOn){
    uint32_t rit_rx_freq = globalSettings.ritFrequency;
    globalSettings.ritFrequency = GetActiveVfoFreq();
    setFrequency(rit_rx_freq);
  }
  else{
    if(globalSettings.splitOn){
      if(Vfo_e::VFO_B == globalSettings.activeVfo){
        globalSettings.activeVfo = Vfo_e::VFO_A;
      }
      else{
        globalSettings.activeVfo = Vfo_e::VFO_B;
      }
    }
    setFrequency(GetActiveVfoFreq());
  }
}

bool txIsActive()
{
  return TxSource::None != owner;
}

TxSource txSource()
{
  return owner;
}

bool txStart(const TxSource source)
{
  if(TxSource::None == source){
    return false;
  }
  if(txIsActive()){
    return source == owner;
  }

  enterTx(TxSource::CwKey == source ? TuningMode_e::TUNE_CW : TuningMode_e::TUNE_SSB);
  owner = source;
  if(TxSource::CwKey == source){
    txCwTailRestart();
  }
  indicatorStale = true;
  return true;
}

void txStop(const TxSource source)
{
  if(TxSource::None == source || source != owner){
    return;
  }

  leaveTx();
  owner = TxSource::None;
  indicatorStale = true;
}

void txCwTailRestart()
{
  cwTailDeadlineMs = millis() + globalSettings.cwActiveTimeoutMs;
}

void txStopIfCwTailExpired(const uint32_t nowMs)
{
  if(TxSource::CwKey == owner && static_cast<int32_t>(nowMs - cwTailDeadlineMs) >= 0){
    txStop(TxSource::CwKey);
  }
}

bool consumeTxIndicatorStale()
{
  const bool stale = indicatorStale;
  indicatorStale = false;
  return stale;
}
