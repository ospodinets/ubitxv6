#include "smeter.h"

#include "settings.h"
#include "wiring_time.h"
#include "wiring_analog.h"

#include "pin_definitions.h"

static const uint16_t SMETER_REFRESH_INTERVAL_MS = 100;
static const uint16_t SMETER_ADC_MAX = 1023;

const int16_t SMETER_MAX_VALUE = 15;

void updateSMeter()
{
  static uint32_t last_sample_ms = 0;
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - last_sample_ms) < SMETER_REFRESH_INTERVAL_MS)
  {
    return;
  }
  last_sample_ms = now;

  const uint16_t adc_value = analogRead(PIN_ANALOG_SMETER);
  const uint16_t bounded_adc = adc_value > SMETER_ADC_MAX ? SMETER_ADC_MAX : adc_value;
  // Provisional linear conversion; replace with calibrated thresholds as needed.
  globalSettings.sMeterValue = static_cast<uint8_t>(
      (static_cast<uint32_t>(bounded_adc) * SMETER_MAX_VALUE + SMETER_ADC_MAX / 2) /
      SMETER_ADC_MAX);
}