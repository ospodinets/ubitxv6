#include "touch.h"

#include <Arduino.h>
#include <XPT2046_Touchscreen.h>

#include "pin_definitions.h"
#include "settings.h"

XPT2046_Touchscreen ts(PIN_TOUCH_CS);

#if TOUCH_USE_IRQ
static volatile bool touchWakePending = true;//Start true so a touch already in progress at boot is seen

//Only flags the touch; the SPI bus is shared with the display, so it must not be used from here
static void touchIsr()
{
  touchWakePending = true;
}

bool touchIrqConsume()
{
  const bool pending = touchWakePending;
  touchWakePending = false;
  return pending;
}
#endif

void initTouch()
{
#if defined(ENABLE_TRACE) && ENABLE_TRACE
  Serial.println(F("TOUCH: initializing"));
#endif
  ts.begin();
  ts.setRotation(1);
#if TOUCH_USE_IRQ
  pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_IRQ), touchIsr, FALLING);
#if defined(ENABLE_TRACE) && ENABLE_TRACE
  Serial.println(F("TOUCH: wake interrupt enabled"));
#endif
#endif
#if defined(ENABLE_TRACE) && ENABLE_TRACE
  Serial.println(F("TOUCH: controller ready (rotation 1)"));
#endif
}

bool readTouch(Point *const touch_point_out)
{
#if defined(ENABLE_TRACE) && ENABLE_TRACE
  static bool wasTouched = false;
#endif
  const bool istouched = ts.touched();
  if (istouched) 
  {
    TS_Point p = ts.getPoint();
    touch_point_out->x = p.x;
    touch_point_out->y = p.y;
#if defined(ENABLE_TRACE) && ENABLE_TRACE
    if (!wasTouched)
    {
      Serial.print(F("TOUCH: contact raw x="));
      Serial.print(p.x);
      Serial.print(F(" y="));
      Serial.println(p.y);
    }
    wasTouched = true;
#endif
    return true;
  }
  else
  {
#if defined(ENABLE_TRACE) && ENABLE_TRACE
    if (wasTouched)
    {
      Serial.println(F("TOUCH: contact released"));
    }
    wasTouched = false;
#endif
    return false;
  }
}

void scaleTouch(Point *const touch_point_in_out)
{
  touch_point_in_out->x = ((long)(touch_point_in_out->x - globalSettings.touchOffsetX) * 10L) / (long)globalSettings.touchSlopeX;
  touch_point_in_out->y = ((long)(touch_point_in_out->y - globalSettings.touchOffsetY) * 10L) / (long)globalSettings.touchSlopeY;
}