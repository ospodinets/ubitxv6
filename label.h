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
  void invalidate() const;

  void draw();

  int16_t x() const { return m_x; }
  int16_t y() const { return m_y; }
  int16_t width() const { return m_w; }
  int16_t height() const { return m_h; }
  const char* staticText() const { return m_text; }

protected:
  void drawWithStyle(Style* style) const;

  int16_t m_x, m_y, m_w, m_h;
  const char* m_text;//nullptr if text_override should be used
  LabelTextOverride_t m_text_override;//nullptr if text should be used
  Style* m_style;//nullptr if default style should be used

  mutable bool m_dirty;
  mutable bool m_has_drawn_text;
  mutable uint32_t m_last_display_generation;
  mutable Style m_last_style{};
  mutable char m_last_drawn_text[30]{};//Used to determine if the label needs to be redrawn
};
