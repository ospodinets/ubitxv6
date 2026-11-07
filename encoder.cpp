#include <Arduino.h>
#include <stm32f4xx_hal.h>

#include "encoder.h"
#include "pin_definitions.h"

static const uint16_t CALLBACK_PERIOD_MS = 200;
static const uint32_t ENCODER_TIMER_MAX = 0xFFFF;
static const int32_t ENCODER_TIMER_RANGE = ENCODER_TIMER_MAX + 1;
static const int32_t ENCODER_TIMER_HALF_RANGE = ENCODER_TIMER_RANGE / 2;

static TIM_HandleTypeDef encoderTimer;
static int32_t enc_count = 0;
static int32_t enc_count_periodic = 0;
static int32_t momentum[3] = {0};
static uint16_t previousEncoderCount = 0;
static uint32_t lastMomentumSampleMs = 0;

void enc_setup(void)
{
  enc_count = 0;
  enc_count_periodic = 0;
  momentum[0] = 0;
  momentum[1] = 0;
  momentum[2] = 0;

  GPIO_InitTypeDef gpioInit = {};
  gpioInit.Pin = GPIO_PIN_4 | GPIO_PIN_5;
  gpioInit.Mode = GPIO_MODE_AF_PP;
  gpioInit.Pull = GPIO_NOPULL;
  gpioInit.Speed = GPIO_SPEED_FREQ_LOW;
  gpioInit.Alternate = GPIO_AF2_TIM3;
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_Init(GPIOB, &gpioInit);

  __HAL_RCC_TIM3_CLK_ENABLE();
  encoderTimer.Instance = TIM3;
  encoderTimer.Init.Prescaler = 0;
  encoderTimer.Init.CounterMode = TIM_COUNTERMODE_UP;
  encoderTimer.Init.Period = ENCODER_TIMER_MAX;
  encoderTimer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  encoderTimer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  TIM_Encoder_InitTypeDef encoderConfig = {};
  encoderConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  encoderConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  encoderConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  encoderConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  encoderConfig.IC1Filter = 0xF;
  encoderConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  encoderConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  encoderConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  encoderConfig.IC2Filter = 0xF;

  if (HAL_TIM_Encoder_Init(&encoderTimer, &encoderConfig) != HAL_OK ||
      HAL_TIM_Encoder_Start(&encoderTimer, TIM_CHANNEL_ALL) != HAL_OK) {
    Error_Handler();
  }

  previousEncoderCount = static_cast<uint16_t>(TIM3->CNT);
  lastMomentumSampleMs = millis();
}

static int32_t readEncoderDelta(void)
{
  const uint16_t currentCount = static_cast<uint16_t>(TIM3->CNT);
  int32_t delta = static_cast<int32_t>(currentCount) - previousEncoderCount;
  previousEncoderCount = currentCount;

  if (delta > ENCODER_TIMER_HALF_RANGE) {
    delta -= ENCODER_TIMER_RANGE;
  } else if (delta < -ENCODER_TIMER_HALF_RANGE) {
    delta += ENCODER_TIMER_RANGE;
  }
  return delta;
}

static uint32_t minMomentumMagnitude(void)
{
  uint32_t minMagnitude = UINT32_MAX;
  for (uint8_t i = 0; i < sizeof(momentum) / sizeof(momentum[0]); ++i) {
    const uint32_t magnitude = momentum[i] < 0
      ? static_cast<uint32_t>(-static_cast<int64_t>(momentum[i]))
      : static_cast<uint32_t>(momentum[i]);
    if (magnitude < minMagnitude) {
      minMagnitude = magnitude;
    }
  }
  return minMagnitude;
}

int enc_read(void)
{
  const int32_t delta = readEncoderDelta();
  enc_count += delta;
  enc_count_periodic += delta;

  const uint32_t now = millis();
  while (now - lastMomentumSampleMs >= CALLBACK_PERIOD_MS) {
    momentum[2] = momentum[1];
    momentum[1] = momentum[0];
    momentum[0] = enc_count_periodic;
    enc_count_periodic = 0;
    lastMomentumSampleMs += CALLBACK_PERIOD_MS;
  }

  if (enc_count == 0) {
    return 0;
  }

  int32_t result = enc_count;
  const int32_t direction = (enc_count < 0) ? -1 : 1;
  const uint32_t momentumMagnitude = minMomentumMagnitude();
  if (momentumMagnitude >= 20) {
    result += direction * 40;
  } else if (momentumMagnitude >= 5) {
    result += direction * static_cast<int32_t>((20 + momentumMagnitude) / (20 - momentumMagnitude));
  }

  enc_count = 0;
  return static_cast<int>(result);
}
