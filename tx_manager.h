#pragma once

#include <stdint.h>

/*
 * Owns the transmit state. Whoever wants to transmit asks for it with a source,
 * and only that source can end it, so a stale release or timeout from one source
 * can't stop a transmission started by another.
 */
enum class TxSource : uint8_t {
  None,
  Ptt,  //SSB, held with the PTT input
  CwKey //CW, keyed by the keyer
};

bool txIsActive();
TxSource txSource();

//Puts the radio in TX if it is idle. Returns true if the source owns TX afterwards,
//false if another source already does.
bool txStart(const TxSource source);

//Puts the radio back in RX, but only if the source owns TX
void txStop(const TxSource source);

//CW only: restarts the hang time that keeps TX on after the key is released
void txCwTailRestart();

//CW only: ends TX if the hang time has run out. The keyer calls this when it is idle,
//so that TX isn't dropped in the middle of a character.
void txStopIfCwTailExpired(const uint32_t nowMs);

//Returns true once after each TX state change, meaning the TX indicator needs redrawing
bool consumeTxIndicatorStale();
