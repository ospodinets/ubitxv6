#include "nano_font.h"
#include <avr/pgmspace.h>

#include "PDQ_MinLib/FreeSansBold9pt7b.h"
#include "PDQ_MinLib/Picopixel.h"
#include "PDQ_MinLib/OxygenMono_Regular20pt7b.h"
//#include "PDQ_MinLib/org_01.h"
//#include "PDQ_MinLib/TomThumb.h"

const GFXfont* font_sans_bold()
{
  return &FreeSansBold9pt7b;
}

const GFXfont* font_picopixel()
{
  return &Picopixel;
}

const GFXfont* font_oxygen_mono()
{
  return &OxygenMono_Regular20pt7b;
}