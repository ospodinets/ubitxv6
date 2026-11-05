#pragma once

#include <stdint.h>
#include "nano_gui.h"

class Label 
{
public:
    using LabelTextOverride_t = void (*)(char* text_out, const uint16_t max_text_size);
public:
  Label(int16_t x, int16_t y, int16_t w, int16_t h, const char* text = nullptr,
        LabelTextOverride_t text_override = nullptr, Style* style = nullptr);
  void setText(const char* text);
  void setTextOverride(LabelTextOverride_t text_override);
  void setStyle(Style* style);
  void invalidate();

  void draw();

protected:
  int16_t m_x, m_y, m_w, m_h;
  const char* m_text;//nullptr if text_override should be used
  LabelTextOverride_t m_text_override;//nullptr if text should be used
  Style* m_style;//nullptr if default style should be used

  bool m_dirty;
  bool m_has_drawn_text;
  Style m_last_style{};
  char m_last_drawn_text[30]{};//Used to determine if the label needs to be redrawn
};
