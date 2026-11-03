#include "nano_font.h"
#include <avr/pgmspace.h>

#include "PDQ_MinLib/RobotoCondensed_Bold10pt7b.h"
#include "PDQ_MinLib/B612Mono_Bold20pt7b_stripped.h"
#include "PDQ_MinLib/B612Mono_Bold10pt7b_stripped.h"
#include "PDQ_MinLib/Picopixel.h"
#include "PDQ_MinLib/Icons.h"


const GFXfont* font_regular()
{
  return &RobotoCondensed_Bold10pt7b;
}
const GFXfont* font_small_titles()
{
  return &Picopixel;
}
const GFXfont* font_big_digits()
{
  return &B612Mono_Bold20pt7b;
}
const GFXfont* font_small_digits()
{
  return &B612Mono_Bold10pt7b;
}

const GFXfont* font_icons()
{
  return &Icons;
}