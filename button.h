#pragma once

#include "label.h"

enum ButtonStatus_e : uint8_t {
  Stateless,
  Inactive,
  Active
};

class Button : public Label
{
public:
  using ButtonStatusCallback_t = ButtonStatus_e (*)();
  using ButtonClickHandler_t = void (*)();

  Button(int16_t x, int16_t y, int16_t w, int16_t h,
         const char* text, LabelTextOverride_t text_override,
         ButtonStatusCallback_t status, ButtonClickHandler_t on_select,
         char morse, Style* inactiveStyle = nullptr, Style* activeStyle = nullptr);

  void draw() const;

  int16_t x() const { return m_x; }
  int16_t y() const { return m_y; }
  int16_t width() const { return m_w; }
  int16_t height() const { return m_h; }
  const char* staticText() const { return m_text; }
  ButtonStatus_e getStatus() const { return m_status(); }
  void select() const { m_on_select(); }
  char morse() const { return m_morse; }

private:
  ButtonStatusCallback_t m_status;
  ButtonClickHandler_t m_on_select;
  char m_morse;
  Style* m_inactiveStyle;
  Style* m_activeStyle;
};
