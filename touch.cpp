#include "touch.h"

#include <Arduino.h>
#include <XPT2046_Touchscreen.h>

#include "pin_definitions.h"
#include "settings.h"

XPT2046_Touchscreen ts(PIN_TOUCH_CS);

void initTouch()
{
  Serial.println(F("TOUCH: initializing"));
  ts.begin();
  ts.setRotation(1);
  Serial.println(F("TOUCH: controller ready (rotation 1)"));
}

bool readTouch(Point *const touch_point_out)
{
  static bool wasTouched = false;
  const bool istouched = ts.touched();
  if (istouched) 
  {
    TS_Point p = ts.getPoint();
    touch_point_out->x = p.x;
    touch_point_out->y = p.y;
    if (!wasTouched)
    {
      Serial.print(F("TOUCH: contact raw x="));
      Serial.print(p.x);
      Serial.print(F(" y="));
      Serial.println(p.y);
    }
    wasTouched = true;
    return true;
  }
  else
  {
    if (wasTouched)
    {
      Serial.println(F("TOUCH: contact released"));
    }
    wasTouched = false;
    return false;
  }
}

void scaleTouch(Point *const touch_point_in_out)
{
  touch_point_in_out->x = ((long)(touch_point_in_out->x - globalSettings.touchOffsetX) * 10L) / (long)globalSettings.touchSlopeX;
  touch_point_in_out->y = ((long)(touch_point_in_out->y - globalSettings.touchOffsetY) * 10L) / (long)globalSettings.touchSlopeY;
}