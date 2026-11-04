#include "button.h"

#include <string.h>
#include <avr/pgmspace.h>

#include "color_theme.h"
#include "nano_gui.h"
#include "scratch_space.h"

Style defaultInactiveStyle = {
    COLOR_INACTIVE_TEXT,
    COLOR_INACTIVE_BACKGROUND,
    COLOR_INACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Regular,
    1
  };

Style defaultActiveStyle = {
    COLOR_ACTIVE_TEXT,
    COLOR_ACTIVE_BACKGROUND,
    COLOR_ACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Regular,
    1
  };

void drawButton(Button* button)
{
  char b[30];
  Style* style = button->inactiveStyle ? button->inactiveStyle : &defaultInactiveStyle;
  switch(button->status())
  {
    case ButtonStatus_e::Stateless://Fallthrough intended
    case ButtonStatus_e::Inactive://Fallthrough intended
    default:
    {
      //Colors are initialized for this, so we're done
      break;
    }
    case ButtonStatus_e::Active:
    {
      style = button->activeStyle ? button->activeStyle : &defaultActiveStyle;
      break;
    }
  }


  if(nullptr != button->text){
    strncpy_P(b,button->text,sizeof(b));
  }
  else if(nullptr != button->text_override){
    button->text_override(b,sizeof(b));
  }
  else{
    //Something's messed up
    //Serial.println(F("No text for button!"));
    return;
  }
  displayText(b, button->x, button->y, button->w, button->h, *style);
}

void extractAndDrawButton(Button* button_out, const Button* button_P)
{
  memcpy_P(button_out,button_P,sizeof(*button_out));
  drawButton(button_out);
}
