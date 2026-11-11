// ---------------------------------------------------------------------------
// toneAC2 Library - v1.1 - 09/15/2015
//
// AUTHOR/LICENSE:
// Created by Tim Eckel - teckel@leethost.com
// Copyright 2015 License: GNU GPL v3 http://www.gnu.org/licenses/gpl-3.0.html
//
// LINKS:
// Project home: https://bitbucket.org/teckel12/arduino-toneac/wiki/Home
// Blog: http://forum.arduino.cc/index.php?topic=142097.0
//
// DISCLAIMER:
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//
// PURPOSE:
// Compatibility wrapper for the existing toneAC2 call sites. On STM32 it
// delegates tone generation to the stm32duino timer-backed tone() API.
//
// USAGE:
// Connect the speaker or piezo between the selected output pin and ground.
//
// SYNTAX:
//   toneAC2( pin, frequency [, length [, background ]] ) - Play a note.
//     Parameters:
//       * pin        - Output pin.
//       * frequency  - Play the specified frequency indefinitely, turn off with noToneAC2().
//       * length     - [optional] Set the length to play in milliseconds. (default: 0 [forever], range: 0 to 2^32-1)
//       * background - [optional] true plays asynchronously; false blocks until length elapses.
//   noToneAC2()  - Stop playing.
//
// HISTORY:
// 09/15/2015 v1.1 - Fix a potential race condition with _tAC2_time. Moved
// development to Bitbucket.
//
// 01/27/2013 v1.0 - Initial release.
//
// ---------------------------------------------------------------------------

#ifndef toneAC2_h
  #define toneAC2_h

  #if defined(ARDUINO) && ARDUINO >= 100
    #include <Arduino.h>
  #else
    #include <WProgram.h>
  #endif

  void toneAC2(uint8_t pin, unsigned int frequency = 0, unsigned long length = 0, uint8_t background = false);
  void noToneAC2();
#endif