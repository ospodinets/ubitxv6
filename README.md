# uBiTXv6

This project is found at https://github.com/reedbn/ubitxv6/

It was forked from https://github.com/afarhan/ubitxv6/

The purpose of this project is to clean up (modularize) the source code, and add features that were not present
in Ashhar's original version of the project, without requiring any hardware modifications to a stock uBiTXv6.

New features include:

* Much faster screen refresh (vs Ashhar's 6.3.1 aka 6.0 release)
* Morse code readback for sightless operation
* Save/recall your favorite frequencies
* When adjusting settings, the existing/current setting is shown as reference
* Cancel touch recalibration

User Manual: https://docs.google.com/document/d/1jlllZbvFMCzO1MJLzlJDGb10HXSehlFNMDPsxGJZtvY/edit?usp=drivesdk

# Installing on Your Radio

There are plenty of tutorials on how to upload sketches to Arduino Nanos. Just search for them. Addtionally,
Ashhar created a video explaining the process specifically for uBiTX v6: https://www.youtube.com/watch?v=3n_V3prSJ_E

I developed this code using the Arduino IDE 1.8.9 toolchain, with -Wall and -Wextra compiler options turned on.
Arduino IDE 1.8.13 was reported to compile too big (see https://groups.io/g/BITX20/topic/75008576), but this
should be resolved in this project's tag R1.5.1.

# Personalized Callsign

To edit the callsign displayed, open the file `callsign.cpp` and change the string. Then re-compile and upload.

# Settings storage and timing

On the Black Pill STM32F411CE, settings use a flash journal in sectors 6 and 7. The firmware is limited to the first 256 KiB so it cannot overlap these sectors. Existing EEPROM settings are loaded when no journal record exists and migrated on the first changed save. Back up settings before flashing: some upload tools erase all flash, including settings. Older firmware does not understand the journal and can erase it.

Changed settings are appended as a checksummed record; unchanged saves do not write flash. When a sector fills, the other is erased before the next record is committed. A previous valid record remains available if power is lost during recycling. Erasing a sector can still cause an occasional noticeable pause.

The active VFO is saved when settings are saved and restored on startup. Older settings without an active VFO selection default to VFO A.

Diagnostic traces are disabled by default. To enable them, uncomment `-D ENABLE_TRACE=1` in `platformio.ini` and rebuild. The serial monitor (38400 baud) then reports touch activity and settings-save timing, including the changed byte count and whether a sector was erased. Warnings and errors remain enabled regardless of this flag. Serial output is excluded from timings. Timings use the STM32 cycle counter because flash operations can stall SysTick; they assume a constant CPU clock and a save shorter than one counter wrap (about 43 seconds at 100 MHz).

# Future Features/Modifications

There are some features that would be good to add, but I just didn't get around to.

* Setting to choose the tuning step size
* Setting to choose whether or not the knob tuning should accelerate (current behavior) or have a fixed interval
* Provide an option in each menu screen to load the default option for each setting

While the current code (as of 2020-05-05) is ~100 bytes shy of the full 30720 available on the nano, there's still
opportunity to add new features by "creating" room. Below is a list of places you might create room:

I added lots of bounds checking, especially on string writes, that, if removed, could free a good number of bytes.
While keeping them is best practice, for a non-IoT, non-critical piece of hardware, it shouldn't be a huge issue.

I added a bunch of strings to the menuing with the intention of helping people understand their functions, but
technically they're not necessary, and could all be removed.

I switched to a smaller footprint font than Ashhar's original code, but there are MUCH smaller fonts out there.
Changing to a lower resolution, scaled up font can save hundreds or thousands of bytes, but won't look as pretty.
Also, the star, gear, and numpad icons will need to be either added to the new font, or replaced with characters.

The first change I made to this fork was to replace Ashhar's original (incredibly slow) screen drawing routines
with PDQ. Since that change, Ashhar has updated his drawing routine to be MUCH faster than his original, but
still slightly slower than PDQ. It may be that Ashhar's new routines are smaller that PDQ, but I don't actually
know that for certain.

There are a good number of instances of back-to-back calls of strncpy_P and displayText. Creating a single
function that performs these operations together, and then calling that new function instead of the
back-to-back calls everywhere may save space.

# License

The majority of this code is released under GPL v3 license, per Ashhar's original code.

The files in the toneAC2 were copied from https://bitbucket.org/teckel12/arduino-toneac2/wiki/Home, and are GPL v3 licensed.
