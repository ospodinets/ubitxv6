#pragma once

#include "pin_definitions.h"
#include "point.h"

void initTouch();

#if TOUCH_USE_IRQ
//Returns true if the touch interrupt fired since the last call
bool touchIrqConsume();
#endif
//Returns true if touched, false otherwise
bool readTouch(Point *const touch_point_out);

//Applies the touch calibration the point passed in
void scaleTouch(Point *const touch_point_in_out);