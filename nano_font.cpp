#include "nano_font.h"
#include <avr/pgmspace.h>

#include "Fonts/RobotoCondensed_Bold10pt7b.h"
#include "Fonts/B612Mono_Bold20pt7b_stripped.h"
#include "Fonts/B612Mono_Bold10pt7b_stripped.h"
#include "Fonts/RobotoCondensed_Bold8pt7b.h"

#include "Fonts/Icons.h"


const GFXfont* font_regular()
{
  return &RobotoCondensed_Bold10pt7b;
}
const GFXfont* font_small_titles()
{
  return &RobotoCondensed_Bold8pt7b;
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