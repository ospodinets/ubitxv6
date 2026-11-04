#include "button.h"

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

void drawButton(const Button* button)
{
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
    displayText(button->text, button->x, button->y, button->w, button->h, *style);
  }
  else if(nullptr != button->text_override){
    char text[30] = {0};
    button->text_override(text,sizeof(text));
    text[sizeof(text) - 1] = '\0';
    displayText(text, button->x, button->y, button->w, button->h, *style);
  }
  else{
    //Something's messed up
    //Serial.println(F("No text for button!"));
    return;
  }
}
