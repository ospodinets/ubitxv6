#include "button.h"

#include "color_theme.h"
#include "nano_gui.h"

static Style defaultInactiveStyle = {
    COLOR_INACTIVE_TEXT,
    COLOR_INACTIVE_BACKGROUND,
    COLOR_INACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Regular,
    1
  };

static Style defaultActiveStyle = {
    COLOR_ACTIVE_TEXT,
    COLOR_ACTIVE_BACKGROUND,
    COLOR_ACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Regular,
    1
  };

Button::Button(int16_t x, int16_t y, int16_t w, int16_t h,
               const char* text, LabelTextOverride_t text_override,
               ButtonStatusCallback_t status, ButtonClickHandler_t on_select,
               char morse, Style* inactiveStyle, Style* activeStyle)
    : Label(x, y, w, h, text, text_override, nullptr),
      m_status(status), m_on_select(on_select), m_morse(morse),
      m_inactiveStyle(inactiveStyle), m_activeStyle(activeStyle)
{
}

void Button::draw() const
{
  Style* style = m_inactiveStyle ? m_inactiveStyle : &defaultInactiveStyle;
  if (getStatus() == ButtonStatus_e::Active) {
    style = m_activeStyle ? m_activeStyle : &defaultActiveStyle;
  }
  drawWithStyle(style);
}