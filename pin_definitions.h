#pragma once

/*
 * Pin configurations for uBiTXv6 on WeAct BlackPill (STM32F411CEU6)
 * Framework: Arduino (stm32duino)
 *
 * Attached devices are:
 *  * A rotary encoder for tuning, which has a built-in push button for selection
 *  * A Push-to-talk button input
 *  * A morse keyer input, using analog voltage divider for paddle support
 *  * A SI5351A 3-output frequency generator
 *  * A model tjctm24028-spi touch screen LCD, which has:
 *     * An ILI9341 display controller
 *     * A XPT2046 touch controller
 *  * A TX/RX output control pin
 *  * A tone output pin to provide audio feedback to the operator when sending CW
 *  * Three filter selection output control pins
 *  * A CW keydown output
 *
 * NOTES:
 *  - All GPIO used here are 3.3 V logic. Analog inputs (PA1, PA2) are NOT 5 V tolerant.
 *    If the keyer divider / pull-up is fed from 5 V, move it to 3.3 V and recalibrate
 *    the thresholds (and use analogReadResolution(12)).
 *  - PA15, PB3, PB4 are JTAG pins at reset (briefly floating). Fine for inputs.
 *  - Avoid: PA0 (KEY button), PC13 (LED), PC14/PC15 (32 kHz crystal),
 *           PA11/PA12 (USB), PA13/PA14 (SWD), PB2 (BOOT1).
 */

// ---- Controls (inputs) ----
static const uint32_t PIN_ENC_A           = PB4;   // Tuning encoder interface (TIM3_CH1)
static const uint32_t PIN_ENC_B           = PB5;   // Tuning encoder interface (TIM3_CH2)
static const uint32_t PIN_ENC_PUSH_BUTTON = PB3;   // Tuning encoder push button
static const uint32_t PIN_PTT             = PA15;  // Sense it for ssb and as a straight key for cw operation
static const uint32_t PIN_ANALOG_KEYER    = PA1;   // Analog keyer (ADC1_IN1). 3.3 V max! Details: www.hfsignals.com
static const uint32_t PIN_ANALOG_SPARE    = PA2;   // Not used yet (ADC1_IN2)

// ---- I2C1 (Si5351) ----
// PB7 is I2C1 SDA
// PB6 is I2C1 SCL

// ---- SPI1 (TFT + touch) ----
// PA5 is SPI1 SCK; PA6/PA7 are SPI1 MISO/MOSI (configured by the BlackPill variant).
static const uint32_t PIN_TFT_SCLK = PA5;
static const uint32_t PIN_TFT_MISO = PA6;   // SPI1 MISO (TFT + touch)
static const uint32_t PIN_TFT_MOSI = PA7;   // SPI1 MOSI (TFT + touch)
static const uint32_t PIN_TFT_CS   = PA4;   // Selects the LCD controller on SPI interface (active low)
static const uint32_t PIN_TFT_DC   = PB9;   // Tells the LCD controller if it's getting data (D, high) or commands (C, low)
static const uint32_t PIN_TOUCH_CS = PB8;   // Selects the touch controller on SPI interface (active low)
static const uint32_t PIN_TOUCH_SCLK = PIN_TFT_SCLK; // XPT2046 shares SPI1 clock with TFT
static const uint32_t PIN_TOUCH_MISO = PIN_TFT_MISO; // XPT2046 shares SPI1 MISO with TFT
static const uint32_t PIN_TOUCH_MOSI = PIN_TFT_MOSI; // XPT2046 shares SPI1 MOSI with TFT

// ---- Radio control outputs ----
static const uint32_t PIN_TX_RXn   = PA9;   // Pin to the radio to switch to TX (HIGH) and RX (LOW)
static const uint32_t PIN_CW_TONE  = PB10;  // Generates a square wave sidetone while sending the CW
static const uint32_t PIN_TX_LPF_A = PB12;  // The 30 MHz LPF is permanently connected in the output of the PA...
static const uint32_t PIN_TX_LPF_B = PB13;  //  ...Alternatively, either 3.5 MHz, 7 MHz or 14 Mhz LPFs are...
static const uint32_t PIN_TX_LPF_C = PB14;  //  ...switched inline depending upon the TX frequency
// static const uint32_t PIN_TX_LPF_D = PB15; // Spare 4th filter-select line (PB12..PB15 are one contiguous
                                              // 4-bit group: handy for a 74LS145 decoder or more filters)
static const uint32_t PIN_CW_KEY   = PA8;   //  Pin goes high during CW keydown to transmit the carrier.
                                            // ... The PIN_CW_KEY is needed in addition to the TX/RX key as the...
                                            // ...key can be up within a tx period

// Serial debug: use USB CDC (build_flags = -D PIO_FRAMEWORK_ARDUINO_ENABLE_CDC -D USBCON),
// since PA9/PA2 are taken above and the Nano's UART pins 0/1 no longer exist here.